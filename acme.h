/*******************************************************************
 * acme.h
 * DESCRIPTION: ACME v2 (RFC 8555) client for automatic Let's Encrypt
 *              certificate issuance and renewal.
 *
 * Uses the HTTP-01 challenge type via a temporary TCP listener on the
 * configured challenge port (default: 80).
 *
 * Crypto : Windows BCrypt API (Vista+)
 * HTTP   : WinHTTP (XP SP1+)
 *
 * INI file usage:
 *   acme domain=example.com [email=admin@example.com] \
 *        [staging=true] [challenge_port=80]
 *
 * Output files (next to the executable):
 *   rmtsvc_acme.crt          - PEM certificate chain  (passed to setCacert)
 *   rmtsvc_acme.key          - 32-byte raw P-256 private key scalar
 *   rmtsvc_acme_account.key  - 96-byte account key (X || Y || d)
 *******************************************************************/
#pragma once
#ifndef __RMTSVC_ACME_H__
#define __RMTSVC_ACME_H__

#include <string>
#include <vector>
#include <map>
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <wincrypt.h>

// ---------------------------------------------------------------------------
// Global configuration – populated by the "acme" INI directive.
// ---------------------------------------------------------------------------
extern std::string g_acme_domain;
extern std::string g_acme_email;
extern bool        g_acme_staging;
extern int         g_acme_challenge_port;

// ---------------------------------------------------------------------------
// AcmeClient
// ---------------------------------------------------------------------------
class AcmeClient
{
public:
    AcmeClient();
    ~AcmeClient();

    // Must be called before hasCert() or run().
    // Resolves absolute paths for the output files.
    void configure(const char *domain, const char *email,
                   bool staging = false, int challengePort = 80);

    // Returns true if a stored certificate exists and expires no sooner than
    // minDays from now.
    bool hasCert(int minDays = 30) const;

    // Execute the full ACME workflow:
    //   account-key → directory → account → order → authz
    //   → HTTP-01 challenge → CSR → finalize → download cert
    // On success the cert PEM is written to certFile() and the raw 32-byte
    // P-256 private key scalar is written to keyFile().
    bool run();

    // Absolute paths of the output certificate files.
    const std::string &certFile() const { return m_certFile; }
    const std::string &keyFile()  const { return m_keyFile;  }

private:
    // ---------------------------------------------------------------- config
    std::string m_domain;
    std::string m_email;
    bool        m_staging;
    int         m_challengePort;

    // Absolute file paths
    std::string m_acctKeyFile;
    std::string m_certFile;
    std::string m_keyFile;

    // --------------------------------------------------------- account key
    BCRYPT_ALG_HANDLE m_hEcAlg;
    BCRYPT_KEY_HANDLE m_hAcctKey;
    unsigned char     m_acctX[32], m_acctY[32], m_acctD[32];

    bool loadOrGenAcctKey();
    bool loadAcctKey();
    bool genAcctKey();

    // ------------------------------------------------------ ACME state
    std::string m_acctUrl;
    std::string m_urlNewNonce;
    std::string m_urlNewAcct;
    std::string m_urlNewOrder;

    // ------------------------------------------------- ACME protocol steps
    bool        fetchDirectory();
    std::string getNonce();
    bool        doNewAccount();
    bool        doNewOrder(std::string &orderUrl, std::string &finalizeUrl,
                           std::string &authzUrl);
    bool        doGetAuthz(const std::string &authzUrl,
                           std::string &challUrl, std::string &token);
    bool        doTriggerAndPoll(const std::string &challUrl,
                                 const std::string &authzUrl,
                                 const std::string &token);
    std::string pollForStatus(const std::string &url, const char *want,
                              int maxSec, std::string *certUrlOut = nullptr);
    bool        doFinalize(const std::string &finalizeUrl,
                           const std::string &orderUrl,
                           BCRYPT_KEY_HANDLE hCertKey,
                           const unsigned char certX[32],
                           const unsigned char certY[32],
                           std::string &certUrl);
    bool        doDownloadCert(const std::string &certUrl);

    // ------------------------------------------------ CSR / DER helpers
    std::vector<unsigned char> buildCSR(BCRYPT_KEY_HANDLE hKey,
                                        const unsigned char pubX[32],
                                        const unsigned char pubY[32]);

    // ------------------------------------------------------- JWK / JWS
    std::string jwkJson()       const;
    std::string jwkThumbprint() const;
    // payload="" → POST-as-GET; otherwise base64url-encoded payload
    std::string makeJws(const std::string &url, const std::string &nonce,
                        const std::string &payload);

    // -------------------------------------------------- crypto (BCrypt)
    bool sha256(const void *data, size_t len, unsigned char hash[32]) const;
    bool signEcdsa(BCRYPT_KEY_HANDLE hKey,
                   const unsigned char hash32[32],
                   unsigned char sig64[64]) const;

    // --------------------------------------------- HTTP helper (WinHTTP)
    struct ParsedUrl {
        bool          https;
        std::wstring  host;
        INTERNET_PORT port;
        std::wstring  path;
    };
    static bool parseUrl(const std::string &url, ParsedUrl &out);

    HINTERNET m_hSession;

    bool httpRequest(const std::string &method, const std::string &url,
                     const std::string &body,   const std::string &ctype,
                     int &status,
                     std::map<std::string,std::string> &respHdrs,
                     std::string &respBody);

    bool httpPost(const std::string &url, const std::string &body,
                  int &status,
                  std::map<std::string,std::string> &hdrs,
                  std::string &resp);

    bool httpGet(const std::string &url,
                 int &status, std::string &resp);

    bool httpHead(const std::string &url,
                  int &status,
                  std::map<std::string,std::string> &hdrs);
};

#endif // __RMTSVC_ACME_H__
