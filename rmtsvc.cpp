/*******************************************************************
   *	rmtsvc.cpp
   *    DESCRIPTION: rmtsvc remote control tool
   *
   *******************************************************************/

#include "rmtsvc.h"
const char * MyService :: ServiceVers="2.5.2 (build110105)";
static const char * sServiceName="rmtsvc";
static const char * sServiceDesc="Remote Control Service";

static std::string g_cfgfile;//configuration parameter filename
MyService :: MyService(LPCTSTR ServiceName, LPCTSTR ServiceDesc)
:CNTService(ServiceName,NULL)
{
	m_lpServiceDesc=ServiceDesc;
	m_hStop=NULL;
	m_hStopEvent=NULL;
	//change service default type to allow desktop interaction
	m_dwServiceType|=SERVICE_INTERACTIVE_PROCESS;
	m_bSpyself=true;//whether to monitor self for abnormal exit
	m_bFaceless=false; //whether to run without console window by default when double-clicked
}

//根据停止passwordcreatestopEvent事件
void MyService :: CreateStopEvent(const char *stop_pswd)
{//在服务模式下create的对象不能被application（debug模式）访问
//因此必须改变createevent对象的访问permissions，以便application能够访问此事件
	if(stop_pswd==NULL || stop_pswd[0]==0) return;
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sd;
	::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);	
	//加入一个空的安全description符，为了能使得其他程序都能访问。
	::SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE);
	std::string strStopswd(m_lpServiceName); strStopswd.append(stop_pswd);
	int nlen=cCoder::Base64EncodeSize(strStopswd.length());
	char *tmpbuf=new char[nlen+1];//BASE64编码
	if(tmpbuf==NULL) return;
	nlen=cCoder::base64_encode((char *)strStopswd.c_str(),strStopswd.length(),tmpbuf);
	tmpbuf[nlen]=0;
	m_hStopEvent = ::CreateEvent(&sa, TRUE, FALSE, tmpbuf);
	delete[] tmpbuf; return; 
}
void MyService :: SetStopEvent(const char *stop_pswd)
{
	if(stop_pswd==NULL || stop_pswd[0]==0) return;
	std::string strStopswd(m_lpServiceName); strStopswd.append(stop_pswd);
	int nlen=cCoder::Base64EncodeSize(strStopswd.length());
	char *tmpbuf=new char[nlen+1];//BASE64编码
	if(tmpbuf==NULL) return;
	nlen=cCoder::base64_encode((char *)strStopswd.c_str(),strStopswd.length(),tmpbuf);
	tmpbuf[nlen]=0;
	HANDLE hStopEvent = ::OpenEvent(EVENT_MODIFY_STATE, TRUE, tmpbuf);
	if(hStopEvent){ ::SetEvent(hStopEvent); ::CloseHandle(hStopEvent); }
	delete[] tmpbuf; return; 
}

void MyService :: Stop_Request() //service stopped请求事件
{	
	if(m_hStopEvent && WaitForSingleObject(m_hStopEvent, 100) != WAIT_OBJECT_0)
		return; //否则允许receiveSERVICE_ACCEPT_STOP
	m_dwControlsAccepted |=SERVICE_ACCEPT_STOP;
	ReportStatus(SERVICE_RUNNING);
}
void MyService :: Stop() //service stopped事件
{	
	if( m_hStop ) SetEvent(m_hStop);
	m_hSockEvent.Close();
	ReportStatus(SERVICE_STOP_PENDING,3000); 
}
void MyService :: Shutdown() //system shutdownhandle
{
	if( m_hStop ) SetEvent(m_hStop);
	m_hSockEvent.Close();
	ReportStatus(SERVICE_STOP_PENDING,3000);
	return;
}

//-----------------------------------------------------------------------

#define MAX_SAVEEXE_PARAMLENGTH 4096
long fileparam_flags=0x31323334;//文件末尾的参数起始flag
//从本文件末尾读出配置data
bool readParamfromfile(std::string &strret,const char *strexefile)
{
	char modalname[MAX_PATH]; char *pbuf=NULL;
	if(strexefile==NULL)
		::GetModuleFileName( NULL, modalname, MAX_PATH-1);
	else strcpy(modalname,strexefile);
	FILE *fp=fopen(modalname,"rb");
	if(fp==NULL) return false;
	//读出配置参数的length，不包含flag字符和本域length
	::fseek(fp,0-sizeof(long),SEEK_END);
	::fread((void *)modalname,sizeof(long),1,fp);
	int i;
	for(i=0;i<sizeof(long);i++)
		modalname[i]^=*( (char *)&fileparam_flags+i );
	long paramlen=*((long *)modalname); //获取参数length
	bool b=false;
	if(paramlen<=MAX_SAVEEXE_PARAMLENGTH && paramlen>0)
	{
		::fseek(fp,0-paramlen-sizeof(long)-sizeof(fileparam_flags),SEEK_END);
		::fread((void *)modalname,sizeof(fileparam_flags),1,fp);
		b=(*((long *)modalname)==fileparam_flags);
	}
	if( !b || (pbuf=new char[paramlen+1])==NULL )
	{
		::fclose(fp); return false;
	}
	paramlen=::fread((void *)pbuf,sizeof(char),paramlen,fp);
	pbuf[paramlen]=0;
	//先和flag进行逐字节异或
	for(i=0;i<paramlen;i++)
		pbuf[i]^=*( (char *)&fileparam_flags+i%sizeof(fileparam_flags) );
	//此时得到的是一个base64 encoding的字符串,先进行base64 decoding
	paramlen=cCoder::base64_decode(pbuf,paramlen,pbuf);
	pbuf[paramlen]=0; strret.append(pbuf);
	delete[] pbuf;
	::fclose(fp); return true;
}
//将specified的参数加密写入到specifiedexe的末尾
bool writeParamintofile(const char *strexefile,const char *param,long paramlen)
{
	if(param==NULL || paramlen<=0)
	{
		RW_LOG_PRINT(LOGLEVEL_ERROR,0,"wrong param to write.\r\n");
		return false;
	}
	FILE *fp=::fopen(strexefile,"ab");
	if(fp==NULL)
	{
		RW_LOG_PRINT(LOGLEVEL_ERROR,"failed to open %s for writing.\r\n",strexefile);
		return false;
	}
	//加密编码要写入的参数
	int buflen=cCoder::Base64EncodeSize(paramlen)+sizeof(long)+sizeof(fileparam_flags);
	char *pbuf=new char[buflen];
	if(pbuf==NULL){ ::fclose(fp); return false; }
	char *pstart=pbuf;
	memcpy((void *)pstart,(const void *)&fileparam_flags,sizeof(fileparam_flags));
	pstart+=sizeof(fileparam_flags);
	paramlen=cCoder::base64_encode((char *)param,paramlen,pstart);
	int i;
	for(i=0;i<paramlen;i++)
		pstart[i]^=*( (char *)&fileparam_flags+i%sizeof(fileparam_flags) );
	pstart+=paramlen;
	for(i=0;i<sizeof(long);i++)
		pstart[i]=*((char *)&paramlen+i) ^ *((char *)&fileparam_flags+i);
	//总共要追加写入的字节size
	paramlen+=sizeof(long)+sizeof(fileparam_flags);
	
	//-------------------
	char buffer[8];
	::fseek(fp,0-sizeof(long),SEEK_END);
	::fread((void *)buffer,sizeof(long),1,fp);
	for(i=0;i<sizeof(long);i++)
		buffer[i]^=*( (char *)&fileparam_flags+i );
	long datalen=*((long *)buffer); //获取参数length
	if(datalen<=MAX_SAVEEXE_PARAMLENGTH && datalen>0)
	{
		::fseek(fp,0-datalen-sizeof(long)-sizeof(fileparam_flags),SEEK_END);
		::fread((void *)buffer,sizeof(fileparam_flags),1,fp);
	}
	else *((long *)buffer)=~fileparam_flags;
	if(*((long *)buffer)==fileparam_flags)
		::fseek(fp,0-datalen-sizeof(long)-sizeof(fileparam_flags),SEEK_END);
	else
		::fseek(fp,0,SEEK_END);
	::fwrite(pbuf,sizeof(char),paramlen,fp);
	::fclose(fp); delete[] pbuf;
	return true;
}

//生成新的exe文件
bool saveAsExe(std::string &saveasfile)
{
	if(saveasfile=="") return false;
	bool ifNo_ini=false;
	const char *ptr_saveasfile=saveasfile.c_str();
	if(saveasfile[0]=='*'){ifNo_ini=true; ptr_saveasfile+=1;}
	FILE *fp=::fopen(g_cfgfile.c_str(),"rb");
	if(fp){
		::fseek(fp,0,SEEK_END);
		long filelen=::ftell(fp);
		if(filelen<MAX_SAVEEXE_PARAMLENGTH)
		{
			::fseek(fp,0,SEEK_SET);
			char buf[MAX_SAVEEXE_PARAMLENGTH+1];
			int l=(ifNo_ini)?sprintf(buf,"No-ini\r\n"):0;
			filelen=::fread(buf+l,sizeof(char),filelen,fp)+l;
			buf[filelen]=0;

			char modalname[MAX_PATH]; //复制文件
			::GetModuleFileName( NULL, modalname, MAX_PATH-1);
			if(::CopyFile(modalname,ptr_saveasfile,FALSE))
			{
				if(writeParamintofile(ptr_saveasfile,buf,filelen))
					RW_LOG_PRINT(LOGLEVEL_ERROR,"success to saving as %s.\r\n",ptr_saveasfile);
				else
					RW_LOG_PRINT(LOGLEVEL_ERROR,"failed to saving as %s.\r\n",ptr_saveasfile);
			}else RW_LOG_PRINT(LOGLEVEL_ERROR,0,"failed to create New copy\r\n");
		}else
			RW_LOG_PRINT(LOGLEVEL_ERROR,"failed to saving as %s,saving param too long(max %d)\r\n"
					,ptr_saveasfile,MAX_SAVEEXE_PARAMLENGTH);
		::fclose(fp); return true;
	}//?if(fp)
	else RW_LOG_PRINT(LOGLEVEL_ERROR,"failed to saving as %s.\r\n",ptr_saveasfile);
	return false;
}

#define NEXT_ARG ((((*Argv)[2])==TEXT('\0'))?(--Argc,*++Argv):(*Argv)+2)
#define NEXT_ARG_E ((--Argc,*++Argv))
void MyService :: Run(DWORD argc, LPTSTR *argv)
{
	// report to the SCM that we're about to start
	ReportStatus(SERVICE_START_PENDING);
	//设置defaultlog output级别为LOGLEVEL_INFO
	RW_LOG_SETLOGLEVEL(LOGLEVEL_INFO);
	
	bool bReadedIni=false; //用户是否进行ini参数配置
	//获取exe本身的配置参数 start----------------------------
	std::string strParam;
	if(bReadedIni=readParamfromfile(strParam,NULL))
	{//parseexe本身的配置info
		const char *pstart=strParam.c_str();
		const char *ptr=strchr(pstart,'\r');
		while(true){
			if(ptr) *(char *)ptr=0;
			//告诉应用程序不要read configuration文件
			if(strcmp(pstart,"No-ini")==0) g_cfgfile="";
			if(pstart[0]!='!') //不解释comment行
				parseCommand(pstart); //解释命令行pstart参数含义
			if(ptr==NULL) break;
			*(char *)ptr='\r'; pstart=ptr+1;
			if(*pstart=='\n') pstart++;
			ptr=strchr(pstart,'\r');
		}//?while
	}//?if(readParamfromfile(strParam,NULL))
	//获取exe本身的配置参数  end ----------------------------
	//read configuration文件info start---------------------------------
	FILE *fp=(g_cfgfile=="")?NULL:(::fopen(g_cfgfile.c_str(),"r"));
	if(fp){
		char sline[1024]; bReadedIni=true;
		while( ::fgets(sline,1024,fp) ){
			if(sline[0]!='!'){ //不解释comment行
				int len=strlen(sline); len--;
				while(len>=0 &&(sline[len]=='\n' || sline[len]=='\r')) sline[len--]=0;
				parseCommand(sline); //解释命令行sline参数含义
			}
		}//?while( ::fgets(sline,1024,fp)
		::fclose(fp);
	}//?if(fp)
	//read configuration文件info  end ---------------------------------
	//获取command-line arguments-----------------------------------------
	DWORD Argc;  LPTSTR * Argv;
	#ifdef UNICODE
		Argv = CommandLineToArgvW(GetCommandLineW(), &Argc );
	#else
		Argc = (DWORD) argc; Argv = argv;
	#endif
	if(Argc>0){
	while( ++Argv, --Argc ) {
		if( Argv[0][0] == TEXT('-') ) {
			switch( Argv[0][1] ) {
				case TEXT('d'):	// debug the service
					m_bFaceless=false;
					if(Argv[0][2]>='0' && Argv[0][2]<='4') 
						RW_LOG_SETLOGLEVEL((LOGLEVEL)('4'-Argv[0][2]));
					break;
//*********************用户其他代码 statrt ****************************************
//*********************用户其他代码  end  ****************************************
			}//?switch( Argv[0][1] ) 
		}//?if( Argv[0][0] == TEXT('-') ) 
	}//?while( ++Argv, --Argc )
	}//?if(Argc>0){	
	//获取command-line arguments------------------------------------------
	if(!m_bDebug) m_bFaceless=true;//服务模式运行,没有控制台窗口
	//如果没有控制台且specifiedoutput to console则取消输出
	if(m_bFaceless) ::FreeConsole();
	if(m_bFaceless){ if(RW_LOG_LOGTYPE()==LOGTYPE_STDOUT) RW_LOG_SETNONE();}
	else RW_LOG_OUTSTDOUT(true);//如果有控制台界面则同步output to console
	//createstop service事件
	m_hStop = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if(m_hStop==NULL){
		RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[Fatal] Can not create Event object.\r\n");
		ReportStatus(SERVICE_STOPPED,3000,0); return;
	}
	if(m_bDebug && m_hStopEvent){ ::CloseHandle(m_hStopEvent); m_hStopEvent=NULL; }
	//是否禁止通过控制台stop service
	if(m_hStopEvent) m_dwControlsAccepted &=(~SERVICE_ACCEPT_STOP); // 禁止receiveSTOP事件
	ReportStatus(SERVICE_START_PENDING);
	
	//服务start启动------------------------------
	if(RW_LOG_CHECK(LOGLEVEL_INFO)) RW_LOG_PRINTTIME(); //打印start运行time
	if(m_bSpyself) //启动自身异常退出监视
	{
		char commandline[MAX_PATH]; int len=0;
		if(!m_bDebug) len=sprintf(commandline," -s");
		for(int i=1;i<(int)argc;i++)
			len+=sprintf(commandline+len," %s",argv[i]);
		commandline[len]=0; AutoSpy(commandline);
	} RW_LOG_PRINT(LOGLEVEL_INFO,0,"program starting up.\r\n");
//*********************用户其他代码 start ****************************************	
	if(!m_websvr.Start()) //start web service
		RW_LOG_PRINT(LOGLEVEL_ERROR,"Can not start WWW server(%d).\r\n",m_websvr.m_svrport);
	if(!m_telsvr.Start()) //启动Telnet服务
		RW_LOG_PRINT(LOGLEVEL_ERROR,"Can not start Telnet server(%d).\r\n",m_telsvr.m_svrport);
	
	m_ftpsvr.readIni(); //读取并配置FTP服务的info
	m_proxysvr.readIni(); //读取并配置proxy service的info
	m_vidcManager.readIni(); //读取并配置vIDC的info
	if(m_vidcManager.m_upnp.upnpinfo().size()>0) m_vidcManager.m_upnp.Search(); //search UPnP device
	if(m_ftpsvr.m_settings.autoStart)
	{
		if(!m_ftpsvr.Start()) //启动FTP服务
			RW_LOG_PRINT(LOGLEVEL_ERROR,"Can not start FTP server(%d).\r\n",m_ftpsvr.m_settings.svrport);
	}
	if(m_proxysvr.m_settings.autorun)
	{
		if(!m_proxysvr.Start()) //启动proxy service
			RW_LOG_PRINT(LOGLEVEL_ERROR,"Can not start Proxy server(%d).\r\n",m_proxysvr.m_settings.svrport);
	}
	if(m_vidcManager.m_vidcsvr.m_autorun)
	{
		if(m_vidcManager.m_vidcsvr.Start()<=0)
			RW_LOG_PRINT(LOGLEVEL_ERROR,"Can not start vIDC server(%d).\r\n",m_vidcManager.m_vidcsvr.m_svrport);
	}
	m_vidcManager.mtcpl_Start();
	DWORD dwCount_vidcc=1;	//,dwCount_msnbot=1; //yyc remove MSN 2010-11-05
	//用户双击临时启动的程序且没有配置ini参数且web service启动了，匿名访问方式。则default启动IE
	if(!bReadedIni && !m_bFaceless && m_websvr.status()==SOCKS_LISTEN){
		char execmd[128]; 
		sprintf(execmd,"exec -shell http://127.0.0.1:%d/",m_websvr.getLocalPort());
		parseCommand(execmd);
	}//?if(!bReadedIni && !m_bFaceless)
//*********************用户其他代码  end  ****************************************
	ReportStatus(SERVICE_RUNNING);
	time_t tStartTime=time(NULL); 
	//等待服务end
	while( WaitForSingleObject(m_hStop, 1000) != WAIT_OBJECT_0 )
	{
//*********************用户其他代码 start ****************************************
		/* //yyc remove MSN 2010-11-05
		if(!m_msnbot.ifSigned() && --dwCount_msnbot==0 )
		{
			if(!m_msnbot.signin()){
				dwCount_msnbot=60; //下一次尝试在一分钟后进行
				int err=m_msnbot.getLastErrorCode();
				//登录failure，可能由于网络原因造成的
				if(err==SOCKSERR_OK) NULL;//没有配置MSNaccount
				else if( err==SOCKSERR_MSN_EMAIL) //登录failure，invalidaccount或password
					RW_LOG_PRINT(LOGLEVEL_ERROR,0,"Can not signin MSN server,invalid account or password.\r\n");
				else
					RW_LOG_PRINT(LOGLEVEL_WARN,0,"program will signin 1 minutes later.\r\n");
			}else dwCount_msnbot=1;
		}//?if(!m_msnbot.ifSigned()) */ //yyc remove MSN 2010-11-05

		//如果设置了自动重连远端vIDCs，则检测并自动重连
		if(--dwCount_vidcc==0){ dwCount_vidcc=10; m_vidcManager.m_vidccSets.autoConnect();}
//*********************用户其他代码  end  ****************************************
		if(m_tasklist.size()>0)
		{//handle定时执行任务列表
			time_t tNow=time(NULL);
			struct tm * ltime=localtime(&tNow);
			for(int i=0;i<(int)m_tasklist.size();i++)
			{
				TaskTimer &task=m_tasklist[i];
				std::string sTask=task.strTask;
				if(task.type=='t' && task.h>0)//specified间隔执行
				{	
					if(task.flag==0) task.flag=(long)tStartTime;
					if( (tNow-task.flag)>=task.h )
						task.flag=(long)tNow,parseCommand(sTask.c_str());
				}else if(task.type=='d'){ //每天定时执行
					if(task.h==ltime->tm_hour && task.m==ltime->tm_min)
					{	
						if(task.flag==0) //是否可执行此定时任务
							task.flag=1,parseCommand(sTask.c_str());
					}else task.flag=0; //可以执行此定时任务
				}
			}//?for(int i=0;i<m_tasklist.size();i++)
		}//?if(m_tasklist.size()>0)
	}//while
	
	if(RW_LOG_CHECK(LOGLEVEL_INFO)) RW_LOG_PRINTTIME(); //打印end运行time
	RW_LOG_PRINT(LOGLEVEL_INFO,0,"Stopping program,please waiting...\r\n");
	RW_LOG_FFLUSH();
//*********************用户其他代码 start ****************************************
	m_ftpsvr.saveIni();   //保存ftp服务的配置info
	m_proxysvr.saveIni();
	m_vidcManager.saveIni(); //保存配置vIDC的info
	m_vidcManager.Destroy(); //停止vidc相关服务并销毁释放相关资源
	m_ftpsvr.Stop(); //停止FTP服务
	m_proxysvr.Stop();
	m_websvr.Stop();
	m_telsvr.Stop();
//	m_msnbot.signout();//退出登录  //yyc remove MSN 2010-11-05
//*********************用户其他代码  end  ****************************************

	if(RW_LOG_CHECK(LOGLEVEL_INFO)) RW_LOG_PRINTTIME(); //打印end运行time
	RW_LOG_PRINT(LOGLEVEL_INFO,0,"program end!\r\n");
	//如果设置了password保护，退出时检测server是否安装正常
	if(m_hStopEvent)
	{
		QUERY_SERVICE_CONFIG sc;
		if( !GetServiceConfig(&sc) ) InstallService(); //服务可能被delete了，重新安装
		else if(sc.dwStartType!=m_dwStartType || sc.dwServiceType!=m_dwServiceType) 
			SetServiceConfig();//服务启动status被modify了
	}//?if(m_hStopEvent)
	//服务已经end------------------------------
	if(m_hStop) ::CloseHandle(m_hStop); //服务/程序退出
	if(m_hStopEvent) ::CloseHandle(m_hStopEvent); //服务/程序退出
	ReportStatus(SERVICE_STOPPED,3000,0); 
}

//往registry读/写service name称
void delreg(const char *svrkey)
{
	HKEY  hKEY;
	LPCTSTR lpRegPath = "Software\\Microsoft\\Windows\\CurrentVersion";
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegPath, 0, KEY_WRITE|KEY_READ, &hKEY)==ERROR_SUCCESS)
	{
		::RegDeleteValue(hKEY,svrkey);
		::RegCloseKey(hKEY);
	}
	return;
}
bool regsvrname(const char *svrkey, std::string &svrname,bool bWrite)
{
	HKEY  hKEY; bool bret=false;
	LPCTSTR lpRegPath = "Software\\Microsoft\\Windows\\CurrentVersion";
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegPath, 0, KEY_WRITE|KEY_READ, &hKEY)==ERROR_SUCCESS){
		if(bWrite){
				if( ::RegSetValueEx(hKEY, svrkey, NULL,REG_SZ, (LPBYTE)svrname.c_str(),svrname.length()+1)== ERROR_SUCCESS) bret=true;
		}
		else{
			char buf[64]; DWORD retLen=64;
			DWORD dwType=REG_SZ;
			if(::RegQueryValueEx(hKEY, svrkey, NULL,&dwType,(unsigned char *)buf,&retLen)==ERROR_SUCCESS){
				buf[retLen]=0; 
				svrname.assign(buf); bret=true;
			}
		}
		::RegCloseKey(hKEY);
	}
	if(!bret) svrname=(bWrite)?"":svrkey;
	return bret;
}
bool getDefaultCfgfile(std::string &cfgfile)
{
	char buf[MAX_PATH];
	DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH);
	buf[dwret]=0;
	const char *ptr=strrchr(buf,'.');
	cfgfile.assign(buf,ptr-buf);
	cfgfile.append(".ini");
	return true;
}
void getDefaultSvrname(std::string &svrname)
{
	char buf[MAX_PATH];
	DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH);
	buf[dwret]=0;
	const char *ptr=strrchr(buf,'\\');
	if(ptr) svrname.assign(ptr+1); else svrname.assign(buf);
	if( (ptr=strrchr(svrname.c_str(),'.')) )
		svrname.erase(ptr-svrname.c_str());
	if(svrname=="") svrname.assign(sServiceName);
	return;
}
//将一个相对path名转换为一个绝对path名
void getAbsolutfilepath(std::string &spath)
{
	if(spath!="" && spath[1]==':') return; //本身是一个绝对path
	char buf[MAX_PATH];
	DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH);
	buf[dwret]=0; 
	const char *ptr=strrchr(buf,'\\');
	if(ptr==NULL) return;
	*(char *)(ptr+1)=0;
	spath.insert(0,buf); return;
}
//delete升级后的临时文件
void delUpdateTmpfile()
{
	char buf[MAX_PATH];
	DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH-6);
	strcpy(buf+dwret,".tmp"); buf[dwret+4]=0;
	::DeleteFile(buf);
}
//command-line arguments
//-i dsplatName serverDesc --- install service
//-u ---uninstall service
//-s --- start service
//-e --- stop service
//-d --- Debug方式运行
//-n serverName ---- specifiedinstall service的名字
//		如果不通过-nspecified安装的service name称，则default的service name称为可执行filename，否则为specified的name
//		specified了install servicename，程序会记录安装的service name到registry，可执行filename为key值
//-c saveasfile --- 将currentini配置参数写入exe文件本身并另存为specified的文件
//-C saveasfile --- 将currentini配置参数写入exe文件本身并另存为specified的文件,
//					同时运行另存的exe程序将not supportedini配置文件
//-f filename specified本程序的配置filename,if not specified, defaults to a file with the same name as the program but with .ini extension
int main(int argc, char* argv[])
{
	std::string saveasfile;
	std::string svrname,svrkey;
	getDefaultSvrname(svrkey);
	char *ptr_stoppswd=NULL; //stop servicepassword
	for(int i=1;i<argc;i++){
		if(strcmp(argv[i],"-n")==0){
			if(i<(argc-1) && argv[i+1][0]!='-'){
				svrname.assign(argv[i+1]); 
				regsvrname(svrkey.c_str(),svrname,true); 
			}
		}else if(strcmp(argv[i],"-f")==0){
			if(i<(argc-1) && argv[i+1][0]!='-') g_cfgfile.assign(argv[i+1]);
		}else if(strcmp(argv[i],"-u")==0)
			delreg(svrkey.c_str());
		else if(strcmp(argv[i],"-e")==0)
		{
			if(i<(argc-1) && argv[i+1][0]!='-') ptr_stoppswd=argv[i+1];
		}else if(strcasecmp(argv[i],"-c")==0)
		{
			if(i<(argc-1) && argv[i+1][0]!='-')
			{
				if(argv[i][1]=='C') saveasfile.assign("*");
				saveasfile.append(argv[i+1]);
			}else printf("Error param : -c/C new_filename\r\n");
		}
		//*********************用户其他代码 start ****************************************
		//*********************用户其他代码  end  ****************************************
	}//?for(int i=1;i<argc;i++){
	if(svrname=="") regsvrname(svrkey.c_str(),svrname,false);
	//配置filename，default为exe同filename的ini文件
	if(g_cfgfile=="") 
		getDefaultCfgfile(g_cfgfile);
	else //如果用户输入的是相对path则在以服务方式启动时可能找不到配置文件
		getAbsolutfilepath(g_cfgfile);
	//将参数写入，另存为新的exe 
	if(saveasfile!=""){ saveAsExe(saveasfile); return 0; }
	delUpdateTmpfile();//delete升级后的临时文件
	MyService serv(svrname.c_str(),sServiceDesc);
	//输入了stop servicepassword,尝试此passwordstop service
	if(ptr_stoppswd) serv.SetStopEvent(ptr_stoppswd);
	serv.RegisterService(argc, argv);
	return 0;
}
