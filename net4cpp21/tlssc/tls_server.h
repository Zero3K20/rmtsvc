#pragma once

// tls_server.h – TLS 1.2 server-side wrapper built on the same primitives as tls_client.
//
// Usage:
//   1. Accept a TCP socket with ::accept().
//   2. Construct tls_server_conn, passing the socket, a DER-encoded X.509 certificate
//      whose Subject Public Key is a P-256 (secp256r1) key, and the matching raw 32-byte
//      private key (big-endian).
//   3. Call handshake().  Returns nullptr on success; an error string on failure.
//   4. Use send() / recv() for application data, just like tls_client.
//   5. Call close() when done.
//
// Supported cipher suites (server preference order):
//   TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256  (0xC0,0x2B)
//   TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384  (0xC0,0x2C)
//
// The server performs a standard TLS 1.2 full handshake:
//   ← ClientHello
//   → ServerHello
//   → Certificate
//   → ServerKeyExchange  (ephemeral ECDHE, ECDSA-signed with privkey)
//   → ServerHelloDone
//   ← ClientKeyExchange
//   ← ChangeCipherSpec
//   ← Finished
//   → ChangeCipherSpec
//   → Finished

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <WinSock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdarg.h>
#include "tlsclient.cpp"

// ---------------------------------------------------------------------------
// Verbose debug-logging helper.
// When cLogger.h has already been included (i.e. its include-guard is defined)
// we emit messages at DEBUG level through the application's logger.
// Otherwise the macro is a no-op so this header can still be used standalone.
// ---------------------------------------------------------------------------
#ifndef TLS_SERVER_DEBUG
#  ifdef __YY_CLOGGER_H__
#    define TLS_SERVER_DEBUG(...) \
         if(net4cpp21::cLogger::getInstance().ifOutPutLog(net4cpp21::LOGLEVEL_DEBUG)) \
             net4cpp21::cLogger::getInstance().debug(__VA_ARGS__)
#  else
#    define TLS_SERVER_DEBUG(...) ((void)0)
#  endif
#endif

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// DER-encode a raw ECDSA signature (r || s, each ecc_bytes long) into 'out'.
// Returns the number of bytes written, or -1 if out_size is too small.
static int tls_server_ecdsa_raw_to_der(const uint8_t *sig, int ecc_bytes,
                                       uint8_t *out, int out_size)
{
    const uint8_t *r = sig;
    const uint8_t *s = sig + ecc_bytes;

    // Add a leading 0x00 if the high bit is set (ASN.1 INTEGER is signed).
    int r_pad = (r[0] & 0x80) ? 1 : 0;
    int s_pad = (s[0] & 0x80) ? 1 : 0;
    int r_len = ecc_bytes + r_pad;
    int s_len = ecc_bytes + s_pad;

    int seq_len = 2 + r_len + 2 + s_len;   // 02 LL r  02 LL s
    int total   = 2 + seq_len;              // 30 LL ...

    if(total > out_size)
        return -1;

    uint8_t *p = out;
    *p++ = 0x30;                    // SEQUENCE
    *p++ = (uint8_t)seq_len;

    *p++ = 0x02;                    // INTEGER r
    *p++ = (uint8_t)r_len;
    if(r_pad) *p++ = 0x00;
    memcpy(p, r, ecc_bytes); p += ecc_bytes;

    *p++ = 0x02;                    // INTEGER s
    *p++ = (uint8_t)s_len;
    if(s_pad) *p++ = 0x00;
    memcpy(p, s, ecc_bytes); p += ecc_bytes;

    return total;
}

// ---------------------------------------------------------------------------
// tls_server_conn
// ---------------------------------------------------------------------------

// RFC 5246 §6.2.1: maximum TLS record plaintext payload is 2^14 bytes.
// Encrypted records may carry up to 2048 bytes of additional overhead
// (IV, MAC, padding), so accept up to this ceiling before rejecting.
static const int TLS_MAX_RECORD_PAYLOAD  = 16384;
static const int TLS_MAX_ENCRYPTED_OVERHEAD = 2048;

class tls_server_conn
{
    // -- state ----------------------------------------------------------------
    SOCKET          s               = INVALID_SOCKET;
    tls_cipher      crypto;

    const unsigned char *cert_der       = nullptr; // DER-encoded X.509 certificate (leaf)
    int                  cert_der_len   = 0;
    const unsigned char *privkey        = nullptr; // 32-byte raw P-256 private key

    // Full certificate chain: intermediate cert(s) to append after the leaf.
    // Each entry is prefixed with a 3-byte big-endian length, concatenated.
    const unsigned char *chain_der      = nullptr;
    int                  chain_der_len  = 0;

    // Optional fallback cert+key used when the SNI hostname does not match
    // the primary certificate's domain (e.g. internal LAN hostname vs. ACME domain).
    const unsigned char *fallback_cert_der     = nullptr;
    int                  fallback_cert_der_len = 0;
    const unsigned char *fallback_privkey      = nullptr;
    // Primary cert domain: compared against the SNI name to decide which cert to use.
    const char          *cert_domain           = nullptr;

    // SNI hostname parsed from ClientHello (NUL-terminated, empty if not present).
    char sni_name[256]  = {};

    tlsbuf  send_buf;
    tlsbuf  recv_buf;
    tlsbuf  recv_channel;
    tlsbuf  err_msg;
    int     recv_channel_readed     = 0;
    int     time_out                = 0x7fffffff;
    bool    handshake_done          = false;
    bool    received_close_notify   = false;
    bool    use_ems                 = false;   // extended_master_secret (RFC 7627)
    bool    client_tls12_not_offered = false;  // true if supported_versions excludes TLS 1.2
    const char *m_step              = "";      // current handshake step (for diagnostics)

    // -- private helpers ------------------------------------------------------

    bool is_tls13(TLS_CIPHER cipher)
    {
        return cipher >= TLS_AES_128_GCM_SHA256 && cipher <= TLS_AES_128_CCM_8_SHA256;
    }

    int set_err(const char *msg, int ret)
    {
        int len = (int)strlen(msg) + 1;
        err_msg.set_size(len);
        memcpy(err_msg.buf, msg, len);
        return ret;
    }

    // Build a formatted error string into err_msg.buf and return it.
    const char *set_errf(const char *fmt, ...)
    {
        char tmp[256];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(tmp, sizeof(tmp), fmt, ap);
        va_end(ap);
        tmp[sizeof(tmp)-1] = '\0';
        set_err(tmp, -1);
        return err_msg.buf;
    }

    // When a plain HTTP client hits the HTTPS port, send a 301 redirect to HTTPS
    // so the browser automatically retries with the correct scheme.
    void try_send_http_redirect()
    {
        if(recv_buf.size < 4) return;
        const char *p = recv_buf.buf;
        int  n = recv_buf.size;

        // HTTP request lines begin with an uppercase method (GET, POST, HEAD …).
        if(!(p[0] >= 'A' && p[0] <= 'Z')) return;

        // Find the end of the request line.
        const char *req_end = (const char *)memchr(p, '\n', n);
        if(!req_end) return;

        // Extract the request-target (second token: "GET <target> HTTP/1.1").
        const char *path = (const char *)memchr(p, ' ', req_end - p);
        if(!path) return;
        path++;   // skip the leading space
        const char *path_end = (const char *)memchr(path, ' ', req_end - path);
        if(!path_end) path_end = req_end;

        // Default host – will be overridden by the "Host:" header.
        char host[512] = "localhost";
        bool found_host = false;

        // Scan request headers for "Host:".
        const char *hdr = req_end + 1;
        int hdr_len = n - (int)(hdr - p);
        for(int i = 0; i + 5 < hdr_len; i++)
        {
            if((hdr[i]   == 'H' || hdr[i]   == 'h') &&
               (hdr[i+1] == 'o' || hdr[i+1] == 'O') &&
               (hdr[i+2] == 's' || hdr[i+2] == 'S') &&
               (hdr[i+3] == 't' || hdr[i+3] == 'T') &&
                hdr[i+4] == ':')
            {
                const char *hv = hdr + i + 5;
                while(hv < hdr + hdr_len && (*hv == ' ' || *hv == '\t')) hv++;
                const char *he = hv;
                while(he < hdr + hdr_len && *he != '\r' && *he != '\n') he++;
                int hl = (int)(he - hv);
                if(hl > 0 && hl < (int)sizeof(host))
                {
                    memcpy(host, hv, hl);
                    host[hl] = '\0';
                    found_host = true;
                }
                break;
            }
        }

        // Copy the path (guard against oversized targets, and validate it starts
        // with '/' to prevent open-redirect via an absolute-URI request-target).
        int plen = (int)(path_end - path);
        char pathbuf[2048] = "/";
        if(plen > 0 && plen < (int)sizeof(pathbuf) && path[0] == '/')
        {
            memcpy(pathbuf, path, plen);
            pathbuf[plen] = '\0';
            // Strip any CR/LF that would allow HTTP response-header injection.
            for(int i = 0; i < plen; i++)
                if(pathbuf[i] == '\r' || pathbuf[i] == '\n')
                    { pathbuf[i] = '\0'; break; }
        }

        // Only redirect if we found a valid Host header to redirect to.
        if(!found_host) return;

        char resp[3072];
        int  resp_len = snprintf(resp, sizeof(resp),
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://%s%s\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n",
            host, pathbuf);
        if(resp_len > 0 && resp_len < (int)sizeof(resp))
            ::send(s, resp, resp_len, 0);
    }

    // Send one TLS record. Updates the transcript hash for handshake records.
    const char *send_packet(int packet_type, int ver, tlsbuf &buf)
    {
        if(packet_type == CONTENT_HANDSHAKE && buf.size > 0)
            crypto.update_hash(buf.buf, buf.size);

        tlsbuf tmp;
        tmp.append((char)packet_type);
        tmp.append((short)ver);
        int body_size_idx = tmp.append_size(2);

        bool keep_original = (packet_type == CONTENT_CHANGECIPHERSPEC ||
                              packet_type == CONTENT_ALERT);
        crypto.encode(tmp, buf.buf, buf.size, keep_original, false /* TLS 1.2 only */);

        *(u_short *)(tmp.buf + body_size_idx) = htons(tmp.size - body_size_idx - 2);
        int sent = 0;
        while(sent < tmp.size)
        {
            int n = ::send(s, tmp.buf + sent, tmp.size - sent, 0);
            if(n <= 0)
            {
                int wsa_err = (int)WSAGetLastError(); // capture before any other call
                return set_errf("send failed [%s] (WSAError=%d)",
                                m_step && *m_step ? m_step : "?", wsa_err);
            }
            sent += n;
        }
        return nullptr;
    }

    // Blocking recv into recv_buf; parse and process one or more TLS records,
    // appending application data to recv_channel.
    // Returns an error string, or nullptr on success.
    const char *recv_records(int wait_sec = 5)
    {
        while(true)
        {
            // Try to read from the socket when we don't yet have a complete record.
            if(recv_buf.size < 5 ||
               recv_buf.size < 5 + ntohs(*(unsigned short *)(recv_buf.buf + 3)))
            {
                fd_set set;
                FD_ZERO(&set);
                FD_SET(s, &set);
                timeval tv = { wait_sec, 0 };
                int r = select((int)(s + 1), &set, nullptr, nullptr, &tv);
                if(r <= 0)
                {
                    if(r == 0)
                        return set_errf("timeout [%s] (waited %ds)",
                                        m_step && *m_step ? m_step : "?",
                                        wait_sec);
                    int wsa_err = (int)WSAGetLastError(); // capture before any other call
                    return set_errf("select error [%s] (WSAError=%d)",
                                    m_step && *m_step ? m_step : "?", wsa_err);
                }

                recv_buf.check_size(recv_buf.size + 4096 * 4);
                int len = ::recv(s, recv_buf.buf + recv_buf.size, 4096 * 4, 0);
                if(len <= 0)
                    return "connection closed";
                recv_buf.size += len;
            }

            // Check we have at least one full record.
            if(recv_buf.size < 5)
                continue;

            // Validate TLS record header to fast-fail on non-TLS connections
            // (e.g. plain HTTP clients hitting the HTTPS port).
            unsigned char rec_type = (unsigned char)recv_buf.buf[0];
            if(rec_type < CONTENT_CHANGECIPHERSPEC || rec_type > CONTENT_APPLICATION_DATA)
                return "not a TLS record";
            int pkt_size = ntohs(*(unsigned short *)(recv_buf.buf + 3));
            // RFC 5246: maximum TLS record payload is 2^14 bytes (16384).
            // Allow a small overhead for encrypted records (up to 2048 bytes).
            if(pkt_size > TLS_MAX_RECORD_PAYLOAD + TLS_MAX_ENCRYPTED_OVERHEAD)
                return "TLS record too large";
            if(recv_buf.size < 5 + pkt_size)
                continue;

            return nullptr; // at least one full record is ready
        }
    }

    // Read exactly one TLS record from recv_buf, decrypt if needed, and
    // return its type, version and payload via out-params.
    // Advances recv_buf past the consumed record.
    const char *consume_record(int &out_type, int &out_ver,
                               tlsbuf &out_payload)
    {
        if(recv_buf.size < 5)
            return "no record";
        int pkt_size = ntohs(*(unsigned short *)(recv_buf.buf + 3));
        if(recv_buf.size < 5 + pkt_size)
            return "incomplete record";

        out_type = (unsigned char)recv_buf.buf[0];
        out_ver  = ntohs(*(u_short *)(recv_buf.buf + 1));

        tlsbuf_reader reader(recv_buf.buf + 5, pkt_size);

        if(crypto.get_encoding())
        {
            const char *err = crypto.decode(reader, out_type, out_ver, false);
            if(err)
                return err;
        }

        out_payload.clear();
        out_payload.append(reader.buf, reader.buf_size);

        // Advance recv_buf.
        int consumed = 5 + pkt_size;
        memmove(recv_buf.buf, recv_buf.buf + consumed, recv_buf.size - consumed);
        recv_buf.size -= consumed;

        return nullptr;
    }

    // -- handshake send helpers -----------------------------------------------

    const char *send_server_hello(TLS_CIPHER cipher,
                                  const unsigned char *server_rand,
                                  const unsigned char *session_id,
                                  int session_id_len)
    {
        send_buf.clear();
        send_buf.append((char)MSG_SERVER_HELLO);
        int hs_size_idx = send_buf.append_size(3);

        send_buf.append((short)htons(0x0303));          // TLS 1.2
        send_buf.append(server_rand, RAND_SIZE);
        send_buf.append((char)session_id_len);
        if(session_id_len > 0)
            send_buf.append(session_id, session_id_len);
        send_buf.append((short)htons((short)cipher));   // chosen cipher
        send_buf.append((char)0);                       // no compression

        // Include renegotiation_info extension (RFC 5746) for the initial handshake:
        // this is required by modern browsers (e.g. Chrome/BoringSSL). Without it,
        // Chrome sends a fatal alert and closes the connection.
        // renegotiation_info: type(2) + ext_length(2) + ri_length(1) = 5 bytes total.
        // Echo extended_master_secret (RFC 7627) if the client offered it: 4 bytes.
        int ext_total = 5; // renegotiation_info always present
        if(use_ems) ext_total += 4;
        send_buf.append((short)htons((u_short)ext_total));  // extensions total length

        // renegotiation_info extension: empty renegotiated_connection for initial handshake
        send_buf.append((short)htons(0xFF01));  // ext type: renegotiation_info
        send_buf.append((short)htons(1));       // ext data length = 1
        send_buf.append((char)0);               // renegotiated_connection length = 0

        // extended_master_secret extension (RFC 7627)
        if(use_ems)
        {
            send_buf.append((short)htons(0x0017));  // ext type: extended_master_secret
            send_buf.append((short)htons(0));       // ext data length = 0
        }

        // Fill in the 3-byte handshake length.
        int body = send_buf.size - hs_size_idx - 3;
        send_buf.buf[hs_size_idx]     = 0;
        *(u_short *)(send_buf.buf + hs_size_idx + 1) = htons((u_short)body);

        return send_packet(CONTENT_HANDSHAKE, 0x0303, send_buf);
    }

    const char *send_certificate()
    {
        send_buf.clear();
        send_buf.append((char)MSG_CERTIFICATE);
        int hs_size_idx = send_buf.append_size(3);

        // Certificate list total length (3 bytes).
        // Includes the leaf cert (3-byte length prefix + DER) plus any chain certs
        // that are already stored with their 3-byte length prefixes.
        int list_len = 3 + cert_der_len;
        if(chain_der && chain_der_len > 0)
            list_len += chain_der_len;
        send_buf.append((char)(list_len >> 16));
        send_buf.append((short)htons((u_short)(list_len & 0xFFFF)));

        // Leaf certificate length (3 bytes) + DER bytes.
        send_buf.append((char)(cert_der_len >> 16));
        send_buf.append((short)htons((u_short)(cert_der_len & 0xFFFF)));
        send_buf.append(cert_der, cert_der_len);

        // Intermediate / chain certificates (already prefixed with 3-byte lengths).
        if(chain_der && chain_der_len > 0)
            send_buf.append((char*)chain_der, chain_der_len);

        int body = send_buf.size - hs_size_idx - 3;
        send_buf.buf[hs_size_idx]     = 0;
        *(u_short *)(send_buf.buf + hs_size_idx + 1) = htons((u_short)body);

        return send_packet(CONTENT_HANDSHAKE, 0x0303, send_buf);
    }

    // Generate the ephemeral ECDH key pair (secp256r1), fill in *pubkey_out
    // (65 bytes: 0x04 | x | y), sign the ServerKeyExchange body, and send it.
    const char *send_server_key_exchange(const unsigned char *client_rand,
                                         const unsigned char *server_rand,
                                         unsigned char *pubkey_out,
                                         int *pubkey_len_out)
    {
        // Generate ephemeral ECDH key pair.
        // tls_cipher::compute_pubkey uses ecc_index 0 = secp256r1 (32 bytes).
        tlsbuf server_pubkey_buf;
        const char *ret = crypto.compute_pubkey(0, server_pubkey_buf);
        if(ret) return ret;

        *pubkey_len_out = server_pubkey_buf.size;
        memcpy(pubkey_out, server_pubkey_buf.buf, server_pubkey_buf.size);

        // Build the signed portion:
        //   curve_type (1) || named_curve (2) || pubkey_len (1) || pubkey
        // This is the ServerECDHParams structure.
        uint8_t params[4 + 65]; // at most 1 + 2 + 1 + 65
        params[0] = 0x03;                          // named_curve
        params[1] = 0x00; params[2] = 0x17;        // secp256r1
        params[3] = (uint8_t)*pubkey_len_out;
        memcpy(params + 4, pubkey_out, *pubkey_len_out);
        int params_len = 4 + *pubkey_len_out;

        // Hash: SHA-256(client_random || server_random || params)
        sha256_ctx hctx;
        sha256_init(&hctx);
        sha256_update(&hctx, client_rand,  RAND_SIZE);
        sha256_update(&hctx, server_rand,  RAND_SIZE);
        sha256_update(&hctx, params, params_len);
        uint8_t hash[32];
        sha256_final(&hctx, hash);

        // ECDSA sign the hash using the server's static private key.
        EccState ecc_state;
        if(ecc_init(&ecc_state, 32 /*secp256r1*/) != 0)
            return "ecc_init failed";

        uint8_t raw_sig[64]; // r || s (each 32 bytes for P-256)
        if(!ecdsa_sign(&ecc_state, privkey, hash, raw_sig))
            return "ecdsa_sign failed";

        uint8_t der_sig[80]; // 2 + 2*(2+32+1) = 72 + 6 overhead ≤ 80
        int der_sig_len = tls_server_ecdsa_raw_to_der(raw_sig, 32, der_sig, sizeof(der_sig));
        if(der_sig_len < 0)
            return "DER encoding of signature failed";

        // Build the ServerKeyExchange handshake message.
        send_buf.clear();
        send_buf.append((char)MSG_SERVER_KEY_EXCHANGE);
        int hs_size_idx = send_buf.append_size(3);

        // ServerECDHParams
        send_buf.append(params, params_len);

        // DigitallySigned: hash_alg (SHA-256=4), sig_alg (ECDSA=3), length, signature
        send_buf.append((char)4);                       // SHA-256
        send_buf.append((char)3);                       // ECDSA
        send_buf.append((short)htons((u_short)der_sig_len));
        send_buf.append(der_sig, der_sig_len);

        int body = send_buf.size - hs_size_idx - 3;
        send_buf.buf[hs_size_idx]     = 0;
        *(u_short *)(send_buf.buf + hs_size_idx + 1) = htons((u_short)body);

        return send_packet(CONTENT_HANDSHAKE, 0x0303, send_buf);
    }

    const char *send_server_hello_done()
    {
        send_buf.clear();
        send_buf.append((char)MSG_SERVER_HELLO_DONE);
        send_buf.append((char)0);
        send_buf.append((short)htons(0)); // empty body
        return send_packet(CONTENT_HANDSHAKE, 0x0303, send_buf);
    }

    const char *send_change_cipherspec()
    {
        send_buf.clear();
        send_buf.append((char)1);
        return send_packet(CONTENT_CHANGECIPHERSPEC, 0x0303, send_buf);
    }

    const char *send_server_finish()
    {
        tlsbuf verify;
        crypto.compute_verify(verify, 1 /*server finished*/, 12, false, 0);

        send_buf.clear();
        send_buf.append((char)MSG_FINISHED);
        send_buf.append((char)0);
        send_buf.append((short)htons((u_short)verify.size));
        send_buf.append(verify.buf, verify.size);
        return send_packet(CONTENT_HANDSHAKE, 0x0303, send_buf);
    }

    // -- handshake receive helpers --------------------------------------------

    // Parse a ClientHello from a raw handshake body (starting after the record
    // header, i.e. pointing at the MSG_CLIENT_HELLO byte).
    const char *parse_client_hello(const char *data, int data_len,
                                   unsigned char *client_rand_out,
                                   TLS_CIPHER &chosen_cipher_out,
                                   unsigned char *session_id_out,
                                   int &session_id_len_out)
    {
        tlsbuf_reader r(const_cast<char*>(data), data_len);

        if(r.buf_size < 1) return "truncated ClientHello";
        int hs_type = r.read<unsigned char>();
        if(hs_type != MSG_CLIENT_HELLO)
            return "expected ClientHello";

        // 3-byte handshake body length.
        if(r.buf_size - r.readed < 3) return "truncated ClientHello";
        int hs_len = (r.read<unsigned char>() << 16) |
                     (unsigned)(ntohs(r.read<short>()) & 0xFFFF);
        (void)hs_len;

        // Client version.
        if(r.buf_size - r.readed < 2) return "truncated";
        int client_ver = (int)(unsigned short)ntohs(r.read<short>());
        TLS_SERVER_DEBUG("[SSL] server: ClientHello received, client_version=0x%04X\r\n",
                         client_ver);

        // Client random (32 bytes).
        if(r.buf_size - r.readed < (int)RAND_SIZE) return "truncated";
        r.read(reinterpret_cast<char*>(client_rand_out), RAND_SIZE);

        // Session ID.
        if(r.buf_size - r.readed < 1) return "truncated";
        session_id_len_out = r.read<unsigned char>();
        if(session_id_len_out > 32) return "invalid session id length";
        if(r.buf_size - r.readed < session_id_len_out) return "truncated";
        r.read(reinterpret_cast<char*>(session_id_out), session_id_len_out);

        // Cipher suites.
        if(r.buf_size - r.readed < 2) return "truncated";
        int cs_len = (int)ntohs(r.read<short>());
        if(r.buf_size - r.readed < cs_len) return "truncated";

        // We prefer ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, then AES_256_GCM_SHA384.
        static const TLS_CIPHER preferred[] = {
            TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
            TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
        };
        chosen_cipher_out = TLS_NONE;
        int cs_start = r.readed;
        for(int i = 0; i < cs_len / 2; ++i)
        {
            TLS_CIPHER c = (TLS_CIPHER)ntohs(r.read<short>());
            if(chosen_cipher_out == TLS_NONE)
            {
                for(auto &p : preferred)
                    if(c == p) { chosen_cipher_out = c; break; }
            }
        }
        r.readed = cs_start + cs_len; // ensure we skip the full list

        if(chosen_cipher_out == TLS_NONE)
        {
            TLS_SERVER_DEBUG("[SSL] server: no supported cipher in ClientHello"
                             " (%d suite(s) offered)\r\n", cs_len / 2);
            return "no supported cipher suite offered by client";
        }

        // Parse compression methods.
        if(r.buf_size - r.readed < 1) return nullptr;
        int comp_len = r.read<unsigned char>();
        if(r.buf_size - r.readed < comp_len) return nullptr;
        r.readed += comp_len;

        // Parse extensions to detect extended_master_secret (RFC 7627),
        // supported_versions (RFC 8446 §4.2.1), and server_name (SNI, RFC 6066).
        // All fields are optional.
        if(r.buf_size - r.readed >= 2)
        {
            int ext_total = (int)(unsigned short)ntohs(r.read<short>());
            int ext_end   = r.readed + ext_total;
            while(r.readed + 4 <= ext_end && r.readed + 4 <= r.buf_size)
            {
                int ext_type = (int)(unsigned short)ntohs(r.read<short>());
                int ext_len  = (int)(unsigned short)ntohs(r.read<short>());
                int ext_data_end = r.readed + ext_len;
                if(ext_type == 0x0000 /* server_name (SNI) */ && ext_len >= 5)
                {
                    // ServerNameList: list_len(2) name_type(1) hostname_len(2) hostname
                    // We only care about name_type 0 (host_name).
                    int off = r.readed;
                    if(off + 2 <= r.buf_size)
                    {
                        int list_len = ((int)(unsigned char)r.buf[off] << 8) |
                                        (int)(unsigned char)r.buf[off+1];
                        off += 2;
                        if(off + list_len <= ext_data_end && off + 3 <= r.buf_size)
                        {
                            int name_type = (int)(unsigned char)r.buf[off];
                            if(name_type == 0 /* host_name */)
                            {
                                int hlen = ((int)(unsigned char)r.buf[off+1] << 8) |
                                            (int)(unsigned char)r.buf[off+2];
                                off += 3;
                                if(hlen > 0 && off + hlen <= ext_data_end &&
                                   hlen < (int)sizeof(sni_name) - 1)
                                {
                                    memcpy(sni_name, r.buf + off, hlen);
                                    sni_name[hlen] = '\0';
                                }
                            }
                        }
                    }
                }
                else if(ext_type == 0x0017 /* extended_master_secret */ && ext_len == 0)
                {
                    use_ems = true;
                }
                else if(ext_type == 0x002B /* supported_versions */ && ext_len >= 1)
                {
                    // The supported_versions list: one-byte count + two-byte version
                    // entries (RFC 8446 §4.2.1).  Read directly from the buffer
                    // without advancing r.readed; line 641 handles the skip.
                    int sv_off = 0;
                    if(r.readed + sv_off < r.buf_size)
                    {
                        int sv_list_len = (int)(unsigned char)r.buf[r.readed + sv_off];
                        sv_off++;
                        // Bounds: sv_list_len must fit within the remaining ext data.
                        if(sv_off + sv_list_len <= ext_len)
                        {
                            bool found_tls12 = false;
                            for(int sv_i = 0; sv_i + 1 < sv_list_len; sv_i += 2)
                            {
                                int pos = r.readed + sv_off + sv_i;
                                if(pos + 1 >= r.buf_size) break;
                                int ver = ((int)(unsigned char)r.buf[pos]     << 8) |
                                           (int)(unsigned char)r.buf[pos + 1];
                                if(ver == 0x0303) found_tls12 = true;
                            }
                            // If the extension is present but TLS 1.2 is not listed,
                            // the client did not offer TLS 1.2 via supported_versions.
                            if(!found_tls12)
                                client_tls12_not_offered = true;
                        }
                    }
                    // r.readed is NOT advanced here; the assignment below sets it to ext_data_end.
                }
                if(ext_data_end > ext_end || ext_data_end > r.buf_size) break;
                r.readed = ext_data_end;
            }
        }

        TLS_SERVER_DEBUG("[SSL] server: chosen cipher=0x%04X, EMS=%d\r\n",
                         (int)chosen_cipher_out, (int)use_ems);

        return nullptr;
    }

    // Parse a ClientKeyExchange for ECDHE: extract the client's ECDH public key.
    const char *parse_client_key_exchange(const char *data, int data_len,
                                          unsigned char *client_pubkey_out,
                                          int &client_pubkey_len_out)
    {
        tlsbuf_reader r(const_cast<char*>(data), data_len);
        if(r.buf_size < 1) return "truncated ClientKeyExchange";
        int hs_type = r.read<unsigned char>();
        if(hs_type != MSG_CLIENT_KEY_EXCHANGE)
            return "expected ClientKeyExchange";

        // 3-byte length.
        if(r.buf_size - r.readed < 3) return "truncated";
        int hs_len = (r.read<unsigned char>() << 16) |
                     (unsigned)(ntohs(r.read<short>()) & 0xFFFF);
        (void)hs_len;

        // 1-byte length prefix for the ECDH public key point.
        if(r.buf_size - r.readed < 1) return "truncated";
        client_pubkey_len_out = r.read<unsigned char>();
        if(r.buf_size - r.readed < client_pubkey_len_out) return "truncated";
        r.read(reinterpret_cast<char*>(client_pubkey_out), client_pubkey_len_out);
        return nullptr;
    }

    // Verify the client's Finished message.
    const char *verify_client_finished(const char *data, int data_len)
    {
        tlsbuf_reader r(const_cast<char*>(data), data_len);
        if(r.buf_size < 1) return "truncated Finished";
        int hs_type = r.read<unsigned char>();
        if(hs_type != MSG_FINISHED)
            return "expected Finished";

        // 3-byte length.
        if(r.buf_size - r.readed < 3) return "truncated";
        int fin_len = (r.read<unsigned char>() << 16) |
                      (unsigned)(ntohs(r.read<short>()) & 0xFFFF);

        tlsbuf expected;
        crypto.compute_verify(expected, 0 /*client finished*/, fin_len, false, 0);

        if(r.buf_size - r.readed < fin_len) return "truncated verify_data";
        if(memcmp(expected.buf, r.buf + r.readed, fin_len) != 0)
            return "Finished verify_data mismatch";
        return nullptr;
    }

    // Receive and decrypt application data into recv_channel.
    const char *process_recv()
    {
        if(s == INVALID_SOCKET) return nullptr;

        recv_buf.check_size(recv_buf.size + 4096 * 4);
        int len = ::recv(s, recv_buf.buf + recv_buf.size, 4096 * 4, 0);
        if(len <= 0)
            throw "connection closed";
        recv_buf.size += len;

        int cur = 0;
        while(cur + 5 <= recv_buf.size)
        {
            int pkt_size = ntohs(*(unsigned short *)(recv_buf.buf + cur + 3));
            if(cur + 5 + pkt_size > recv_buf.size) break;

            int ptype = (unsigned char)recv_buf.buf[cur];
            int pver  = ntohs(*(u_short *)(recv_buf.buf + cur + 1));
            tlsbuf_reader rdr(recv_buf.buf + cur + 5, pkt_size);

            if(crypto.get_encoding())
            {
                const char *err = crypto.decode(rdr, ptype, pver, false);
                if(err) throw err;
            }

            if(ptype == CONTENT_APPLICATION_DATA)
                recv_channel.append(rdr.buf, rdr.buf_size);
            else if(ptype == CONTENT_ALERT && rdr.buf_size >= 2)
            {
                int code = (unsigned char)rdr.buf[1];
                if(code == 0) { received_close_notify = true; }
                else if((unsigned char)rdr.buf[0] == 2) throw "fatal alert";
            }

            cur += 5 + pkt_size;
        }
        memmove(recv_buf.buf, recv_buf.buf + cur, recv_buf.size - cur);
        recv_buf.size -= cur;
        return nullptr;
    }

    int read_channel(char *out, int size)
    {
        int n = min(size, recv_channel.size - recv_channel_readed);
        memcpy(out, recv_channel.buf + recv_channel_readed, n);
        recv_channel_readed += n;
        if(recv_channel_readed >= recv_channel.size ||
           (recv_channel_readed > recv_channel.size / 4 * 3 &&
            recv_channel.size > 1024 * 1024))
        {
            memmove(recv_channel.buf,
                    recv_channel.buf + recv_channel_readed,
                    recv_channel.size - recv_channel_readed);
            recv_channel.size -= recv_channel_readed;
            recv_channel_readed = 0;
        }
        return n;
    }

public:
    // -------------------------------------------------------------------------
    // Public interface
    // -------------------------------------------------------------------------

    tls_server_conn() {}
    ~tls_server_conn() { close(); }

    // Initialise the connection.
    //   sock          – already-accepted TCP socket
    //   cert_der_buf  – DER-encoded X.509 certificate (P-256 key)
    //   cert_der_size – byte length of cert_der_buf
    //   privkey_buf   – 32-byte raw P-256 private key (big-endian)
    void init(SOCKET sock,
              const unsigned char *cert_der_buf, int cert_der_size,
              const unsigned char *privkey_buf)
    {
        s            = sock;
        cert_der     = cert_der_buf;
        cert_der_len = cert_der_size;
        privkey      = privkey_buf;
    }

    // Set intermediate/chain certificates to send after the leaf cert.
    // chain_buf must remain valid for the lifetime of this connection.
    // Each cert in chain_buf must be prefixed with a 3-byte big-endian length.
    void set_chain(const unsigned char *chain_buf, int chain_len)
    {
        chain_der     = chain_buf;
        chain_der_len = chain_len;
    }

    // Set a fallback certificate and key used when the SNI hostname from the
    // client does not match cert_domain (the primary certificate's domain).
    // This allows serving a self-signed cert with the local hostname in its SAN
    // for LAN clients while still serving the Let's Encrypt cert to external clients.
    void set_fallback_cert(const unsigned char *cert_buf, int cert_len,
                           const unsigned char *key_buf,
                           const char *domain)
    {
        fallback_cert_der     = cert_buf;
        fallback_cert_der_len = cert_len;
        fallback_privkey      = key_buf;
        cert_domain           = domain;
    }

    // Perform the TLS 1.2 server handshake.
    // Returns nullptr on success, or an error string on failure.
    const char *handshake()
    {
        try
        {
            // ---- Receive ClientHello ----------------------------------------
            m_step = "ClientHello";
            TLS_SERVER_DEBUG("[SSL] server: waiting for ClientHello\r\n");
            const char *ret = recv_records(10);
            if(ret)
            {
                // If Chrome (or any browser) hit the HTTPS port with plain HTTP,
                // send an HTTP 301 redirect to HTTPS before giving up.
                if(strcmp(ret, "not a TLS record") == 0)
                    try_send_http_redirect();
                throw ret;
            }

            int rtype, rver;
            tlsbuf payload;
            ret = consume_record(rtype, rver, payload);
            if(ret) throw ret;
            if(rtype != CONTENT_HANDSHAKE)
                throw "expected handshake record";

            unsigned char client_rand[RAND_SIZE];
            unsigned char session_id[32];
            int           session_id_len = 0;
            TLS_CIPHER    chosen_cipher  = TLS_NONE;

            ret = parse_client_hello(payload.buf, payload.size,
                                     client_rand, chosen_cipher,
                                     session_id, session_id_len);
            if(ret) throw ret;

            // If a fallback cert is configured, select the right cert based on SNI.
            // When the client connects with a hostname that differs from the primary
            // cert's domain (e.g. a LAN hostname instead of the ACME/LE domain),
            // switch to the fallback cert so the hostname in the cert SAN matches.
            if(fallback_cert_der && fallback_privkey && cert_domain)
            {
                bool sni_matches_primary = (sni_name[0] != '\0' &&
                                            strcmp(sni_name, cert_domain) == 0);
                if(!sni_matches_primary)
                {
                    // Use the fallback cert (self-signed with local hostname SAN).
                    // Clear the chain: the fallback is self-signed so there are no
                    // intermediates to send.
                    cert_der     = fallback_cert_der;
                    cert_der_len = fallback_cert_der_len;
                    privkey      = fallback_privkey;
                    chain_der     = nullptr;
                    chain_der_len = 0;
                    TLS_SERVER_DEBUG("[SSL] server: SNI='%s' != domain='%s', "
                                     "using fallback self-signed cert\r\n",
                                     sni_name, cert_domain);
                }
                else
                {
                    TLS_SERVER_DEBUG("[SSL] server: SNI='%s' matches primary cert domain, "
                                     "using LE/ACME cert\r\n", sni_name);
                }
            }

            // If the client sent supported_versions but did NOT include TLS 1.2,
            // we cannot serve this client.  Fail fast rather than sending a TLS 1.2
            // ServerHello that the client will reject (which would result in a
            // mid-handshake WSAECONNABORTED on our side).
            if(client_tls12_not_offered)
                throw "TLS 1.2 not offered in client supported_versions";

            // Update transcript hash with ClientHello.
            crypto.update_hash(payload.buf, payload.size);

            // ---- Prepare cipher and randoms --------------------------------
            // Generate a cryptographically random server_rand using CryptGenRandom.
            // Using rand() is not appropriate here: it is not cryptographically
            // secure and its output is predictable, which could weaken the handshake.
            unsigned char server_rand[RAND_SIZE];
            {
                HCRYPTPROV hprov = 0;
                bool ok = false;
                if(CryptAcquireContext(&hprov, NULL, NULL, PROV_RSA_FULL,
                                       CRYPT_VERIFYCONTEXT))
                {
                    ok = (CryptGenRandom(hprov, RAND_SIZE, server_rand) != FALSE);
                    CryptReleaseContext(hprov, 0);
                }
                if(!ok)
                    throw "CryptGenRandom failed for server random";
            }

            // update_server_info sets the cipher index, creates the encoder,
            // and stores server_rand in crypto.data12.server_rand.
            ret = crypto.update_server_info((int)chosen_cipher, server_rand, false);
            if(ret) throw ret;

            // Store the client random so key derivation uses it.
            crypto.set_client_rand(client_rand);

            // ---- Send ServerHello ------------------------------------------
            m_step = "ServerHello";
            TLS_SERVER_DEBUG("[SSL] server: sending ServerHello\r\n");
            // Do not echo the client's session_id back.  This server does not
            // implement TLS 1.2 session resumption; echoing the client's
            // session_id would signal resumption to the client, which would then
            // expect an abbreviated handshake and reject the full handshake we send.
            // Sending an empty session_id correctly tells the client to start a
            // new full-handshake session.
            ret = send_server_hello(chosen_cipher, server_rand, nullptr, 0);
            if(ret) throw ret;

            // ---- Send Certificate ------------------------------------------
            m_step = "Certificate";
            TLS_SERVER_DEBUG("[SSL] server: sending Certificate\r\n");
            ret = send_certificate();
            if(ret) throw ret;

            // ---- Send ServerKeyExchange ------------------------------------
            m_step = "ServerKeyExchange";
            TLS_SERVER_DEBUG("[SSL] server: sending ServerKeyExchange\r\n");
            unsigned char server_pubkey[65];
            int           server_pubkey_len = 0;
            ret = send_server_key_exchange(client_rand, server_rand,
                                           server_pubkey, &server_pubkey_len);
            if(ret) throw ret;

            // ---- Send ServerHelloDone --------------------------------------
            m_step = "ServerHelloDone";
            TLS_SERVER_DEBUG("[SSL] server: sending ServerHelloDone\r\n");
            ret = send_server_hello_done();
            if(ret) throw ret;

            // ---- Receive ClientKeyExchange, ChangeCipherSpec, Finished -----
            m_step = "ClientKeyExchange";
            TLS_SERVER_DEBUG("[SSL] server: waiting for ClientKeyExchange/CCS/Finished\r\n");
            unsigned char client_pubkey[65];
            int           client_pubkey_len = 0;
            bool got_cke = false, got_ccs = false, got_fin = false;

            while(!(got_cke && got_ccs && got_fin))
            {
                ret = recv_records(10);
                if(ret) throw ret;

                ret = consume_record(rtype, rver, payload);
                if(ret) throw ret;

                if(rtype == CONTENT_HANDSHAKE)
                {
                    if(payload.size < 1) throw "empty handshake record";
                    int hs_t = (unsigned char)payload.buf[0];

                    if(!got_cke && hs_t == MSG_CLIENT_KEY_EXCHANGE)
                    {
                        TLS_SERVER_DEBUG("[SSL] server: received ClientKeyExchange\r\n");
                        ret = parse_client_key_exchange(payload.buf, payload.size,
                                                        client_pubkey,
                                                        client_pubkey_len);
                        if(ret) throw ret;

                        // Update transcript hash.
                        crypto.update_hash(payload.buf, payload.size);
                        got_cke = true;

                        // Derive session keys (server perspective: swap client/server keys).
                        m_step = "key derivation";
                        ret = crypto.tls12_compute_key_server(
                            ECC_secp256r1,
                            reinterpret_cast<const char*>(client_pubkey),
                            client_pubkey_len,
                            use_ems);
                        if(ret) throw ret;
                        m_step = "ClientKeyExchange";
                    }
                    else if(got_ccs && hs_t == MSG_FINISHED)
                    {
                        TLS_SERVER_DEBUG("[SSL] server: received client Finished\r\n");
                        m_step = "Finished (client)";
                        // Finished is sent encrypted; consume_record already decrypted it.
                        ret = verify_client_finished(payload.buf, payload.size);
                        if(ret) throw ret;

                        // Update transcript hash with client Finished.
                        crypto.update_hash(payload.buf, payload.size);
                        got_fin = true;
                    }
                }
                else if(rtype == CONTENT_CHANGECIPHERSPEC)
                {
                    TLS_SERVER_DEBUG("[SSL] server: received ChangeCipherSpec\r\n");
                    // Enable encryption/decryption for subsequent records.
                    // Sequence numbers are already 0 from key derivation; no reset needed.
                    crypto.set_encoding(true);
                    got_ccs = true;
                    m_step = "Finished (client)";
                }
                else if(rtype == CONTENT_ALERT)
                {
                    // A fatal alert from the client (e.g. handshake_failure) means
                    // the client rejected our ServerHello.  Abort immediately instead
                    // of timing out waiting for a ClientKeyExchange that will never arrive.
                    if(payload.size >= 2 && (unsigned char)payload.buf[0] == 2 /* fatal */)
                    {
                        TLS_SERVER_DEBUG("[SSL] server: fatal alert from client"
                                         " (code=%d)\r\n",
                                         payload.size >= 2 ? (int)(unsigned char)payload.buf[1] : -1);
                        throw "fatal alert from client";
                    }
                    // Warning alerts are non-fatal; ignore and continue.
                }
            }

            // ---- Send ChangeCipherSpec + Finished --------------------------
            m_step = "ChangeCipherSpec (server)";
            TLS_SERVER_DEBUG("[SSL] server: sending ChangeCipherSpec + Finished\r\n");
            ret = send_change_cipherspec();
            if(ret) throw ret;

            m_step = "Finished (server)";
            ret = send_server_finish();
            if(ret) throw ret;

            handshake_done = true;
            m_step = "";
            return nullptr;
        }
        catch(const char *err)
        {
            close();
            set_err(err, -1);
            return err_msg.buf;
        }
    }

    // Send application data (after handshake).
    int send(const char *buf, int size)
    {
        if(!handshake_done) return 0;
        send_buf.clear();
        for(int i = 0; i < size; )
        {
            int chunk = min(size - i, 60000);
            send_buf.set_size(chunk);
            memcpy(send_buf.buf, buf + i, chunk);
            const char *ret = send_packet(CONTENT_APPLICATION_DATA, 0x0303, send_buf);
            if(ret) return set_err(ret, 0);
            i += chunk;
        }
        return size;
    }

    // Receive application data (after handshake).  Blocks until data arrives or
    // the connection closes.  Returns 0 on graceful close, -1 on timeout, or the
    // number of bytes written into 'out'.
    int recv(char *out, int size)
    {
        if(!handshake_done) return set_err("handshake not complete", 0);

        if(received_close_notify && recv_channel.size <= recv_channel_readed)
        {
            close();
            return 0;
        }

        DWORD start = GetTickCount();
        try
        {
            while(true)
            {
                if(received_close_notify) break;

                fd_set set;
                FD_ZERO(&set);
                FD_SET(s, &set);
                timeval tv = { recv_channel.size > recv_channel_readed ? 0 : 1, 0 };
                int sig = select((int)(s + 1), &set, nullptr, nullptr, &tv);

                if(sig == -1) return set_err("select error", 0);
                if(sig == 0 && recv_channel.size > recv_channel_readed) break;
                if(sig == 0)
                {
                    if(GetTickCount() - start > (DWORD)time_out) return -1;
                    continue;
                }
                process_recv();
            }
        }
        catch(const char *err)
        {
            close();
            return set_err(err, 0);
        }
        return read_channel(out, size);
    }

    void close()
    {
        handshake_done           = false;
        received_close_notify    = false;
        use_ems                  = false;
        client_tls12_not_offered = false;
        recv_buf.clear();
        recv_channel.clear();
        recv_channel_readed = 0;
        crypto.reset();
        time_out = 0x7fffffff;
        m_step   = "";
        if(s != INVALID_SOCKET)
        {
            shutdown(s, SD_BOTH);
            closesocket(s);
        }
        s = INVALID_SOCKET;
    }

    void set_timeout(int ms) { time_out = ms; }

    const char *errmsg() { return err_msg.buf; }

    bool is_connected() const { return handshake_done; }

    // Release ownership of the socket so that close()/destructor will not call
    // closesocket().  Call this before deleting the object when you want to
    // manage the socket lifetime externally.
    void detach_socket() { s = INVALID_SOCKET; }
};
