/*******************************************************************
   *	rmtsvc.h 
   *    DESCRIPTION: rmtsvc remote control tool
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-30
   *	
   *******************************************************************/

#include "net4cpp21/include/sysconfig.h"
#include "net4cpp21/include/cCoder.h"
#include "net4cpp21/include/cLogger.h"
#include "net4cpp21/utils/utils.h"

#include "net4cpp21/include/httpsvr.h"
#ifdef _DEBUG
#pragma comment( lib, "libs/bin/net4cpp21_d" )
#elif defined _SUPPORT_OPENSSL_
#pragma comment( lib, "libs/bin/net4cpp21" )
#else
#pragma comment( lib, "libs/bin/net4cpp21_nossl" )
#endif

//#include "msnbot/msnbot.h"	//yyc remove MSN function
#include "ftpserver.h"
#include "proxyserver.h"
#include "telnetserver.h"
//-----------------------------vIDC--------------------------
#include "libs/vidc/webI/vidcManager.h"

using namespace std;
using namespace net4cpp21;

#define RMTSVC_ACCESS_NONE			0x0000
#define RMTSVC_ACCESS_ALL			0xFFFFFFFF
#define RMTSVC_ACCESS_SCREEN_ALL	0x0003
#define RMTSVC_ACCESS_SCREEN_VIEW	0x0001
#define RMTSVC_ACCESS_REGIST_ALL	0x000c
#define RMTSVC_ACCESS_REGIST_VIEW	0x0004
#define RMTSVC_ACCESS_SERVICE_ALL	0x0030
#define RMTSVC_ACCESS_SERVICE_VIEW	0x0010
#define RMTSVC_ACCESS_TELNET_ALL	0x00c0
#define RMTSVC_ACCESS_TELNET_VIEW	0x0040
#define RMTSVC_ACCESS_FILE_ALL		0x0300
#define RMTSVC_ACCESS_FILE_VIEW		0x0100
#define RMTSVC_ACCESS_FTP_ADMIN		0x0c00 //FTP configuration management permission
#define RMTSVC_ACCESS_VIDC_ADMIN	0x3000 //vIDC/Proxy management permission

class webServer : public httpServer
{
public:
	webServer();
	virtual ~webServer(){}
	bool Start(); //start service
	void Stop();//stop service
	
	void setRoot(const char *rpath,long lAccess,const char *defaultPage);
	void docmd_webs(const char *strParam);
	void docmd_webiprules(const char *strParam);
	void docmd_user(const char *strParam);

	int m_svrport;
	std::string m_bindip;
	bool m_bPowerOff; //whether shutdown/restart can be performed without permissions
private:
	virtual bool onHttpReq(socketTCP *psock,httpRequest &httpreq,httpSession &session,
			std::map<std::string,std::string>& application,httpResponse &httprsp);

	bool setLastModify(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool httprsp_version(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_docommandEx(socketTCP *psock,httpResponse &httprsp,const char *strCommand);
	bool httprsp_telnet(socketTCP *psock,httpResponse &httprsp,long lAccess);
	bool httprsp_checkcode(socketTCP *psock,httpResponse &httprsp,httpSession &session);
	bool httprsp_login(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_capSetting(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session,bool bSetting);
	//set to only capture the screen of the specified window
	bool httprsp_capWindow(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_getpswdfromwnd(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_GetClipBoard(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_SetClipBoard(socketTCP *psock,httpResponse &httprsp,const char *strval);
	bool httprsp_msevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_keyevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool httprsp_command(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd);
	bool httprsp_cmdpage(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd);
	bool httprsp_capDesktop(socketTCP *psock,httpResponse &httprsp,httpSession &session);
	bool httprsp_capStream(socketTCP *psock,httpResponse &httprsp,httpSession &session);
	bool httprsp_capAudio(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_getCursor(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_sysinfo(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_usageimage(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_fport(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_plist(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_mlist(socketTCP *psock,httpResponse &httprsp,DWORD pid);
	bool httprsp_pkill(socketTCP *psock,httpResponse &httprsp,DWORD pid);
	bool httprsp_mdattach(socketTCP *psock,httpResponse &httprsp,DWORD pid,HMODULE hmdl,long count);
	bool httprsp_slist(socketTCP *psock,httpResponse &httprsp);
	bool sevent(const char *sname,const char *cmd);
	bool httprsp_reglist(socketTCP *psock,httpResponse &httprsp,const char *skey,int listWhat);
	bool httprsp_regkey_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *skey);
	bool httprsp_regkey_add(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *skey);
	bool httprsp_regitem_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *sname);
	bool httprsp_regitem_add(socketTCP *psock,httpResponse &httprsp,const char *spath,
							const char *stype,const char *sname,const char *svalue);
	bool httprsp_regitem_md(socketTCP *psock,httpResponse &httprsp,const char *spath,
							const char *stype,const char *sname,const char *svalue);
	bool httprsp_regitem_ren(socketTCP *psock,httpResponse &httprsp,const char *spath,
							const char *sname,const char *snewname);

	bool  httprsp_filelist(socketTCP *psock,httpResponse &httprsp,const char *spath,int listWhat,bool bdsphide);
	bool  httprsp_folder_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *fname,bool bdsphide);
	bool  httprsp_folder_new(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *fname,bool bdsphide);
	bool  httprsp_folder_ren(socketTCP *psock,httpResponse &httprsp,const char *spath,
									 const char *fname,const char *newname,bool bdsphide);
	bool  httprsp_file_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *fname,bool bdsphide);
	bool  httprsp_file_ren(socketTCP *psock,httpResponse &httprsp,const char *spath,
									 const char *fname,const char *newname,bool bdsphide);
	bool  httprsp_file_run(socketTCP *psock,httpResponse &httprsp,const char *spath);
	
	bool  httprsp_profile_verinfo(socketTCP *psock,httpResponse &httprsp,const char *spath);
	bool  httprsp_profile(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *prof);
	bool  httprsp_profolder(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *prof);
	bool  httprsp_prodrive(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *svolu);
	bool  httprsp_get_upratio(socketTCP *psock,httpResponse &httprsp,httpSession &session);
	bool  httprsp_upload(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	
	bool  httprsp_ftpsets(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_ftpusers(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_ftpini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	
	bool  httprsp_proxysets(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_proxyusers(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_proxyini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	//--------------------------vIDC--------------------------------------------------
	bool  httprsp_mportl(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_mportr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_vidcini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_vidcsvr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_vidccs(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	
	bool httprsp_upnp(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool httprsp_upnpxml(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);

	std::string m_defaultPage; //default document
	int m_quality;//capture desktop image quality
	DWORD m_dwImgSize;//capture desktop image size: 0=desktop size, otherwise specified size HWORD=h, WWORD=w

	//first, visitor account - case insensitive. Saved in lowercase
	//second.first - visitor account  second.second - visitor permissions
	std::map<std::string,std::pair<std::string,long> > m_mapUsers;

	// remember-me tokens: token -> (username, lAccess)
	struct RememberEntry { std::string user; long lAccess; time_t expires; };
	std::map<std::string,RememberEntry> m_rememberTokens;
	cMutex m_rememberMutex;

	bool m_bSSLenabled; //start SSL service
	bool m_bSSLverify; //whether to perform client certificate authentication
	//yyc add 2010-02-23 if account is configured in ini, anonymous access is not supported
	bool m_bAnonymous; //whether to allow anonymous access, default is true
};

//---------------------------------------------------------------
//---------------------------------------------------------------
typedef struct _TaskTimer //scheduled task structure
{
	long h,m;//scheduled hour/minute, if interval task then h indicates the interval (seconds)
	long type; //schedule type: 't'-poll interval (s) execution, 'd'-execute daily at scheduled time
	long flag; //default: 0
	std::string strTask; //scheduled task
}TaskTimer;
class sockEvent : public socketBase
{
public:
	sockEvent(){ m_sockstatus=SOCKS_OPENED; }
	virtual ~sockEvent(){}
	virtual void Close(){ m_sockstatus=SOCKS_CLOSED;}
}; //used to close all blocked sockets in time when program exits
#include "NTService.h"
class MyService : public CNTService 
{
	sockEvent m_hSockEvent; //used to close all blocked sockets in time when program exits
	HANDLE m_hStop; //service stopped event object handle
	HANDLE m_hStopEvent; //event for whether to allow stopping service via SCM or console
						//if set, stop password, then create a named event based on stop password
	bool m_bSpyself;//whether to monitor for self crash/abnormal exit
	bool m_bFaceless; //default double-click run: whether to show console window
	std::vector<TaskTimer> m_tasklist; //scheduled task list
	bool CreateTaskTime(const char *ptrAt,const char *strTask);
	void parseCommand(const char *strCommand);
	void docmd_sets(const char *strParam);
//*********************user additional code start ****************************************
public:
	webServer m_websvr;
//	msnShell m_msnbot;	//yyc remove MSN function
	ftpsvrEx m_ftpsvr;
	proxysvrEx m_proxysvr;
	telServerEx m_telsvr;
	//---------------------------vIDC------------------------------
	vidcManager m_vidcManager; //VIDC manager class
	std::string m_preCmdpage; //cmdpage command line page handler prefix
//*********************user additional code end  ****************************************
public:
	static const char *ServiceVers;
	static MyService *GetService() { return (MyService *)AfxGetService(); }
public:
	explicit MyService(LPCTSTR ServiceName, LPCTSTR ServiceDesc = 0);
	virtual ~MyService(){}
	
	void SetStopEvent(const char *stop_pswd);
	BOOL AutoSpy(const char *commandline);//start automatic monitoring
	socketBase *GetSockEvent(){ return &m_hSockEvent; }
private:
	//create service stopped password protection event
	void CreateStopEvent(const char *stop_pswd);
	//overloaded function
	virtual void	Run(DWORD argc, LPTSTR *argv); //service/program execution body
	virtual void	Stop();//service stopped handle
	virtual void	Stop_Request();
	virtual void	Shutdown(); //system shutdown handle

};
//convert a relative path to an absolute path
extern void getAbsoluteFilePath(std::string &spath);
extern int splitString(const char *str,char delm,std::map<std::string,std::string> &maps);
extern int splitString(const char *str,char delm,std::vector<std::string> &vec,int maxSplit);
