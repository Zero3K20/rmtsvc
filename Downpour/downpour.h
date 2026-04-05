/*******************************************************************
 *  downpour.h
 *  DESCRIPTION: Downpour - BitTorrent client built on RmtSvc's
 *               HTTP(S) server infrastructure and mtTorrent engine.
 *
 *  Remote desktop, remote management, vIDC, FTP, proxy, and telnet
 *  features from RmtSvc have been intentionally removed; only the
 *  web-server plumbing (net4cpp21) is retained as the transport.
 *******************************************************************/
#pragma once

#include "../net4cpp21/include/sysconfig.h"
#include "../net4cpp21/include/cCoder.h"
#include "../net4cpp21/include/cLogger.h"
#include "../net4cpp21/utils/utils.h"
#include "../net4cpp21/include/httpsvr.h"

#ifdef _DEBUG
#pragma comment(lib, "../libs/bin/net4cpp21_d")
#elif defined _SUPPORT_OPENSSL_
#pragma comment(lib, "../libs/bin/net4cpp21")
#else
#pragma comment(lib, "../libs/bin/net4cpp21_nossl")
#endif

#include "../NTService.h"

// mtTorrent API
#include "mtTorrent/Api/Core.h"
#include "mtTorrent/Api/Configuration.h"
#include "mtTorrent/Api/Torrent.h"
#include "mtTorrent/Api/Interface.h"
#include "mtTorrent/Api/FileTransfer.h"
#include "mtTorrent/Api/Peers.h"
#include "mtTorrent/Api/Files.h"
#include "mtTorrent/Api/MetadataDownload.h"
#include "mtTorrent/Public/Alerts.h"

#include <memory>
#include <string>
#include <map>
#include <mutex>

using namespace std;
using namespace net4cpp21;

// Access level flags (simplified – only torrent management, no remote-desktop)
#define DOWNPOUR_ACCESS_NONE    0x0000
#define DOWNPOUR_ACCESS_ALL     0xFFFF
#define DOWNPOUR_ACCESS_TORRENT 0x0001   // add / remove / control torrents

// Global mtTorrent core instance (shared between web-handler and main)
extern std::shared_ptr<mttApi::Core> g_torrentCore;

// -----------------------------------------------------------------------
// DownpourServer – HTTP(S) server that exposes the torrent management API
// and serves the single-page web UI.
// -----------------------------------------------------------------------
class DownpourServer : public httpServer
{
public:
    DownpourServer();
    virtual ~DownpourServer() {}

    bool Start();
    void Stop();

    void setRoot(const char* rpath, long lAccess, const char* defaultPage);
    void docmd_webs(const char* strParam);
    void docmd_user(const char* strParam);

    int         m_svrport;
    std::string m_bindip;
    bool        m_bAnonymous;

private:
    virtual bool onHttpReq(socketTCP* psock, httpRequest& httpreq,
                           httpSession& session,
                           std::map<std::string, std::string>& application,
                           httpResponse& httprsp);

    // Authentication helpers (same pattern as rmtsvc)
    bool httprsp_checkcode(socketTCP* psock, httpResponse& httprsp,
                           httpSession& session);
    bool httprsp_login(socketTCP* psock, httpRequest& httpreq,
                       httpResponse& httprsp, httpSession& session);

    // Torrent API handlers
    bool httprsp_torrent_list(socketTCP* psock, httpResponse& httprsp);
    bool httprsp_torrent_add_magnet(socketTCP* psock, httpRequest& httpreq,
                                    httpResponse& httprsp);
    bool httprsp_torrent_add_file(socketTCP* psock, httpRequest& httpreq,
                                   httpResponse& httprsp);
    bool httprsp_torrent_start(socketTCP* psock, httpRequest& httpreq,
                                httpResponse& httprsp);
    bool httprsp_torrent_stop(socketTCP* psock, httpRequest& httpreq,
                               httpResponse& httprsp);
    bool httprsp_torrent_remove(socketTCP* psock, httpRequest& httpreq,
                                 httpResponse& httprsp);

    // Small helpers
    void sendJson(socketTCP* psock, httpResponse& httprsp,
                  const std::string& json, int code = 200);
    void sendText(socketTCP* psock, httpResponse& httprsp,
                  const std::string& text, int code = 200);

    // Remember-me token storage (same pattern as rmtsvc)
    struct RememberEntry { std::string user; long lAccess; time_t expires; };
    std::map<std::string, RememberEntry> m_rememberTokens;
    cMutex m_rememberMutex;
    void saveRememberTokens();
    void loadRememberTokens();

    bool m_bSSLenabled;
    bool m_bSSLverify;
    std::string m_defaultPage;
};

// -----------------------------------------------------------------------
// DownpourService – the NT service / console app host
// -----------------------------------------------------------------------
class DownpourService : public CNTService
{
public:
    static const char* ServiceVers;

    DownpourService(LPCTSTR ServiceName, LPCTSTR ServiceDesc);

    void Stop()    override;
    void Shutdown() override;

    bool installStartup()   const;
    void removeStartup()    const;
    bool isStartupInstalled() const;

    socketBase* GetSockEvent() { return &m_hSockEvent; }

    static DownpourService* GetService()
    {
        return static_cast<DownpourService*>(AfxGetService());
    }

    DownpourServer m_websvr;

private:
    void Run(DWORD argc, LPTSTR* argv) override;
    void parseConfig(const char* strCommand);
    void docmd_sets(const char* strParam);

    bool   m_bFaceless;
    HANDLE m_hStop;
    socketBase m_hSockEvent;
    std::string m_preCmdpage;
};
