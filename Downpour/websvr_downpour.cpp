/*******************************************************************
 *  websvr_downpour.cpp
 *  DESCRIPTION: Downpour HTTP(S) server implementation.
 *
 *  Provides:
 *    GET  /api/list          – JSON list of all torrents + stats
 *    POST /api/add_magnet    – add torrent via magnet link
 *    POST /api/add_torrent   – add torrent by uploading a .torrent file
 *    POST /api/start         – start / resume a torrent
 *    POST /api/stop          – stop / pause a torrent
 *    POST /api/remove        – remove a torrent (optionally delete files)
 *    GET  /api/settings      – get current speed-limit settings
 *    POST /api/settings      – update speed-limit settings
 *
 *  All remote-desktop, registry, process, file-manager, FTP, proxy,
 *  telnet and vIDC endpoints from rmtsvc have been intentionally omitted.
 *******************************************************************/

#include "downpour.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

// ---------------------------------------------------------------------------
// DownpourServer constructor / Start / Stop
// ---------------------------------------------------------------------------
DownpourServer::DownpourServer()
    : m_svrport(7778)
    , m_bAnonymous(true)
    , m_bSSLenabled(false)
    , m_bSSLverify(false)
{
#ifdef _SUPPORT_OPENSSL_
    setCacert(NULL, NULL, NULL, true);
#endif
    setRoot(NULL, HTTP_ACCESS_NONE, NULL);
    m_defaultPage = "index.htm";
}

bool DownpourServer::Start()
{
    if (m_svrport == 0) return true;
    loadRememberTokens();

    RW_LOG_PRINT(LOGLEVEL_INFO, "[websvr] Starting on port %d – SSL=%s\r\n",
                 m_svrport, m_bSSLenabled ? "enabled" : "disabled");

#ifdef _SUPPORT_OPENSSL_
    if (m_bSSLenabled)
    {
        setCacert(NULL, NULL, NULL, true, NULL, NULL);
        return (Listen(m_svrport, TRUE,
                       m_bindip.empty() ? NULL : m_bindip.c_str()) > 0);
    }
#endif
    return (Listen(m_svrport, FALSE,
                   m_bindip.empty() ? NULL : m_bindip.c_str()) > 0);
}

void DownpourServer::Stop()
{
    saveRememberTokens();
    Close();
}

void DownpourServer::setRoot(const char* rpath, long lAccess,
                              const char* defaultPage)
{
    std::string spath;
    if (rpath && rpath[0])
        spath = rpath;
    else
    {
        char exeDir[MAX_PATH];
        ::GetModuleFileName(NULL, exeDir, MAX_PATH);
        char* p = strrchr(exeDir, '\\');
        if (p) *(p + 1) = 0;
        spath = exeDir;
        spath += "html\\";
    }
    setvpath("/", spath.c_str(), lAccess);
    if (defaultPage && defaultPage[0]) m_defaultPage = defaultPage;
}

// ---------------------------------------------------------------------------
// Remember-me token persistence
// ---------------------------------------------------------------------------
static std::string tokenFilePath()
{
    char buf[MAX_PATH];
    ::GetModuleFileName(NULL, buf, MAX_PATH);
    char* p = strrchr(buf, '\\');
    if (p) *(p + 1) = 0;
    strcat(buf, "downpour_tokens.dat");
    return buf;
}

void DownpourServer::saveRememberTokens()
{
    FILE* fp = fopen(tokenFilePath().c_str(), "w");
    if (!fp) return;
    time_t now = time(NULL);
    std::lock_guard<cMutex> lk(m_rememberMutex);
    for (auto& kv : m_rememberTokens)
        if (kv.second.expires > now)
            fprintf(fp, "%s %s %ld %lld\n",
                    kv.first.c_str(), kv.second.user.c_str(),
                    kv.second.lAccess, (long long)kv.second.expires);
    fclose(fp);
}

void DownpourServer::loadRememberTokens()
{
    FILE* fp = fopen(tokenFilePath().c_str(), "r");
    if (!fp) return;
    char token[64], user[256];
    long lAccess;
    long long expires;
    time_t now = time(NULL);
    std::lock_guard<cMutex> lk(m_rememberMutex);
    while (fscanf(fp, "%63s %255s %ld %lld", token, user, &lAccess, &expires) == 4)
    {
        if ((time_t)expires > now)
        {
            RememberEntry e;
            e.user = user; e.lAccess = lAccess; e.expires = (time_t)expires;
            m_rememberTokens[token] = e;
        }
    }
    fclose(fp);
}

// ---------------------------------------------------------------------------
// docmd_webs – parse "webs port=N [bindip=X] [root=X] [ssl_enabled=true]"
// ---------------------------------------------------------------------------
void DownpourServer::docmd_webs(const char* strParam)
{
    std::map<std::string, std::string> maps;
    if (splitString(strParam, ' ', maps) <= 0) return;
    auto it = maps.find("port");
    if (it != maps.end()) m_svrport = atoi(it->second.c_str());
    it = maps.find("bindip");
    if (it != maps.end()) m_bindip = it->second;
    it = maps.find("ssl_enabled");
    if (it != maps.end()) m_bSSLenabled = (it->second == "true");
    it = maps.find("ssl_verify");
    if (it != maps.end()) m_bSSLverify = (it->second == "true");

    long lAccess = HTTP_ACCESS_READ;
    std::string rootpath, defaultPage;
    it = maps.find("root");
    if (it != maps.end()) rootpath = it->second;
    it = maps.find("default");
    if (it != maps.end()) defaultPage = it->second;
    setRoot(rootpath.empty() ? NULL : rootpath.c_str(), lAccess,
            defaultPage.empty() ? NULL : defaultPage.c_str());
}

// ---------------------------------------------------------------------------
// docmd_user – parse "user account=X pswd=Y access=ACCESS_ALL"
// ---------------------------------------------------------------------------
void DownpourServer::docmd_user(const char* strParam)
{
    std::map<std::string, std::string> maps;
    if (splitString(strParam, ' ', maps) <= 0) return;
    auto itAcct = maps.find("account");
    auto itPswd = maps.find("pswd");
    auto itAccs = maps.find("access");
    if (itAcct == maps.end()) return;

    long lAccess = DOWNPOUR_ACCESS_NONE;
    if (itAccs != maps.end())
    {
        const char* ptr = itAccs->second.c_str();
        if (strstr(ptr, "ACCESS_ALL")) lAccess = DOWNPOUR_ACCESS_ALL;
        else if (strstr(ptr, "ACCESS_TORRENT")) lAccess = DOWNPOUR_ACCESS_TORRENT;
    }
    if (lAccess == DOWNPOUR_ACCESS_NONE) return;

    std::string user = itAcct->second;
    ::_strlwr((char*)user.c_str());
    std::string pswd = (itPswd != maps.end()) ? itPswd->second : "";
    m_mapUsers[user] = { pswd, lAccess };
    m_bAnonymous = false;
}

// ---------------------------------------------------------------------------
// Small response helpers
// ---------------------------------------------------------------------------
void DownpourServer::sendJson(socketTCP* psock, httpResponse& httprsp,
                               const std::string& json, int code)
{
    httprsp.NoCache();
    httprsp.AddHeader(std::string("Content-Type"),
                      std::string("application/json; charset=utf-8"));
    httprsp.lContentLength((long)json.size());
    httprsp.send_rspH(psock, code, "OK");
    psock->Send((int)json.size(), json.c_str(), -1);
}

void DownpourServer::sendText(socketTCP* psock, httpResponse& httprsp,
                               const std::string& text, int code)
{
    httprsp.NoCache();
    httprsp.AddHeader(std::string("Content-Type"),
                      std::string("text/plain; charset=utf-8"));
    httprsp.lContentLength((long)text.size());
    httprsp.send_rspH(psock, code, code == 200 ? "OK" : "Error");
    psock->Send((int)text.size(), text.c_str(), -1);
}

// ---------------------------------------------------------------------------
// JSON escaping helper
// ---------------------------------------------------------------------------
static std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s)
    {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20)  { char buf[8]; snprintf(buf,sizeof(buf),"\\u%04x",c); out+=buf; }
        else                out += (char)c;
    }
    return out;
}

// Convert a 20-byte hash to a 40-char hex string
static std::string hashToHex(const uint8_t* h)
{
    char buf[41];
    for (int i = 0; i < 20; i++)
        snprintf(buf + i * 2, 3, "%02x", h[i]);
    buf[40] = 0;
    return buf;
}

// Convert a 40-char hex string to a 20-byte hash; returns false on error
static bool hexToHash(const char* hex, uint8_t out[20])
{
    if (!hex || strlen(hex) < 40) return false;
    for (int i = 0; i < 20; i++)
    {
        unsigned int v;
        if (sscanf(hex + i * 2, "%02x", &v) != 1) return false;
        out[i] = (uint8_t)v;
    }
    return true;
}

// ---------------------------------------------------------------------------
// GET /api/list
// Returns a JSON array of torrent objects with current stats.
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_torrent_list(socketTCP* psock,
                                           httpResponse& httprsp)
{
    if (!g_torrentCore)
    {
        sendJson(psock, httprsp, "[]");
        return true;
    }

    std::string json = "[";
    bool first = true;

    for (const auto& t : g_torrentCore->getTorrents())
    {
        if (!first) json += ",";
        first = false;

        const char* stateStr = "stopped";
        switch (t->getState())
        {
        case mttApi::Torrent::State::Active:
            stateStr = t->finished() ? "seeding" : "downloading"; break;
        case mttApi::Torrent::State::Stopping:
            stateStr = "stopping"; break;
        case mttApi::Torrent::State::CheckingFiles:
            stateStr = "checking"; break;
        case mttApi::Torrent::State::DownloadingMetadata:
            stateStr = "metadata"; break;
        default: break;
        }

        auto& xfer = t->getFileTransfer();
        auto& peers = t->getPeers();

        // total size from files info (0 if metadata not yet available)
        uint64_t totalSize = 0;
        if (t->hasMetadata())
            for (const auto& f : t->getFilesInfo())
                totalSize += f.size;

        // upload ratio: uploaded / downloaded (−1 when no data downloaded yet)
        uint64_t dl = xfer.downloaded();
        uint64_t ul = xfer.uploaded();
        double ratio = (dl > 0) ? (double)ul / (double)dl : -1.0;

        // ETA in seconds (−1 = unknown / not applicable)
        int64_t eta = -1;
        uint32_t dlSpeed = xfer.downloadSpeed();
        if (dlSpeed > 0 && totalSize > 0 && !t->finished())
        {
            uint64_t remaining = totalSize > dl ? totalSize - dl : 0;
            eta = (int64_t)(remaining / dlSpeed);
        }

        char obj[1024];
        snprintf(obj, sizeof(obj),
            "{"
            "\"hash\":\"%s\","
            "\"name\":\"%s\","
            "\"state\":\"%s\","
            "\"progress\":%.4f,"
            "\"downloadSpeed\":%u,"
            "\"uploadSpeed\":%u,"
            "\"downloaded\":%llu,"
            "\"uploaded\":%llu,"
            "\"totalSize\":%llu,"
            "\"peers\":%u,"
            "\"seeds\":%u,"
            "\"ratio\":%.4f,"
            "\"eta\":%lld"
            "}",
            hashToHex(t->hash()).c_str(),
            jsonEscape(t->name()).c_str(),
            stateStr,
            (double)t->progress(),
            dlSpeed,
            xfer.uploadSpeed(),
            (unsigned long long)dl,
            (unsigned long long)ul,
            (unsigned long long)totalSize,
            peers.connectedCount(),
            peers.receivedCount(),
            ratio,
            (long long)eta
        );
        json += obj;
    }
    json += "]";
    sendJson(psock, httprsp, json);
    return true;
}

// ---------------------------------------------------------------------------
// POST /api/add_magnet   body: magnet=<URI>
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_torrent_add_magnet(socketTCP* psock,
                                                  httpRequest& httpreq,
                                                  httpResponse& httprsp)
{
    const char* magnet = httpreq.Request("magnet");
    if (!magnet || !*magnet)
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"missing magnet parameter\"}", 400);
        return true;
    }

    if (!g_torrentCore)
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"core not initialised\"}", 500);
        return true;
    }

    auto [status, torrent] = g_torrentCore->addMagnet(magnet);
    if (torrent)
    {
        torrent->start();
        std::string resp = "{\"ok\":true,\"hash\":\"";
        resp += hashToHex(torrent->hash());
        resp += "\"}";
        sendJson(psock, httprsp, resp);
    }
    else
    {
        char err[128];
        snprintf(err, sizeof(err),
                 "{\"ok\":false,\"error\":\"addMagnet failed, status %d\"}",
                 (int)status);
        sendJson(psock, httprsp, err, 400);
    }
    return true;
}

// ---------------------------------------------------------------------------
// POST /api/add_torrent   – raw .torrent binary in the request body
// The client submits:  Content-Type: application/octet-stream  (or multipart)
// and the binary data is in httpreq.get_contentData().
// Any remaining bytes are streamed in via psock.
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_torrent_add_file(socketTCP* psock,
                                               httpRequest& httpreq,
                                               httpResponse& httprsp)
{
    if (!g_torrentCore)
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"core not initialised\"}", 500);
        return true;
    }

    // Collect the full file data
    long contentLen = httpreq.get_contentLen();
    cBuffer& buf    = httpreq.get_contentData();
    long received   = buf.len();

    // Stream the remainder if the request body wasn't fully buffered
    if (received < contentLen && contentLen > 0)
    {
        buf.Resize(contentLen + 1);
        while (received < contentLen)
        {
            int n = psock->Receive(buf.str() + received,
                                   buf.size() - received,
                                   HTTP_MAX_RESPTIMEOUT);
            if (n <= 0) break;
            received += n;
        }
        buf.len() = received;
    }

    if (buf.len() == 0)
    {
        sendJson(psock, httprsp,
                 "{\"ok\":false,\"error\":\"empty torrent file\"}", 400);
        return true;
    }

    auto [status, torrent] = g_torrentCore->addFile(
        reinterpret_cast<const uint8_t*>(buf.str()),
        (std::size_t)buf.len());

    if (torrent)
    {
        torrent->start();
        std::string resp = "{\"ok\":true,\"hash\":\"";
        resp += hashToHex(torrent->hash());
        resp += "\",\"name\":\"";
        resp += jsonEscape(torrent->name());
        resp += "\"}";
        sendJson(psock, httprsp, resp);
    }
    else
    {
        char err[128];
        snprintf(err, sizeof(err),
                 "{\"ok\":false,\"error\":\"addFile failed, status %d\"}",
                 (int)status);
        sendJson(psock, httprsp, err, 400);
    }
    return true;
}

// ---------------------------------------------------------------------------
// POST /api/start   body: hash=<40-char hex>
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_torrent_start(socketTCP* psock,
                                            httpRequest& httpreq,
                                            httpResponse& httprsp)
{
    const char* hashStr = httpreq.Request("hash");
    uint8_t hash[20];
    if (!hexToHash(hashStr, hash))
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"invalid hash\"}", 400);
        return true;
    }
    auto t = g_torrentCore ? g_torrentCore->getTorrent(hash) : nullptr;
    if (!t)
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"torrent not found\"}", 404);
        return true;
    }
    t->start();
    sendJson(psock, httprsp, "{\"ok\":true}");
    return true;
}

// ---------------------------------------------------------------------------
// POST /api/stop   body: hash=<40-char hex>
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_torrent_stop(socketTCP* psock,
                                           httpRequest& httpreq,
                                           httpResponse& httprsp)
{
    const char* hashStr = httpreq.Request("hash");
    uint8_t hash[20];
    if (!hexToHash(hashStr, hash))
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"invalid hash\"}", 400);
        return true;
    }
    auto t = g_torrentCore ? g_torrentCore->getTorrent(hash) : nullptr;
    if (!t)
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"torrent not found\"}", 404);
        return true;
    }
    t->stop();
    sendJson(psock, httprsp, "{\"ok\":true}");
    return true;
}

// ---------------------------------------------------------------------------
// POST /api/remove   body: hash=<40-char hex> [&delete_files=1]
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_torrent_remove(socketTCP* psock,
                                             httpRequest& httpreq,
                                             httpResponse& httprsp)
{
    const char* hashStr = httpreq.Request("hash");
    uint8_t hash[20];
    if (!hexToHash(hashStr, hash))
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"invalid hash\"}", 400);
        return true;
    }
    if (!g_torrentCore)
    {
        sendJson(psock, httprsp, "{\"ok\":false,\"error\":\"core not initialised\"}", 500);
        return true;
    }

    const char* delParam = httpreq.Request("delete_files");
    bool deleteFiles = (delParam && atoi(delParam) != 0);

    mtt::Status st = g_torrentCore->removeTorrent(hash, deleteFiles);
    if (st == mtt::Status::Success || st == mtt::Status::I_AlreadyExists)
        sendJson(psock, httprsp, "{\"ok\":true}");
    else
    {
        char err[128];
        snprintf(err, sizeof(err),
                 "{\"ok\":false,\"error\":\"remove failed, status %d\"}", (int)st);
        sendJson(psock, httprsp, err, 400);
    }
    return true;
}

// ---------------------------------------------------------------------------
// GET /api/settings
// Returns current speed-limit settings as JSON.
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_settings_get(socketTCP* psock,
                                           httpResponse& httprsp)
{
    auto cfg = mtt::config::getExternal();
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"maxDownloadSpeed\":%u,\"maxUploadSpeed\":%u}",
             cfg.transfer.maxDownloadSpeed,
             cfg.transfer.maxUploadSpeed);
    sendJson(psock, httprsp, buf);
    return true;
}

// ---------------------------------------------------------------------------
// POST /api/settings
// body: max_download_speed=<bytes/s> [&max_upload_speed=<bytes/s>]
// Zero means unlimited.
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_settings_set(socketTCP* psock,
                                           httpRequest& httpreq,
                                           httpResponse& httprsp)
{
    auto cfg = mtt::config::getExternal();

    const char* dlParam = httpreq.Request("max_download_speed");
    const char* ulParam = httpreq.Request("max_upload_speed");

    if (dlParam) cfg.transfer.maxDownloadSpeed = (uint32_t)atol(dlParam);
    if (ulParam) cfg.transfer.maxUploadSpeed   = (uint32_t)atol(ulParam);

    mtt::config::setValues(cfg.transfer);
    sendJson(psock, httprsp, "{\"ok\":true}");
    return true;
}

// ---------------------------------------------------------------------------
// Authentication helpers (same pattern as rmtsvc)
// ---------------------------------------------------------------------------
bool DownpourServer::httprsp_checkcode(socketTCP* psock,
                                        httpResponse& httprsp,
                                        httpSession& session)
{
    // Generate a simple 4-digit numeric code (kept simple for embedded use)
    srand((unsigned)time(NULL));
    char code[8];
    snprintf(code, sizeof(code), "%04d", rand() % 10000);
    session["checkcode"] = code;

    // Return the code as a plain PNG-less text image placeholder
    // (A real implementation could render a proper CAPTCHA bitmap)
    httprsp.NoCache();
    httprsp.AddHeader(std::string("Content-Type"), std::string("text/plain"));
    httprsp.lContentLength((long)strlen(code));
    httprsp.send_rspH(psock, 200, "OK");
    psock->Send((int)strlen(code), code, -1);
    return true;
}

bool DownpourServer::httprsp_login(socketTCP* psock,
                                    httpRequest& httpreq,
                                    httpResponse& httprsp,
                                    httpSession& session)
{
    session["user"] = "";
    session["lAccess"] = "";

    const char* ptr_user  = httpreq.Request("user");
    const char* ptr_pswd  = httpreq.Request("pswd");
    const char* ptr_chk   = httpreq.Request("chkcode");
    const char* ptr_rem   = httpreq.Request("remember");

    if (!ptr_user || !ptr_pswd || !ptr_chk) return false;

    // Verify check code (case-insensitive, same as rmtsvc)
    if (session["checkcode"] == "" ||
        strcasecmp(session["checkcode"].c_str(), ptr_chk) != 0)
        return false;

    // Look up user in our map
    std::string userLower = ptr_user;
    ::_strlwr((char*)userLower.c_str());
    auto it = m_mapUsers.find(userLower);
    if (it == m_mapUsers.end()) return false;
    if (it->second.first != std::string(ptr_pswd)) return false;

    // Authentication succeeded
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%ld", it->second.second);
    session["user"]    = it->first;
    session["lAccess"] = tmp;

    // Issue remember-me token (30 days) using CryptGenRandom for randomness
    if (ptr_rem && strcmp(ptr_rem, "1") == 0)
    {
        static const char chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        char token[33]; token[32] = 0;
        BYTE randBytes[32] = {0};
        HCRYPTPROV hProv = 0;
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL,
                                CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
        {
            CryptGenRandom(hProv, 32, randBytes);
            CryptReleaseContext(hProv, 0);
        }
        else
        {
            srand((unsigned)time(NULL));
            for (int i = 0; i < 32; i++) randBytes[i] = (BYTE)rand();
        }
        for (int i = 0; i < 32; i++) token[i] = chars[randBytes[i] % 62];

        time_t tExpires = time(NULL) + 30 * 24 * 60 * 60;
        RememberEntry entry;
        entry.user = it->first;
        entry.lAccess = it->second.second;
        entry.expires = tExpires;
        {
            std::lock_guard<cMutex> lk(m_rememberMutex);
            m_rememberTokens[token] = entry;
        }
        saveRememberTokens();

        struct tm* gmt = gmtime(&tExpires);
        char strExpires[64];
        strftime(strExpires, sizeof(strExpires),
                 "%a, %d-%b-%Y %H:%M:%S GMT", gmt);

        httprsp.SetCookie("downpour_remember", token, "/");
        TNew_Cookie* pCk = httprsp.SetCookie("downpour_remember");
        if (pCk)
        {
            pCk->cookie_expires = strExpires;
            pCk->cookie_httponly = true;
        }
    }

    httprsp_Redirect(psock, httprsp, "/index.htm");
    return true;
}

// ---------------------------------------------------------------------------
// Main HTTP request dispatcher
// ---------------------------------------------------------------------------
bool DownpourServer::onHttpReq(socketTCP* psock,
                                httpRequest& httpreq,
                                httpSession& session,
                                std::map<std::string, std::string>& /*app*/,
                                httpResponse& httprsp)
{
    const std::string& url = httpreq.url();

    // ---- unauthenticated endpoints ----
    if (strcasecmp(url.c_str(), "/checkcode") == 0)
    {
        httprsp_checkcode(psock, httprsp, session);
        return true;
    }
    if (strcasecmp(url.c_str(), "/login") == 0)
    {
        httprsp_login(psock, httpreq, httprsp, session);
        return true;
    }

    // ---- check authentication ----
    long lAccess = atol(session["lAccess"].c_str());
    if (lAccess == DOWNPOUR_ACCESS_NONE)
    {
        // Check remember-me cookie
        const char* tok = httpreq.Cookies("downpour_remember");
        if (tok && !m_bAnonymous)
        {
            std::lock_guard<cMutex> lk(m_rememberMutex);
            auto it = m_rememberTokens.find(tok);
            if (it != m_rememberTokens.end() && time(NULL) < it->second.expires)
            {
                session["user"]    = it->second.user;
                char tmp[32];
                snprintf(tmp, sizeof(tmp), "%ld", it->second.lAccess);
                session["lAccess"] = tmp;
                lAccess = it->second.lAccess;
            }
        }
    }
    if (lAccess == DOWNPOUR_ACCESS_NONE)
    {
        if (m_bAnonymous)
        {
            session["user"]    = "Anonymous";
            session["lAccess"] = "-1";
        }
        else if (strcasecmp(url.c_str(), "/login.htm") != 0)
        {
            httprsp_Redirect(psock, httprsp, "/login.htm");
            return true;
        }
    }

    // ---- root redirect ----
    if (url == "/")
    {
        std::string dest = "/";
        dest += m_defaultPage;
        httprsp_Redirect(psock, httprsp, dest.c_str());
        return true;
    }

    // ---- torrent API ----
    if (strcasecmp(url.c_str(), "/api/list") == 0)
        return httprsp_torrent_list(psock, httprsp);

    if (strcasecmp(url.c_str(), "/api/add_magnet") == 0)
        return httprsp_torrent_add_magnet(psock, httpreq, httprsp);

    if (strcasecmp(url.c_str(), "/api/add_torrent") == 0)
        return httprsp_torrent_add_file(psock, httpreq, httprsp);

    if (strcasecmp(url.c_str(), "/api/start") == 0)
        return httprsp_torrent_start(psock, httpreq, httprsp);

    if (strcasecmp(url.c_str(), "/api/stop") == 0)
        return httprsp_torrent_stop(psock, httpreq, httprsp);

    if (strcasecmp(url.c_str(), "/api/remove") == 0)
        return httprsp_torrent_remove(psock, httpreq, httprsp);

    if (strcasecmp(url.c_str(), "/api/settings") == 0)
    {
        if (httpreq.get_reqType() == HTTP_REQ_POST)
            return httprsp_settings_set(psock, httpreq, httprsp);
        return httprsp_settings_get(psock, httprsp);
    }

    // Fall through to static file serving (index.htm, style.css, etc.)
    return false;
}
