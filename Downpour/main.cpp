/*******************************************************************
 *  main.cpp
 *  DESCRIPTION: Downpour – entry point.
 *
 *  Starts the net4cpp21 HTTP(S) web server and the mtTorrent engine.
 *  Torrents are downloaded/seeded on this machine; the browser UI is
 *  only a remote control panel.
 *
 *  Command-line flags (same convention as rmtsvc):
 *    -h   Run hidden (no console window)
 *    -i   Install auto-start registry entry
 *    -u   Remove auto-start registry entry
 *******************************************************************/

#include "downpour.h"
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Global mtTorrent core (shared with the HTTP handler)
// ---------------------------------------------------------------------------
std::shared_ptr<mttApi::Core> g_torrentCore;

const char* DownpourService::ServiceVers = "1.0";

static const char* sServiceName = "Downpour";
static const char* sServiceDesc = "Downpour BitTorrent Client Service";

// ---------------------------------------------------------------------------
// DownpourService implementation
// ---------------------------------------------------------------------------
DownpourService::DownpourService(LPCTSTR ServiceName, LPCTSTR ServiceDesc)
    : CNTService(ServiceName, NULL)
    , m_bFaceless(false)
    , m_hStop(NULL)
{
    m_lpServiceDesc = ServiceDesc;
}

void DownpourService::Stop()
{
    if (m_hStop) SetEvent(m_hStop);
    m_hSockEvent.Close();
    ReportStatus(SERVICE_STOP_PENDING, 3000);
}

void DownpourService::Shutdown()
{
    if (m_hStop) SetEvent(m_hStop);
    m_hSockEvent.Close();
    ReportStatus(SERVICE_STOP_PENDING, 3000);
}

bool DownpourService::installStartup() const
{
    HKEY hKEY;
    LPCTSTR lpRunPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (::RegOpenKeyEx(HKEY_CURRENT_USER, lpRunPath, 0, KEY_WRITE, &hKEY) != ERROR_SUCCESS) {
        printf("Failed to open startup registry key.\n");
        return false;
    }
    char szPath[MAX_PATH];
    ::GetModuleFileName(NULL, szPath, MAX_PATH);
    std::string regValue = "\"";
    regValue += szPath;
    regValue += "\" -h";
    bool bRet = (::RegSetValueEx(hKEY, m_lpServiceName, NULL, REG_SZ,
        (LPBYTE)regValue.c_str(), (DWORD)(regValue.length() + 1)) == ERROR_SUCCESS);
    ::RegCloseKey(hKEY);
    if (bRet) printf("%s installed to startup.\n", m_lpServiceName);
    else printf("Failed to install startup entry.\n");
    return bRet;
}

void DownpourService::removeStartup() const
{
    HKEY hKEY;
    LPCTSTR lpRunPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (::RegOpenKeyEx(HKEY_CURRENT_USER, lpRunPath, 0, KEY_WRITE, &hKEY) != ERROR_SUCCESS) {
        printf("Failed to open startup registry key.\n");
        return;
    }
    ::RegDeleteValue(hKEY, m_lpServiceName);
    ::RegCloseKey(hKEY);
    printf("Startup entry removed.\n");
}

bool DownpourService::isStartupInstalled() const
{
    HKEY hKEY;
    LPCTSTR lpRunPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (::RegOpenKeyEx(HKEY_CURRENT_USER, lpRunPath, 0, KEY_READ, &hKEY) != ERROR_SUCCESS)
        return false;
    DWORD dwType = REG_SZ;
    bool bRet = (::RegQueryValueEx(hKEY, m_lpServiceName, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS);
    ::RegCloseKey(hKEY);
    return bRet;
}

// ---------------------------------------------------------------------------
// Config parsing (minimal: sets, webs, user, download_dir)
// ---------------------------------------------------------------------------
void DownpourService::docmd_sets(const char* strParam)
{
    std::map<std::string, std::string> maps;
    if (splitString(strParam, ' ', maps) <= 0) return;
    auto it = maps.find("faceless");
    if (it != maps.end() && it->second == "TRUE") m_bFaceless = true;

    it = maps.find("install");
    if (it != maps.end() && it->second == "TRUE") installStartup();
}

void DownpourService::parseConfig(const char* strCommand)
{
    if (!strCommand || !*strCommand) return;

    if (strncasecmp(strCommand, "sets ", 5) == 0)
        docmd_sets(strCommand + 5);
    else if (strncasecmp(strCommand, "webs ", 5) == 0)
        m_websvr.docmd_webs(strCommand + 5);
    else if (strncasecmp(strCommand, "user ", 5) == 0)
        m_websvr.docmd_user(strCommand + 5);
    else if (strncasecmp(strCommand, "download_dir ", 13) == 0)
    {
        auto cfg = mtt::config::getExternal();
        cfg.files.defaultDirectory = strCommand + 13;
        mtt::config::setValues(cfg.files);
    }
    else if (strncasecmp(strCommand, "max_download_speed ", 19) == 0)
    {
        auto cfg = mtt::config::getExternal();
        cfg.transfer.maxDownloadSpeed = (uint32_t)atol(strCommand + 19);
        mtt::config::setValues(cfg.transfer);
    }
    else if (strncasecmp(strCommand, "max_upload_speed ", 17) == 0)
    {
        auto cfg = mtt::config::getExternal();
        cfg.transfer.maxUploadSpeed = (uint32_t)atol(strCommand + 17);
        mtt::config::setValues(cfg.transfer);
    }
}

// ---------------------------------------------------------------------------
// Main service Run loop
// ---------------------------------------------------------------------------
void DownpourService::Run(DWORD /*argc*/, LPTSTR* /*argv*/)
{
    // Read configuration file
    char cfgPath[MAX_PATH];
    ::GetModuleFileName(NULL, cfgPath, MAX_PATH);
    char* pslash = strrchr(cfgPath, '\\');
    if (pslash) *(pslash + 1) = 0;
    strcat(cfgPath, "downpour.ini");

    FILE* fp = fopen(cfgPath, "r");
    if (fp)
    {
        char linebuf[512];
        while (fgets(linebuf, sizeof(linebuf), fp))
        {
            // Strip trailing newline / whitespace
            int len = (int)strlen(linebuf);
            while (len > 0 && (linebuf[len - 1] == '\r' || linebuf[len - 1] == '\n' || linebuf[len - 1] == ' '))
                linebuf[--len] = 0;
            if (len == 0 || linebuf[0] == '#' || linebuf[0] == ';') continue;
            parseConfig(linebuf);
        }
        fclose(fp);
    }

    // Ensure a download directory is set
    {
        auto cfg = mtt::config::getExternal();
        if (cfg.files.defaultDirectory.empty())
        {
            char dlDir[MAX_PATH];
            ::GetModuleFileName(NULL, dlDir, MAX_PATH);
            char* p = strrchr(dlDir, '\\');
            if (p) *(p + 1) = 0;
            strcat(dlDir, "downloads");
            ::CreateDirectoryA(dlDir, NULL);
            cfg.files.defaultDirectory = dlDir;
            mtt::config::setValues(cfg.files);
        }

        // Enable DHT by default
        cfg.dht.enabled = true;
        mtt::config::setValues(cfg.dht);
    }

    // Initialise mtTorrent
    g_torrentCore = mttApi::Core::create();

    // Load previously added torrents (mtTorrent persists state automatically)
    for (auto& t : g_torrentCore->getTorrents())
        t->start();

    // Start web server
    if (!m_websvr.Start())
        RW_LOG_PRINT(LOGLEVEL_ERROR, "[Downpour] Cannot start web server on port %d.\r\n",
                     m_websvr.m_svrport);
    else
        RW_LOG_PRINT(LOGLEVEL_INFO, "[Downpour] Web UI available at http://127.0.0.1:%d\r\n",
                     m_websvr.m_svrport);

    // Wait for stop signal
    m_hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    socketBase* pEvent = &m_hSockEvent;
    if (pEvent) pEvent->WaitEvent(m_hStop, INFINITE);
    else         WaitForSingleObject(m_hStop, INFINITE);

    m_websvr.Stop();
    g_torrentCore.reset();
    CloseHandle(m_hStop);
    m_hStop = NULL;
}

// ---------------------------------------------------------------------------
// main()
// ---------------------------------------------------------------------------
static DownpourService g_service(sServiceName, sServiceDesc);

int main(int argc, char* argv[])
{
    bool bHidden    = false;
    bool bInstall   = false;
    bool bUninstall = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)  bHidden    = true;
        if (strcmp(argv[i], "-i") == 0)  bInstall   = true;
        if (strcmp(argv[i], "-u") == 0)  bUninstall = true;
    }

    if (bInstall)   { g_service.installStartup();   return 0; }
    if (bUninstall) { g_service.removeStartup();    return 0; }

    if (bHidden)
        ::FreeConsole();

    return g_service.DebugService(argc, argv);
}
