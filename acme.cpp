/*******************************************************************
 * acme.cpp
 * DESCRIPTION: ACME v2 (RFC 8555) client – Let's Encrypt integration.
 *
 * Crypto : BCrypt (Vista+) – P-256 ECDSA, SHA-256
 * HTTP   : WinHTTP – HTTPS requests to Let's Encrypt API
 * Challenge: temporary TCP listener on the challenge port (default 80)
 *******************************************************************/

#include "rmtsvc.h"
#include "acme.h"
#include "net4cpp21/include/cCoder.h"

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>

// ---------------------------------------------------------------------------
// Global ACME configuration (populated by parseCommand)
// ---------------------------------------------------------------------------
std::string g_acme_domain;
std::string g_acme_email;
bool        g_acme_staging        = false;
int         g_acme_challenge_port = 80;

// ===========================================================================
// Base64url helpers  (RFC 4648 §5, no padding)
// ===========================================================================
static const char B64URL[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static std::string b64url_enc(const unsigned char *p, size_t n)
{
    std::string out;
    out.reserve((n * 4 + 2) / 3);
    for (size_t i = 0; i < n; i += 3) {
        unsigned int b = (unsigned int)p[i] << 16;
        if (i + 1 < n) b |= (unsigned int)p[i+1] << 8;
        if (i + 2 < n) b |= (unsigned int)p[i+2];
        out += B64URL[(b >> 18) & 0x3F];
        out += B64URL[(b >> 12) & 0x3F];
        if (i + 1 < n) out += B64URL[(b >>  6) & 0x3F];
        if (i + 2 < n) out += B64URL[ b        & 0x3F];
    }
    return out;
}

static std::string b64url_enc(const std::string &s)
{
    return b64url_enc((const unsigned char *)s.data(), s.size());
}

// ===========================================================================
// Minimal JSON string extractor  (no full parser; handles ACME responses)
// ===========================================================================

// Extract "key": "value" → value
static std::string jstr(const std::string &j, const char *key)
{
    std::string needle = std::string("\"") + key + "\"";
    size_t p = j.find(needle);
    if (p == std::string::npos) return "";
    p = j.find(':', p + needle.size());
    if (p == std::string::npos) return "";
    p = j.find('"', p + 1);
    if (p == std::string::npos) return "";
    size_t e = j.find('"', p + 1);
    if (e == std::string::npos) return "";
    return j.substr(p + 1, e - p - 1);
}

// Extract first string element of "key": ["val1", ...]
static std::string jfirst(const std::string &j, const char *key)
{
    std::string needle = std::string("\"") + key + "\"";
    size_t p = j.find(needle);
    if (p == std::string::npos) return "";
    p = j.find('[', p + needle.size());
    if (p == std::string::npos) return "";
    p = j.find('"', p + 1);
    if (p == std::string::npos) return "";
    size_t e = j.find('"', p + 1);
    if (e == std::string::npos) return "";
    return j.substr(p + 1, e - p - 1);
}

// Find the http-01 challenge object in "challenges":[...] and return token+url
static bool jhttp01(const std::string &j, std::string &token, std::string &url)
{
    size_t arrPos = j.find("\"challenges\"");
    if (arrPos == std::string::npos) return false;
    size_t arrStart = j.find('[', arrPos);
    if (arrStart == std::string::npos) return false;
    size_t arrEnd = j.find(']', arrStart);

    size_t objStart = arrStart;
    while (true) {
        objStart = j.find('{', objStart + 1);
        if (objStart == std::string::npos || objStart >= arrEnd) break;
        int depth = 1;
        size_t p = objStart + 1;
        while (p < j.size() && depth > 0) {
            if      (j[p] == '{') ++depth;
            else if (j[p] == '}') --depth;
            ++p;
        }
        std::string obj = j.substr(objStart, p - objStart);
        if (obj.find("\"http-01\"") != std::string::npos) {
            token = jstr(obj, "token");
            url   = jstr(obj, "url");
            return !token.empty() && !url.empty();
        }
    }
    return false;
}

// ===========================================================================
// DER encoding helpers
// ===========================================================================
static void der_len(std::vector<unsigned char> &b, size_t n)
{
    if (n < 128) {
        b.push_back((unsigned char)n);
    } else if (n < 256) {
        b.push_back(0x81); b.push_back((unsigned char)n);
    } else {
        b.push_back(0x82);
        b.push_back((unsigned char)(n >> 8));
        b.push_back((unsigned char)(n & 0xFF));
    }
}

static void der_tlv(std::vector<unsigned char> &b, unsigned char tag,
                    const unsigned char *data, size_t n)
{
    b.push_back(tag);
    der_len(b, n);
    if (data && n) b.insert(b.end(), data, data + n);
}

static void der_tlv(std::vector<unsigned char> &b, unsigned char tag,
                    const std::vector<unsigned char> &inner)
{
    der_tlv(b, tag, inner.data(), inner.size());
}

// Wrap inner bytes in a DER SEQUENCE (0x30)
static std::vector<unsigned char> der_seq(const std::vector<unsigned char> &inner)
{
    std::vector<unsigned char> out;
    der_tlv(out, 0x30, inner);
    return out;
}

// DER INTEGER from raw big-endian bytes (prepend 0x00 if high bit set)
static std::vector<unsigned char> der_int(const unsigned char *bytes, size_t n)
{
    while (n > 1 && bytes[0] == 0) { ++bytes; --n; } // strip leading zeros
    std::vector<unsigned char> val;
    if (bytes[0] & 0x80) val.push_back(0x00);
    val.insert(val.end(), bytes, bytes + n);
    std::vector<unsigned char> out;
    der_tlv(out, 0x02, val);
    return out;
}

// DER OID
static std::vector<unsigned char> der_oid(const unsigned char *bytes, size_t n)
{
    std::vector<unsigned char> out;
    der_tlv(out, 0x06, bytes, n);
    return out;
}

// Convert raw 64-byte ECDSA r||s to DER SEQUENCE { INTEGER r, INTEGER s }
static std::vector<unsigned char> ecdsa_raw_to_der(const unsigned char sig64[64])
{
    auto r = der_int(sig64,      32);
    auto s = der_int(sig64 + 32, 32);
    std::vector<unsigned char> inner;
    inner.insert(inner.end(), r.begin(), r.end());
    inner.insert(inner.end(), s.begin(), s.end());
    return der_seq(inner);
}

// OID constants
static const unsigned char OID_EC_PUBKEY[]    = {0x2a,0x86,0x48,0xce,0x3d,0x02,0x01};
static const unsigned char OID_P256[]         = {0x2a,0x86,0x48,0xce,0x3d,0x03,0x01,0x07};
static const unsigned char OID_ECDSA_SHA256[] = {0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x02};
static const unsigned char OID_CN[]           = {0x55,0x04,0x03};

// PEM → DER (first block; uses net4cpp21 base64)
static bool pem_to_der(const std::string &pem, std::vector<unsigned char> &der)
{
    size_t s = pem.find("-----BEGIN");
    if (s == std::string::npos) return false;
    s = pem.find('\n', s);
    if (s == std::string::npos) return false;
    ++s;
    size_t e = pem.find("-----END", s);
    if (e == std::string::npos) return false;

    std::string b64;
    for (size_t i = s; i < e; ++i) {
        char c = pem[i];
        if (c != '\r' && c != '\n' && c != ' ' && c != '\t') b64 += c;
    }

    int dlen = net4cpp21::cCoder::Base64DecodeSize((int)b64.size());
    if (dlen <= 0) return false;
    der.resize((size_t)dlen);
    int actual = net4cpp21::cCoder::base64_decode(
        (char *)b64.data(), (unsigned int)b64.size(), (char *)der.data());
    if (actual <= 0) return false;
    der.resize((size_t)actual);
    return true;
}

// ===========================================================================
// AcmeClient  –  constructor / destructor / configure
// ===========================================================================
AcmeClient::AcmeClient()
    : m_staging(false), m_challengePort(80),
      m_hEcAlg(NULL), m_hAcctKey(NULL),
      m_hSession(NULL)
{
    memset(m_acctX, 0, 32);
    memset(m_acctY, 0, 32);
    memset(m_acctD, 0, 32);
}

AcmeClient::~AcmeClient()
{
    if (m_hAcctKey) { BCryptDestroyKey(m_hAcctKey);           m_hAcctKey = NULL; }
    if (m_hEcAlg)   { BCryptCloseAlgorithmProvider(m_hEcAlg, 0); m_hEcAlg = NULL; }
    if (m_hSession) { WinHttpCloseHandle(m_hSession);          m_hSession = NULL; }
}

void AcmeClient::configure(const char *domain, const char *email,
                            bool staging, int challengePort)
{
    m_domain        = domain        ? domain        : "";
    m_email         = email         ? email         : "";
    m_staging       = staging;
    m_challengePort = challengePort;

    m_acctKeyFile = "rmtsvc_acme_account.key";
    m_certFile    = "rmtsvc_acme.crt";
    m_keyFile     = "rmtsvc_acme.key";
    getAbsoluteFilePath(m_acctKeyFile);
    getAbsoluteFilePath(m_certFile);
    getAbsoluteFilePath(m_keyFile);
}

// ===========================================================================
// hasCert  –  check stored certificate validity
// ===========================================================================
bool AcmeClient::hasCert(int minDays) const
{
    if (m_certFile.empty()) return false;

    FILE *f = fopen(m_certFile.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return false; }
    std::vector<char> buf((size_t)(sz + 1), 0);
    fread(buf.data(), 1, (size_t)sz, f);
    fclose(f);

    std::vector<unsigned char> der;
    if (!pem_to_der(std::string(buf.data()), der)) return false;

    PCCERT_CONTEXT ctx = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        der.data(), (DWORD)der.size());
    if (!ctx) return false;

    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);
    FILETIME ftExp = ctx->pCertInfo->NotAfter;
    CertFreeCertificateContext(ctx);

    ULARGE_INTEGER uNow, uExp;
    uNow.LowPart  = ftNow.dwLowDateTime;  uNow.HighPart  = ftNow.dwHighDateTime;
    uExp.LowPart  = ftExp.dwLowDateTime;  uExp.HighPart  = ftExp.dwHighDateTime;

    // Each day = 864 000 000 000 × 100-ns intervals
    ULONGLONG minNs = (ULONGLONG)minDays * 864000000000ULL;
    return uExp.QuadPart > uNow.QuadPart &&
           (uExp.QuadPart - uNow.QuadPart) > minNs;
}

// ===========================================================================
// SHA-256  (BCrypt)
// ===========================================================================
bool AcmeClient::sha256(const void *data, size_t len, unsigned char hash[32]) const
{
    BCRYPT_ALG_HANDLE  hAlg  = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    bool ok = false;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg,
            BCRYPT_SHA256_ALGORITHM, NULL, 0)))
    {
        if (BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0)))
        {
            if (BCRYPT_SUCCESS(BCryptHashData(hHash, (PUCHAR)data, (ULONG)len, 0)) &&
                BCRYPT_SUCCESS(BCryptFinishHash(hHash, hash, 32, 0)))
                ok = true;
            BCryptDestroyHash(hHash);
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return ok;
}

// ===========================================================================
// ECDSA sign  (BCrypt)
// ===========================================================================
bool AcmeClient::signEcdsa(BCRYPT_KEY_HANDLE hKey,
                            const unsigned char hash32[32],
                            unsigned char sig64[64]) const
{
    ULONG cbSig = 0;
    if (!BCRYPT_SUCCESS(BCryptSignHash(hKey, NULL,
            (PUCHAR)hash32, 32, NULL, 0, &cbSig, 0)))
        return false;
    std::vector<unsigned char> buf(cbSig);
    if (!BCRYPT_SUCCESS(BCryptSignHash(hKey, NULL,
            (PUCHAR)hash32, 32, buf.data(), cbSig, &cbSig, 0)))
        return false;
    // BCrypt ECDSA P-256 produces raw r||s (IEEE P1363), 64 bytes
    if (cbSig != 64) return false;
    memcpy(sig64, buf.data(), 64);
    return true;
}

// ===========================================================================
// Account key  –  load or generate P-256 key pair
// ===========================================================================
bool AcmeClient::loadOrGenAcctKey()
{
    return loadAcctKey() || genAcctKey();
}

bool AcmeClient::loadAcctKey()
{
    FILE *f = fopen(m_acctKeyFile.c_str(), "rb");
    if (!f) return false;
    unsigned char buf[96];
    size_t n = fread(buf, 1, 96, f);
    fclose(f);
    if (n != 96) return false;

    memcpy(m_acctX, buf,      32);
    memcpy(m_acctY, buf + 32, 32);
    memcpy(m_acctD, buf + 64, 32);

    if (!m_hEcAlg)
        if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&m_hEcAlg,
                BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0)))
            return false;

    // Build BCRYPT_ECCPRIVATE_BLOB: header(8) + X(32) + Y(32) + d(32) = 104 bytes
    struct { BCRYPT_ECCKEY_BLOB hdr; unsigned char x[32], y[32], d[32]; } blob;
    blob.hdr.dwMagic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
    blob.hdr.cbKey   = 32;
    memcpy(blob.x, m_acctX, 32);
    memcpy(blob.y, m_acctY, 32);
    memcpy(blob.d, m_acctD, 32);

    if (m_hAcctKey) { BCryptDestroyKey(m_hAcctKey); m_hAcctKey = NULL; }
    return BCRYPT_SUCCESS(BCryptImportKeyPair(m_hEcAlg, NULL,
        BCRYPT_ECCPRIVATE_BLOB, &m_hAcctKey,
        (PUCHAR)&blob, (ULONG)sizeof(blob), 0));
}

bool AcmeClient::genAcctKey()
{
    if (!m_hEcAlg)
        if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&m_hEcAlg,
                BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0)))
            return false;

    BCRYPT_KEY_HANDLE hKey = NULL;
    if (!BCRYPT_SUCCESS(BCryptGenerateKeyPair(m_hEcAlg, &hKey, 256, 0)) ||
        !BCRYPT_SUCCESS(BCryptFinalizeKeyPair(hKey, 0)))
    {
        if (hKey) BCryptDestroyKey(hKey);
        return false;
    }

    struct { BCRYPT_ECCKEY_BLOB hdr; unsigned char x[32], y[32], d[32]; } blob;
    ULONG cb = 0;
    if (!BCRYPT_SUCCESS(BCryptExportKey(hKey, NULL, BCRYPT_ECCPRIVATE_BLOB,
            (PUCHAR)&blob, (ULONG)sizeof(blob), &cb, 0)))
    {
        BCryptDestroyKey(hKey); return false;
    }

    memcpy(m_acctX, blob.x, 32);
    memcpy(m_acctY, blob.y, 32);
    memcpy(m_acctD, blob.d, 32);
    if (m_hAcctKey) BCryptDestroyKey(m_hAcctKey);
    m_hAcctKey = hKey;

    unsigned char save[96];
    memcpy(save,      m_acctX, 32);
    memcpy(save + 32, m_acctY, 32);
    memcpy(save + 64, m_acctD, 32);
    FILE *f = fopen(m_acctKeyFile.c_str(), "wb");
    if (f) { fwrite(save, 1, 96, f); fclose(f); }
    return true;
}

// ===========================================================================
// JWK / JWS
// ===========================================================================
std::string AcmeClient::jwkJson() const
{
    // Canonical JWK sorted by key (crv, kty, x, y) – required for thumbprint
    return "{\"crv\":\"P-256\",\"kty\":\"EC\","
           "\"x\":\"" + b64url_enc(m_acctX, 32) + "\","
           "\"y\":\"" + b64url_enc(m_acctY, 32) + "\"}";
}

std::string AcmeClient::jwkThumbprint() const
{
    std::string jwk = jwkJson();
    unsigned char hash[32];
    sha256(jwk.data(), jwk.size(), hash);
    return b64url_enc(hash, 32);
}

std::string AcmeClient::makeJws(const std::string &url,
                                 const std::string &nonce,
                                 const std::string &payload)
{
    // Build protected header
    std::string hdr;
    if (m_acctUrl.empty())
        hdr = "{\"alg\":\"ES256\",\"jwk\":" + jwkJson() +
              ",\"nonce\":\"" + nonce + "\",\"url\":\"" + url + "\"}";
    else
        hdr = "{\"alg\":\"ES256\",\"kid\":\"" + m_acctUrl +
              "\",\"nonce\":\"" + nonce + "\",\"url\":\"" + url + "\"}";

    std::string hdr_b64  = b64url_enc(hdr);
    // RFC 8555: POST-as-GET uses payload="" (empty string in the JWS JSON);
    // signing input is  hdr_b64 + "."  (no payload component).
    std::string pay_b64  = payload.empty() ? "" : b64url_enc(payload);
    std::string sigInput = hdr_b64 + "." + pay_b64;

    unsigned char hash[32], sig64[64];
    if (!sha256(sigInput.data(), sigInput.size(), hash)) return "";
    if (!m_hAcctKey || !signEcdsa(m_hAcctKey, hash, sig64))   return "";

    return "{\"protected\":\"" + hdr_b64 +
           "\",\"payload\":\""  + pay_b64 +
           "\",\"signature\":\"" + b64url_enc(sig64, 64) + "\"}";
}

// ===========================================================================
// HTTP helpers  (WinHTTP)
// ===========================================================================
bool AcmeClient::parseUrl(const std::string &url, ParsedUrl &out)
{
    int wn = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, NULL, 0);
    if (wn <= 0) return false;
    std::wstring wurl((size_t)wn, 0);
    MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wurl[0], wn);

    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {}, path[2048] = {}, extra[1024] = {};
    uc.lpszHostName   = host;  uc.dwHostNameLength   = 256;
    uc.lpszUrlPath    = path;  uc.dwUrlPathLength    = 2048;
    uc.lpszExtraInfo  = extra; uc.dwExtraInfoLength  = 1024;

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return false;

    out.https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    out.host  = host;
    out.port  = uc.nPort;
    out.path  = path;
    if (extra[0]) out.path += extra;
    return true;
}

bool AcmeClient::httpRequest(const std::string &method, const std::string &url,
                              const std::string &body,   const std::string &ctype,
                              int &status,
                              std::map<std::string,std::string> &hdrs,
                              std::string &bodyOut)
{
    status = 0; hdrs.clear(); bodyOut.clear();

    ParsedUrl pu;
    if (!parseUrl(url, pu)) return false;

    if (!m_hSession) {
        m_hSession = WinHttpOpen(L"rmtsvc/1.0",
                                  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME,
                                  WINHTTP_NO_PROXY_BYPASS, 0);
        if (!m_hSession) return false;
        WinHttpSetTimeouts(m_hSession, 30000, 30000, 30000, 30000);
    }

    HINTERNET hConn = WinHttpConnect(m_hSession, pu.host.c_str(), pu.port, 0);
    if (!hConn) return false;

    int wm = MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, NULL, 0);
    std::wstring wmethod((size_t)wm, 0);
    MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, &wmethod[0], wm);

    DWORD flags = pu.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, wmethod.c_str(),
                                         pu.path.c_str(), NULL,
                                         WINHTTP_NO_REFERER,
                                         WINHTTP_DEFAULT_ACCEPT_TYPES,
                                         flags);
    if (!hReq) { WinHttpCloseHandle(hConn); return false; }

    bool ok = false;
    do {
        if (!ctype.empty()) {
            int wc = MultiByteToWideChar(CP_UTF8, 0, ctype.c_str(), -1, NULL, 0);
            std::wstring wct((size_t)wc, 0);
            MultiByteToWideChar(CP_UTF8, 0, ctype.c_str(), -1, &wct[0], wc);
            std::wstring ctHdr = L"Content-Type: " + wct;
            WinHttpAddRequestHeaders(hReq, ctHdr.c_str(), (DWORD)-1,
                                      WINHTTP_ADDREQ_FLAG_ADD);
        }

        LPVOID pBody  = body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str();
        DWORD  cbBody = (DWORD)body.size();
        if (!WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 pBody, cbBody, cbBody, 0)) break;
        if (!WinHttpReceiveResponse(hReq, NULL)) break;

        DWORD dwSt = 0, cbSt = sizeof(dwSt);
        WinHttpQueryHeaders(hReq,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &dwSt, &cbSt,
            WINHTTP_NO_HEADER_INDEX);
        status = (int)dwSt;

        // Collect specific response headers
        static const wchar_t *kHdrs[] = {
            L"Replay-Nonce", L"Location", L"Content-Type", NULL
        };
        for (int i = 0; kHdrs[i]; ++i) {
            DWORD hlen = 0;
            WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CUSTOM, kHdrs[i],
                                 WINHTTP_NO_OUTPUT_BUFFER, &hlen,
                                 WINHTTP_NO_HEADER_INDEX);
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                std::wstring wv((size_t)(hlen / sizeof(wchar_t) + 1), 0);
                WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CUSTOM, kHdrs[i],
                                     &wv[0], &hlen, WINHTTP_NO_HEADER_INDEX);
                // narrow conversion
                int kn = WideCharToMultiByte(CP_UTF8, 0, kHdrs[i], -1, NULL, 0, NULL, NULL);
                int vn = WideCharToMultiByte(CP_UTF8, 0, wv.c_str(), -1, NULL, 0, NULL, NULL);
                std::string k((size_t)kn, 0), v((size_t)vn, 0);
                WideCharToMultiByte(CP_UTF8, 0, kHdrs[i],    -1, &k[0], kn, NULL, NULL);
                WideCharToMultiByte(CP_UTF8, 0, wv.c_str(),  -1, &v[0], vn, NULL, NULL);
                // strip embedded NUL from WideCharToMultiByte
                if (!k.empty() && k.back() == '\0') k.pop_back();
                if (!v.empty() && v.back() == '\0') v.pop_back();
                hdrs[k] = v;
            }
        }

        // Read body
        char buf[4096];
        DWORD dwRead = 0;
        while (WinHttpReadData(hReq, buf, sizeof(buf), &dwRead) && dwRead > 0)
            bodyOut.append(buf, dwRead);

        ok = true;
    } while (false);

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    return ok;
}

bool AcmeClient::httpPost(const std::string &url, const std::string &body,
                           int &status,
                           std::map<std::string,std::string> &hdrs,
                           std::string &resp)
{
    return httpRequest("POST", url, body, "application/jose+json",
                        status, hdrs, resp);
}

bool AcmeClient::httpGet(const std::string &url, int &status, std::string &resp)
{
    std::map<std::string,std::string> h;
    return httpRequest("GET", url, "", "", status, h, resp);
}

bool AcmeClient::httpHead(const std::string &url,
                           int &status,
                           std::map<std::string,std::string> &hdrs)
{
    std::string resp;
    return httpRequest("HEAD", url, "", "", status, hdrs, resp);
}

// ===========================================================================
// ACME directory + nonce
// ===========================================================================
bool AcmeClient::fetchDirectory()
{
    const char *dir = m_staging
        ? "https://acme-staging-v02.api.letsencrypt.org/directory"
        : "https://acme-v02.api.letsencrypt.org/directory";

    int st; std::string resp;
    if (!httpGet(dir, st, resp)) {
        RW_LOG_PRINT(LOGLEVEL_ERROR,
            "[ACME] Failed to connect to Let's Encrypt API (%s). "
            "Check internet access and firewall.\r\n", dir);
        return false;
    }
    if (st != 200) {
        RW_LOG_PRINT(LOGLEVEL_ERROR,
            "[ACME] Directory fetch returned HTTP %d.\r\n", st);
        return false;
    }

    m_urlNewNonce = jstr(resp, "newNonce");
    m_urlNewAcct  = jstr(resp, "newAccount");
    m_urlNewOrder = jstr(resp, "newOrder");

    if (m_urlNewNonce.empty() || m_urlNewAcct.empty() || m_urlNewOrder.empty()) {
        RW_LOG_PRINT(LOGLEVEL_ERROR,
            "[ACME] Directory response missing required URLs.\r\n");
        return false;
    }
    return true;
}

std::string AcmeClient::getNonce()
{
    int st;
    std::map<std::string,std::string> hdrs;
    if (!httpHead(m_urlNewNonce, st, hdrs)) return "";
    auto it = hdrs.find("Replay-Nonce");
    return (it != hdrs.end()) ? it->second : "";
}

// ===========================================================================
// Account registration
// ===========================================================================
bool AcmeClient::doNewAccount()
{
    std::string nonce = getNonce();
    if (nonce.empty()) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Failed to obtain nonce from Let's Encrypt.\r\n");
        return false;
    }

    std::string payload = "{\"termsOfServiceAgreed\":true";
    if (!m_email.empty())
        payload += ",\"contact\":[\"mailto:" + m_email + "\"]";
    payload += "}";

    std::string jws = makeJws(m_urlNewAcct, nonce, payload);
    if (jws.empty()) return false;

    int st;
    std::map<std::string,std::string> hdrs;
    std::string resp;
    if (!httpPost(m_urlNewAcct, jws, st, hdrs, resp)) return false;
    if (st != 200 && st != 201) {
        RW_LOG_PRINT(LOGLEVEL_WARN, "[ACME] new-account HTTP %d: %s\r\n",
                      st, resp.c_str());
        return false;
    }

    auto it = hdrs.find("Location");
    if (it != hdrs.end()) m_acctUrl = it->second;
    if (m_acctUrl.empty()) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Server did not return account URL.\r\n");
        return false;
    }
    return true;
}

// ===========================================================================
// New order
// ===========================================================================
bool AcmeClient::doNewOrder(std::string &orderUrl,
                             std::string &finalizeUrl,
                             std::string &authzUrl)
{
    std::string nonce = getNonce();
    if (nonce.empty()) return false;

    std::string payload = "{\"identifiers\":[{\"type\":\"dns\",\"value\":\""
                        + m_domain + "\"}]}";
    std::string jws = makeJws(m_urlNewOrder, nonce, payload);
    if (jws.empty()) return false;

    int st;
    std::map<std::string,std::string> hdrs;
    std::string resp;
    if (!httpPost(m_urlNewOrder, jws, st, hdrs, resp)) return false;
    if (st != 201) {
        RW_LOG_PRINT(LOGLEVEL_WARN, "[ACME] new-order HTTP %d: %s\r\n",
                      st, resp.c_str());
        return false;
    }

    auto it = hdrs.find("Location");
    if (it != hdrs.end()) orderUrl = it->second;

    finalizeUrl = jstr(resp, "finalize");
    authzUrl    = jfirst(resp, "authorizations");
    return !finalizeUrl.empty() && !authzUrl.empty();
}

// ===========================================================================
// Get authorization (POST-as-GET) → extract HTTP-01 challenge
// ===========================================================================
bool AcmeClient::doGetAuthz(const std::string &authzUrl,
                             std::string &challUrl,
                             std::string &token)
{
    std::string nonce = getNonce();
    if (nonce.empty()) return false;

    // POST-as-GET: empty payload
    std::string jws = makeJws(authzUrl, nonce, "");
    if (jws.empty()) return false;

    int st;
    std::map<std::string,std::string> hdrs;
    std::string resp;
    if (!httpPost(authzUrl, jws, st, hdrs, resp)) return false;
    if (st != 200) return false;

    return jhttp01(resp, token, challUrl);
}

// ===========================================================================
// Poll an ACME resource (POST-as-GET) until status matches or timeout
// ===========================================================================
std::string AcmeClient::pollForStatus(const std::string &url,
                                       const char *want,
                                       int maxSec,
                                       std::string *certUrlOut)
{
    ULONGLONG deadline = GetTickCount64() + (ULONGLONG)maxSec * 1000;
    while (GetTickCount64() < deadline) {
        Sleep(3000);
        std::string nonce = getNonce();
        if (nonce.empty()) continue;

        std::string jws = makeJws(url, nonce, "");
        if (jws.empty()) continue;

        int st;
        std::map<std::string,std::string> hdrs;
        std::string resp;
        if (!httpPost(url, jws, st, hdrs, resp)) continue;
        if (st != 200) continue;

        std::string status = jstr(resp, "status");
        if (status == "invalid") return "invalid";
        if (status == want) {
            if (certUrlOut) *certUrlOut = jstr(resp, "certificate");
            return want;
        }
    }
    return "timeout";
}

// ===========================================================================
// HTTP-01 challenge server thread
// ===========================================================================
struct ChallengeServerCtx {
    int         port;
    std::string token;
    std::string keyAuth;
    HANDLE      hReady;
    HANDLE      hStop;
    bool        bindOk;
};

static DWORD WINAPI challengeServerThread(LPVOID param)
{
    ChallengeServerCtx *ctx = (ChallengeServerCtx *)param;
    ctx->bindOk = false;

    SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) { SetEvent(ctx->hReady); return 1; }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((u_short)ctx->port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv, (sockaddr *)&addr, sizeof(addr)) != 0 ||
        listen(srv, 5) != 0)
    {
        closesocket(srv);
        SetEvent(ctx->hReady);
        return 1;
    }

    ctx->bindOk = true;
    SetEvent(ctx->hReady);

    // Non-blocking accept loop until hStop is signalled
    u_long nb = 1;
    ioctlsocket(srv, FIONBIO, &nb);

    std::string challPath = "/.well-known/acme-challenge/" + ctx->token;
    std::string okResp    = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " +
                            std::to_string(ctx->keyAuth.size()) +
                            "\r\n\r\n" + ctx->keyAuth;
    std::string notFound  = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";

    while (WaitForSingleObject(ctx->hStop, 0) == WAIT_TIMEOUT) {
        SOCKET cli = accept(srv, NULL, NULL);
        if (cli == INVALID_SOCKET) { Sleep(50); continue; }
        char buf[2048] = {};
        int  n         = recv(cli, buf, (int)sizeof(buf) - 1, 0);
        if (n > 0) {
            std::string req(buf, (size_t)n);
            bool match = req.find("GET " + challPath) != std::string::npos;
            const std::string &r = match ? okResp : notFound;
            send(cli, r.c_str(), (int)r.size(), 0);
        }
        shutdown(cli, SD_BOTH);
        closesocket(cli);
    }

    closesocket(srv);
    return 0;
}

// ===========================================================================
// Trigger challenge + poll authz until valid
// ===========================================================================
bool AcmeClient::doTriggerAndPoll(const std::string &challUrl,
                                   const std::string &authzUrl,
                                   const std::string &token)
{
    // keyAuthorization = token + "." + thumbprint
    std::string keyAuth = token + "." + jwkThumbprint();

    // Start challenge server
    ChallengeServerCtx ctx;
    ctx.port    = m_challengePort;
    ctx.token   = token;
    ctx.keyAuth = keyAuth;
    ctx.bindOk  = false;
    ctx.hReady  = CreateEvent(NULL, TRUE, FALSE, NULL);
    ctx.hStop   = CreateEvent(NULL, TRUE, FALSE, NULL);

    HANDLE hThread = CreateThread(NULL, 0, challengeServerThread, &ctx, 0, NULL);
    if (!hThread) {
        CloseHandle(ctx.hReady);
        CloseHandle(ctx.hStop);
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Failed to create challenge server thread.\r\n");
        return false;
    }

    // Wait for server to start (up to 5 s)
    WaitForSingleObject(ctx.hReady, 5000);
    CloseHandle(ctx.hReady);

    bool ok = false;
    if (!ctx.bindOk) {
        RW_LOG_PRINT(LOGLEVEL_ERROR,
            "[ACME] Could not bind challenge port %d. "
            "Ensure the port is free and the service has permission.\r\n",
            m_challengePort);
    } else {
        // Notify ACME server that we are ready
        std::string nonce = getNonce();
        if (!nonce.empty()) {
            std::string jws = makeJws(challUrl, nonce, "{}");
            int st;
            std::map<std::string,std::string> hdrs;
            std::string resp;
            httpPost(challUrl, jws, st, hdrs, resp);
            RW_LOG_PRINT(LOGLEVEL_INFO,
                "[ACME] Challenge triggered (HTTP %d), polling authorization...\r\n", st);

            // Poll authorization (up to 2 min)
            std::string result = pollForStatus(authzUrl, "valid", 120);
            ok = (result == "valid");
            if (!ok)
                RW_LOG_PRINT(LOGLEVEL_WARN,
                    "[ACME] Authorization ended with status: %s\r\n", result.c_str());
        }
    }

    SetEvent(ctx.hStop);
    WaitForSingleObject(hThread, 10000);
    CloseHandle(ctx.hStop);
    CloseHandle(hThread);
    return ok;
}

// ===========================================================================
// CSR builder  (PKCS#10 DER for P-256 domain)
// ===========================================================================
std::vector<unsigned char> AcmeClient::buildCSR(BCRYPT_KEY_HANDLE hKey,
                                                  const unsigned char pubX[32],
                                                  const unsigned char pubY[32])
{
    // SubjectPublicKeyInfo
    auto algoOid = der_oid(OID_EC_PUBKEY, sizeof(OID_EC_PUBKEY));
    auto curveOid= der_oid(OID_P256,      sizeof(OID_P256));
    std::vector<unsigned char> algoInner;
    algoInner.insert(algoInner.end(), algoOid.begin(),  algoOid.end());
    algoInner.insert(algoInner.end(), curveOid.begin(), curveOid.end());
    auto algoId = der_seq(algoInner);

    // Uncompressed EC point: 0x00 (unused-bits) 0x04 X Y
    std::vector<unsigned char> pubKeyBytes = { 0x00, 0x04 };
    pubKeyBytes.insert(pubKeyBytes.end(), pubX, pubX + 32);
    pubKeyBytes.insert(pubKeyBytes.end(), pubY, pubY + 32);
    std::vector<unsigned char> pubKeyBs;
    der_tlv(pubKeyBs, 0x03, pubKeyBytes);

    std::vector<unsigned char> spkiInner;
    spkiInner.insert(spkiInner.end(), algoId.begin(),   algoId.end());
    spkiInner.insert(spkiInner.end(), pubKeyBs.begin(), pubKeyBs.end());
    auto spki = der_seq(spkiInner);

    // Subject: SEQUENCE { SET { SEQUENCE { OID(CN), UTF8String(domain) } } }
    auto cnOid = der_oid(OID_CN, sizeof(OID_CN));
    std::vector<unsigned char> cnVal;
    der_tlv(cnVal, 0x0C,
            (const unsigned char *)m_domain.data(), m_domain.size());
    std::vector<unsigned char> attrInner;
    attrInner.insert(attrInner.end(), cnOid.begin(),  cnOid.end());
    attrInner.insert(attrInner.end(), cnVal.begin(),  cnVal.end());
    auto attrSeq = der_seq(attrInner);
    std::vector<unsigned char> rdnSet;
    der_tlv(rdnSet, 0x31, attrSeq);
    auto subject = der_seq(rdnSet);

    // Integer 0 (version)
    std::vector<unsigned char> version = { 0x02, 0x01, 0x00 };
    // [0] empty  (attributes)
    std::vector<unsigned char> attrs;
    der_tlv(attrs, 0xA0, std::vector<unsigned char>());

    // CertificationRequestInfo SEQUENCE
    std::vector<unsigned char> tbsInner;
    tbsInner.insert(tbsInner.end(), version.begin(), version.end());
    tbsInner.insert(tbsInner.end(), subject.begin(), subject.end());
    tbsInner.insert(tbsInner.end(), spki.begin(),    spki.end());
    tbsInner.insert(tbsInner.end(), attrs.begin(),   attrs.end());
    auto tbs = der_seq(tbsInner);

    // Sign TBS
    unsigned char hash[32], sig64[64];
    if (!sha256(tbs.data(), tbs.size(), hash)) return {};
    if (!signEcdsa(hKey, hash, sig64))         return {};

    // DER-encode signature and wrap in BIT STRING
    auto sigDer = ecdsa_raw_to_der(sig64);
    std::vector<unsigned char> sigBs;
    {
        std::vector<unsigned char> bsData = { 0x00 }; // unused bits
        bsData.insert(bsData.end(), sigDer.begin(), sigDer.end());
        der_tlv(sigBs, 0x03, bsData);
    }

    // AlgorithmIdentifier for ecdsa-with-SHA256 (no params)
    auto sigAlgOid = der_oid(OID_ECDSA_SHA256, sizeof(OID_ECDSA_SHA256));
    auto sigAlg    = der_seq(sigAlgOid);

    // CertificationRequest SEQUENCE
    std::vector<unsigned char> csrInner;
    csrInner.insert(csrInner.end(), tbs.begin(),    tbs.end());
    csrInner.insert(csrInner.end(), sigAlg.begin(), sigAlg.end());
    csrInner.insert(csrInner.end(), sigBs.begin(),  sigBs.end());
    return der_seq(csrInner);
}

// ===========================================================================
// Finalize order with CSR
// ===========================================================================
bool AcmeClient::doFinalize(const std::string &finalizeUrl,
                             const std::string &orderUrl,
                             BCRYPT_KEY_HANDLE hCertKey,
                             const unsigned char certX[32],
                             const unsigned char certY[32],
                             std::string &certUrl)
{
    auto csr = buildCSR(hCertKey, certX, certY);
    if (csr.empty()) return false;

    std::string csrB64 = b64url_enc(csr.data(), csr.size());
    std::string payload = "{\"csr\":\"" + csrB64 + "\"}";

    std::string nonce = getNonce();
    if (nonce.empty()) return false;
    std::string jws = makeJws(finalizeUrl, nonce, payload);
    if (jws.empty()) return false;

    int st;
    std::map<std::string,std::string> hdrs;
    std::string resp;
    if (!httpPost(finalizeUrl, jws, st, hdrs, resp)) return false;
    if (st != 200 && st != 201) {
        RW_LOG_PRINT(LOGLEVEL_WARN,
            "[ACME] finalize HTTP %d: %s\r\n", st, resp.c_str());
        return false;
    }

    // Poll order until valid (up to 2 min)
    std::string pollUrl = orderUrl.empty() ? finalizeUrl : orderUrl;
    std::string result  = pollForStatus(pollUrl, "valid", 120, &certUrl);
    if (result != "valid") {
        RW_LOG_PRINT(LOGLEVEL_WARN,
            "[ACME] Order polling ended with: %s\r\n", result.c_str());
        return false;
    }
    return !certUrl.empty();
}

// ===========================================================================
// Download and save certificate chain
// ===========================================================================
bool AcmeClient::doDownloadCert(const std::string &certUrl)
{
    std::string nonce = getNonce();
    if (nonce.empty()) return false;

    // POST-as-GET to download cert
    std::string jws = makeJws(certUrl, nonce, "");
    if (jws.empty()) return false;

    int st;
    std::map<std::string,std::string> hdrs;
    std::string resp;
    if (!httpPost(certUrl, jws, st, hdrs, resp)) return false;
    if (st != 200) {
        RW_LOG_PRINT(LOGLEVEL_WARN,
            "[ACME] cert download HTTP %d\r\n", st);
        return false;
    }

    // resp should be PEM certificate chain
    if (resp.find("-----BEGIN CERTIFICATE-----") == std::string::npos) {
        RW_LOG_PRINT(LOGLEVEL_WARN, "[ACME] Unexpected cert response format.\r\n");
        return false;
    }

    FILE *f = fopen(m_certFile.c_str(), "w");
    if (!f) return false;
    fwrite(resp.data(), 1, resp.size(), f);
    fclose(f);
    return true;
}

// ===========================================================================
// Main run() – full ACME workflow
// ===========================================================================
bool AcmeClient::run()
{
    if (m_domain.empty()) return false;

    RW_LOG_PRINT(LOGLEVEL_INFO,
        "[ACME] Starting certificate workflow for domain: %s%s\r\n",
        m_domain.c_str(), m_staging ? " (STAGING)" : "");
    RW_LOG_PRINT(LOGLEVEL_INFO,
        "[ACME] Certificate will be saved to: %s\r\n", m_certFile.c_str());
    RW_LOG_PRINT(LOGLEVEL_INFO,
        "[ACME] HTTP-01 challenge listener will open on port %d.\r\n"
        "       Ensure port %d is reachable from the internet and "
        "forwarded to this machine.\r\n",
        m_challengePort, m_challengePort);

    // 1. Load or generate account key
    if (!loadOrGenAcctKey()) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Failed to load/generate account key.\r\n");
        return false;
    }

    // 2. Fetch ACME directory
    if (!fetchDirectory()) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Failed to fetch ACME directory.\r\n");
        return false;
    }

    // 3. Register (or recover) account
    m_acctUrl.clear();
    if (!doNewAccount()) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Account registration failed.\r\n");
        return false;
    }
    RW_LOG_PRINT(LOGLEVEL_INFO, "[ACME] Account: %s\r\n", m_acctUrl.c_str());

    // 4. Place order
    std::string orderUrl, finalizeUrl, authzUrl;
    if (!doNewOrder(orderUrl, finalizeUrl, authzUrl)) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Failed to create order.\r\n");
        return false;
    }

    // 5. Get HTTP-01 challenge
    std::string challUrl, token;
    if (!doGetAuthz(authzUrl, challUrl, token)) {
        RW_LOG_PRINT(LOGLEVEL_ERROR,
            "[ACME] Failed to get HTTP-01 challenge. "
            "Ensure the domain resolves to this machine.\r\n");
        return false;
    }
    RW_LOG_PRINT(LOGLEVEL_INFO,
        "[ACME] HTTP-01 challenge token: %s (port %d)\r\n",
        token.c_str(), m_challengePort);

    // 6. Serve challenge and poll until valid
    if (!doTriggerAndPoll(challUrl, authzUrl, token)) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] HTTP-01 challenge validation failed.\r\n");
        return false;
    }
    RW_LOG_PRINT(LOGLEVEL_INFO, "[ACME] Authorization valid.\r\n");

    // 7. Generate fresh P-256 key pair for the certificate
    BCRYPT_ALG_HANDLE hEcAlg2   = NULL;
    BCRYPT_KEY_HANDLE hCertKey  = NULL;
    unsigned char certX[32] = {}, certY[32] = {};

    bool certKeyOk = false;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hEcAlg2,
            BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0)) &&
        BCRYPT_SUCCESS(BCryptGenerateKeyPair(hEcAlg2, &hCertKey, 256, 0)) &&
        BCRYPT_SUCCESS(BCryptFinalizeKeyPair(hCertKey, 0)))
    {
        struct { BCRYPT_ECCKEY_BLOB hdr; unsigned char x[32], y[32], d[32]; } blob;
        ULONG cb = 0;
        if (BCRYPT_SUCCESS(BCryptExportKey(hCertKey, NULL,
                BCRYPT_ECCPRIVATE_BLOB,
                (PUCHAR)&blob, (ULONG)sizeof(blob), &cb, 0)))
        {
            memcpy(certX, blob.x, 32);
            memcpy(certY, blob.y, 32);

            // Save raw private key scalar (compatible with setCacert)
            FILE *fk = fopen(m_keyFile.c_str(), "wb");
            if (fk) { fwrite(blob.d, 1, 32, fk); fclose(fk); certKeyOk = true; }
        }
    }

    if (!certKeyOk) {
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Failed to generate certificate key pair.\r\n");
        if (hCertKey)  BCryptDestroyKey(hCertKey);
        if (hEcAlg2)   BCryptCloseAlgorithmProvider(hEcAlg2, 0);
        return false;
    }

    // 8. Finalize order with CSR and download cert
    std::string certUrl;
    bool ok = doFinalize(finalizeUrl, orderUrl, hCertKey, certX, certY, certUrl);
    if (ok) ok = doDownloadCert(certUrl);

    BCryptDestroyKey(hCertKey);
    BCryptCloseAlgorithmProvider(hEcAlg2, 0);

    if (ok)
        RW_LOG_PRINT(LOGLEVEL_INFO,
            "[ACME] Certificate issued and saved to: %s\r\n", m_certFile.c_str());
    else
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[ACME] Certificate issuance failed.\r\n");

    return ok;
}
