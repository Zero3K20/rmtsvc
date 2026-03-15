/*******************************************************************
   *	ftpserver.h 
   *    DESCRIPTION: FTP service configuration save and load
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *******************************************************************/
#include "rmtsvc.h" 

bool ftpsvrEx :: readIni()
{
	HKEY  hKEY;
	LPCTSTR lpRegPath = "Software\\Microsoft\\Windows\\CurrentVersion";
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegPath, 0, KEY_WRITE|KEY_READ, &hKEY)!=ERROR_SUCCESS)
		return false;
	DWORD dwType=REG_SZ,dwSize=0;
	char *pbuffer=NULL;
	if(::RegQueryValueEx(hKEY, "ftpsets", NULL,&dwType,NULL,&dwSize)==ERROR_SUCCESS)
	{
		if( (pbuffer=new char[dwSize]) )
			::RegQueryValueEx(hKEY, "ftpsets", NULL,&dwType,(LPBYTE)pbuffer,&dwSize);
	}
	::RegCloseKey(hKEY);
	if(pbuffer==NULL) return false;
	dwSize=cCoder::base64_decode(pbuffer,dwSize-1,pbuffer);
	pbuffer[dwSize]=0;
	parseIni(pbuffer,dwSize);

	delete[] pbuffer; return true;
}

bool ftpsvrEx :: parseIni(char *pbuffer,long lsize)
{
	if(pbuffer==NULL) return false;
	if(lsize<=0) lsize=strlen(pbuffer);
	if(strncasecmp(pbuffer,"base64",6)==0)
		lsize=cCoder::base64_decode(pbuffer+6,lsize-6,pbuffer);
//	RW_LOG_DEBUG("read FTP param.........\r\n%s\r\n",pbuffer);
	//start parsing Ini file
	const char *pstart=pbuffer;
	const char *ptr=strchr(pstart,'\r');
	while(true){
		if(ptr) *(char *)ptr=0;
		if(pstart[0]!='!') //skip comment lines
		{ 
			if(strncasecmp(pstart,"sets ",5)==0) //set service information
				docmd_sets(pstart+5);
			else if(strncasecmp(pstart,"ssls ",5)==0) //set service SSL information
				docmd_ssls(pstart+5);
			else if(strncasecmp(pstart,"ftps ",5)==0) //set FTP service welcome message
				docmd_ftps(pstart+5);
			else if(strncasecmp(pstart,"iprules ",8)==0) //set IP filter rules for accessing this service
				docmd_iprules(pstart+8);
			else if(strncasecmp(pstart,"user ",5)==0) //add account
				docmd_user(pstart+5);
			else if(strncasecmp(pstart,"vpath ",6)==0) //add account的虚目录
				docmd_vpath(pstart+6);
		} 
		if(ptr==NULL) break;
		*(char *)ptr='\r'; pstart=ptr+1;
		if(*pstart=='\n') pstart++;
		ptr=strchr(pstart,'\r');
	}//?while
	return true;
}

bool ftpsvrEx :: saveAsstring(std::string &strini)
{
	char buf[256];
	int len=sprintf(buf,"sets autorun=%d port=%d bindip=%s maxuser=%d\r\n",
		(m_settings.autoStart)?1:0,m_settings.svrport,m_settings.bindip.c_str(),m_settings.maxUsers);
	strini.append(buf,len);
	
	len=sprintf(buf,"ssls enable=%s\r\n",(m_settings.ifSSLsvr)?"true":"false");
	strini.append(buf,len);

	len=sprintf(buf,"ftps dataport=%d:%d logevent=%d tips=\"",m_settings.dataportB,m_settings.dataportE,
		m_settings.logEvent);
	strini.append(buf,len);
	const char *ptrBegin=m_settings.tips.c_str();
	const char *ptr=strstr(ptrBegin,"\r\n");
	while(true)
	{
		if(ptr)
			strini.append(ptrBegin,ptr-ptrBegin);
		else{ strini.append(ptrBegin); break; }
		strini.append("\\n");
		ptrBegin=ptr+2;
		ptr=strstr(ptrBegin,"\r\n");
	}
	strini.append("\"\r\n");
	//-------------------------保存accountinfo------------------------
	std::map<std::string,TFTPUser>::iterator it=m_userlist.begin();
	for(;it!=m_userlist.end();it++)
	{
		TFTPUser &ftpuser=(*it).second;
		len=sprintf(buf,"user account=%s pswd=%s pswdmode=%d hiddenfile=%d maxlogin=%d forbid=%d",
			ftpuser.username.c_str(),ftpuser.userpwd.c_str(),ftpuser.pswdmode,ftpuser.disphidden,
			ftpuser.maxLoginusers,ftpuser.forbid);
		strini.append(buf,len);
		len=sprintf(buf," maxup=%d maxdw=%d maxupfile=%d maxdisksize=%d:%d",
			ftpuser.maxupratio,ftpuser.maxdwratio,ftpuser.maxupfilesize,ftpuser.maxdisksize,ftpuser.curdisksize);
		strini.append(buf,len);
		if(ftpuser.limitedTime!=0)
		{
			struct tm * ltime=localtime(&ftpuser.limitedTime);
			len=sprintf(buf," expired=%04d-%02d-%02d",(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday);
			strini.append(buf,len);
		}
		std::pair<std::string,long> &p=ftpuser.dirAccess["/"];
		len=sprintf(buf," access=%d root=\"%s\" desc=\"",p.second,p.first.c_str());
		strini.append(buf,len);
		strini+=ftpuser.userdesc; strini.append("\"\r\n");
		
		len=sprintf(buf,"iprules account=%s access=%d",ftpuser.username.c_str(),ftpuser.ipaccess);
		strini.append(buf,len);
		len=sprintf(buf," ipaddr=%s\r\n",ftpuser.ipRules.c_str());
		strini.append(buf,len);

		std::map<std::string,std::pair<std::string,long> >::iterator it1=ftpuser.dirAccess.begin();
		it1++; 
		for(;it1!=ftpuser.dirAccess.end();it1++)
		{
			std::pair<std::string,long> &p1=(*it1).second;
			len=sprintf(buf,"vpath account=%s vdir=%s access=%d",ftpuser.username.c_str(),(*it1).first.c_str(),p1.second);
			strini.append(buf,len);
			len=sprintf(buf," rdir=\"%s\"\r\n",p1.first.c_str());
			strini.append(buf,len);
		}
	}//?for(
//	RW_LOG_DEBUG("save FTP param.........\r\n%s\r\n",strini.c_str());
	return true;
}
bool ftpsvrEx :: saveIni()
{
	std::string strini;
	saveAsstring(strini);
	//write registry--------------------------------------
	HKEY  hKEY;
	LPCTSTR lpRegPath = "Software\\Microsoft\\Windows\\CurrentVersion";
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegPath, 0, KEY_WRITE|KEY_READ, &hKEY)==ERROR_SUCCESS)
	{
		long l=cCoder::Base64EncodeSize(strini.length());
		char *pbuf=new char[l];
		if(pbuf){
			l=cCoder::base64_encode ((char *)strini.c_str(),strini.length(),pbuf);
			::RegSetValueEx(hKEY, "ftpsets", NULL,REG_SZ, (LPBYTE)pbuf,l+1);
			delete[] pbuf;
		}
		::RegCloseKey(hKEY);
	}
	return true;
}

//set service information
//命令format: 
//	sets [autorun=0|1] [port=<服务port>] [bindip=<本服务绑定的local machineIP>] [maxuser=<最大并发connect数>]
//port=<服务port>    : 设置服务port，如果不设置则default为21
//bindip=<本服务绑定的local machineIP> : 设置本服务绑定的local machineIP，如果不设置则default绑定local machine所有IP
//maxuser=<最大并发connect数>   : 设置本服务的最大并发用户connect数，如果不设置此项则为不限制
void ftpsvrEx :: docmd_sets(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("autorun"))!=maps.end())
	{//设置服务的port
		if(atoi((*it).second.c_str())==1)
			m_settings.autoStart=true;
		else m_settings.autoStart=false;
	}
	if( (it=maps.find("port"))!=maps.end())
	{//设置服务的port
		m_settings.svrport=atoi((*it).second.c_str());
		if(m_settings.svrport<0) m_settings.svrport=0;
	}
	if( (it=maps.find("bindip"))!=maps.end())
	{//设置服务绑定IP
		m_settings.bindip=(*it).second;
	}
	if( (it=maps.find("maxuser"))!=maps.end())
	{//设置服务的最大并发数
		m_settings.maxUsers=atoi((*it).second.c_str());
		if(m_settings.maxUsers<0) m_settings.maxUsers=0;
	}
	return;
}
//set service SSL information
//命令format: 
//	ssls [enable=<true|false>] [cacert=<SSL证书文件>] [cakey=<SSL私钥文件>] [capwd=<私钥password>]
//enable=<true|false> : 指示服务是否为SSL加密服务，如果设置为true则为SSL加密服务，default为非SSL加密服务
//下面三项指示SSL加密协商的证书、私钥和私钥password，如果证书或私钥任意一个不写则用程序内置的证书和私钥进行SSL加密协商
//cacert=<SSL证书文件>: specifiedSSL加密协商的证书filename，证书为.pemformat的证书。可specified相对或绝对path
//cakey=<SSL私钥文件> : specifiedSSL加密协商的私钥filename，私钥为.pemformat的私钥。可specified相对或绝对path
//capwd=<私钥password>    : specified的私钥的password
void ftpsvrEx :: docmd_ssls(const char *strParam)
{
#ifdef _SURPPORT_OPENSSL_
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::string strCert,strKey,strPwd;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("cacert"))!=maps.end()) //设置SSL证书
		strCert=(*it).second;
	if( (it=maps.find("cakey"))!=maps.end()) //设置SSL证书私钥
		strKey=(*it).second;
	if( (it=maps.find("capwd"))!=maps.end()) //设置SSL证书私钥password
		strPwd=(*it).second;
	if(strCert!="" && strKey!="")
	{
		getAbsolutfilepath(strCert);
		getAbsolutfilepath(strKey);
		setCacert(strCert.c_str(),strKey.c_str(),strPwd.c_str(),false); //使用用户specified的证书
	}
	else setCacert(NULL,NULL,NULL,true); //使用内置的证书
	if( (it=maps.find("enable"))!=maps.end()) //设置SSL证书
		if(strcasecmp((*it).second.c_str(),"true")==0)
			m_settings.ifSSLsvr=true;
		else m_settings.ifSSLsvr=false;
#endif
	return;
}

//设置FTP服务的欢迎info和绑定data传输port
//命令format: 
//	ftps [dataport=<port起始:portend>] [tips="<欢迎词info>"] [logevent=LOGIN|LOGOUT|DELETE|RMD|UPLOAD|DWLOAD|SITE]
//dataport=<port起始:portend>: 设置本FTP服务的PASVdata传输port，如果不specified则由系统随机分配，否则在你specified的区间随机分配
//	[port起始:portend]范围为闭区间，port为0则代表不限，例如[500,0]代表分配的port要>=500,[0,600]代表分配的port要<=600
//tips="<欢迎词info>" : specifiedftpclientconnect到服务后显示的欢迎info，因为欢迎info可能包含空格，因此最好用""括起
//	可以写多行欢迎info，每行的结尾必须以"\n"字符串end。每行的起始用"220-"开头
//logevent=LOGIN|LOGOUT|DELETE|RMD|UPLOAD|DWLOAD|SITE : specified记录哪些INFO事件
//		    当loglevel记录log level等于DEBUG或INFO时specified要记录哪些事件，default记录LOGIN,LOGOUT,SITE扩展命令事件
//		    如要specified记录多个事件可用|connect。例如logevent=LOGIN|LOGOUT|DELETE
//		    LOGIN事件  -- 记录某个user logintime和IP
//		    LOGOUT事件 -- 记录某个用户离开time和IP
//		    DELETE事件 -- 记录某个用户delete file
//		    RMD事件    -- 记录某个用户delete directory
//		    UPLOAD事件 -- 记录某个用户上载文件
//		    DWLOAD事件 -- 记录某个用户download file
//		    SITE事件   -- 记录某个用户执行FTP扩展SITE命令(不包含SITE INFO)以及命令执行结果
//范例: ftps tips="220-欢迎访问XXXftp服务\n220-test line.\n"
void ftpsvrEx :: docmd_ftps(const char *strParam)
{
	if(strParam==NULL) return;
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("dataport"))!=maps.end())
	{//设置服务的data传输port范围
		m_settings.dataportB=0;
		m_settings.dataportE=0;
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr){
			*(char *)ptr=0;
			m_settings.dataportB=atoi((*it).second.c_str());
			*(char *)ptr=':';
			m_settings.dataportE=atoi(ptr+1);
		}
		else m_settings.dataportB=m_settings.dataportE=atoi((*it).second.c_str());
	}
	if( (it=maps.find("logevent"))!=maps.end())
	{//设置要output log的事件
		const char *ptr=(*it).second.c_str();
		m_settings.logEvent=atoi(ptr);
		if(strstr(ptr,"LOGIN")) m_settings.logEvent|=FTP_LOGEVENT_LOGIN;
		if(strstr(ptr,"LOGOUT")) m_settings.logEvent|=FTP_LOGEVENT_LOGOUT;
		if(strstr(ptr,"DELETE")) m_settings.logEvent|=FTP_LOGEVENT_DELETE;
		if(strstr(ptr,"RMD")) m_settings.logEvent|=FTP_LOGEVENT_RMD;
		if(strstr(ptr,"UPLOAD")) m_settings.logEvent|=FTP_LOGEVENT_UPLOAD;
		if(strstr(ptr,"DWLOAD")) m_settings.logEvent|=FTP_LOGEVENT_DWLOAD;
		if(strstr(ptr,"SITE")) m_settings.logEvent|=FTP_LOGEVENT_SITE;
	}
	if( (it=maps.find("tips"))!=maps.end())
	{//将"\n"替换为\r\n
		const char *ptr,*ptrStart=(*it).second.c_str();
		while( (ptr=strstr(ptrStart,"\\n")) )
		{
			*(char *)ptr='\r';
			*(char *)(ptr+1)='\n';
			ptrStart=ptr+2;
		}
		m_settings.tips=(*it).second;
	}	
	return;
}

//添加新的ftpaccountinfo,如果account已存在则delete旧的，重新添加
//命令format: 
//	user account=<ftpaccount> pswd=<account password> root=<主目录> [pswdmode=<OTP_NONE|OTP_MD4|OTP_MD5>] [access=<对主目录的访问permissions>] [hiddenfile=HIDE] [maxlogin=<限制同时最多登录人数>] [expired=<account有效期限>] [maxup=<最大上载流量>] [maxdw=<最大download流量>] [maxupfile=<max upload file size>] [maxdisksize=<最大可使用磁盘空间>[:<current已使用磁盘空间>]]
//account=<ftpaccount> : 必须项. 要添加的ftpaccount。
//pswd=<account password>   : 必须项. specifiedftpaccount的password，
//					如果account password等于""或空则意味着无需password访问，只要account名对就可访问
//root=<主目录>     : 必须项. specified此ftpaccount的根/对应的主目录。主目录是运行ftp服务local机的实际绝对path
//					如果主目录中包含空格则要用""将主目录括起
//					如果主目录等于""或空则意味着此account可访问计算机所有磁盘目录
//pswdmode=<OTP_NONE|OTP_MD4|OTP_MD5> : password保护模式。OTP_NONE:明文传输password 
//					OTP_MD4: otp S/Key md4加密password传输. ftpclient的password保护应设置为md4或otp自动检测模式
//					OTP_MD4: otp S/Key md5加密password传输. ftpclient的password保护应设置为md5或otp自动检测模式
//					如果不设置则default是OTP_NONE,即password明文传输。
//access=<对主目录的访问permissions> : specified此ftpaccount的主目录的访问permissions。
//						如果不设置则default具有ACCESS_ALL访问permissions。设置format和含义如下
//		<对主目录的访问permissions> : <FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL|DIR_NOINHERIT>
//		ACCESS_ALL=FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL
//		FILE_READ : 允许读文件 FILE_WRITE : 允许写文件 FILE_DEL : 允许delete file FILE_EXEC : 允许执行文件
//		DIR_LIST : 允许目录文件list DIR_MAKE : 允许create directory DIR_DEL : 允许delete directory
//		DIR_NOINHERIT : 是否允许此虚目录对应的真实path下的子目录继承用户specified的目录访问permissions。如果用户specified了此项则禁止继承，
//		    那么defaultftp用户是无法对其子目录进行任何的读写操作，要想对其下的子目录具有操作，必须将其下的子目录映射为虚目录单独specified访问permissions
//		注意:如果设定了FILE_EXEC ，则用户可通过扩展FTP命令EXEC,remote执行此虚目录下的文件。
//hiddenfile=HIDE   : 不显示隐藏文件。如果不设置此项default为显示隐藏文件/目录
//maxlogin=<限制同时最多登录人数> : 限制此ftpaccount的同时登录访问人数	。
//					如果不设置则default为0，即不限制此account的同时登录人数
//expired=<account有效期限> : 限制此account的使用期限，formatYYYY-MM-DD
//					如果不设置则account永不过期
//maxup=<最大上载流量> : 限制此ftpaccount的最大上载流量Kb/秒
//					如果不设置则default为0，即不限制此account的最大上载流量
//maxdw=<最大download流量> : 限制此ftpaccount的最大download流量Kb/秒
//					如果不设置则default为0，即不限制此account的最大download流量
//maxupfile=<max upload file size> : 限制此ftpaccount的最大上载文件的size KBytes
//					如果不设置则default为0，即不限制此account的max upload file size
//maxdisksize=<最大可使用磁盘空间>[:<current已使用磁盘空间>] : 限制此account最大可使用的磁盘空间KBytes
//				<最大可使用磁盘空间>: KBytes,设置此account最大可使用的磁盘空间。如果不设置则 default为0即不限制
//				[:<current已使用磁盘空间>] :KBytes,设置current已使用的磁盘空间。如果不设置则default为0，即没有使用
void ftpsvrEx :: docmd_user(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	TFTPUser ftpuser;
	
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("account"))!=maps.end() )
		ftpuser.username=(*it).second;
	if(ftpuser.username=="") return;
	if( (it=maps.find("pswd"))!=maps.end())
		ftpuser.userpwd=(*it).second;
	if( (it=maps.find("desc"))!=maps.end())
		ftpuser.userdesc=(*it).second;

	std::pair<std::string,long> p("",0);
	if( (it=maps.find("root"))!=maps.end())
		p.first=(*it).second;
	if( (it=maps.find("access"))!=maps.end() && (*it).second!="")
	{
		const char *ptr=(*it).second.c_str();
		long lAccess=atoi(ptr);
		if(strstr(ptr,"FILE_READ")) lAccess|=FTP_ACCESS_FILE_READ;
		if(strstr(ptr,"FILE_WRITE")) lAccess|=FTP_ACCESS_FILE_WRITE;
		if(strstr(ptr,"FILE_DEL")) lAccess|=FTP_ACCESS_FILE_DELETE;
		if(strstr(ptr,"FILE_EXEC")) lAccess|=FTP_ACCESS_FILE_EXEC;
		if(strstr(ptr,"DIR_LIST")) lAccess|=FTP_ACCESS_DIR_LIST;
		if(strstr(ptr,"DIR_MAKE")) lAccess|=FTP_ACCESS_DIR_CREATE;
		if(strstr(ptr,"DIR_DEL")) lAccess|=FTP_ACCESS_DIR_DELETE;
		if(strstr(ptr,"DIR_NOINHERIT")) lAccess|=FTP_ACCESS_SUBDIR_INHERIT;
		if(strstr(ptr,"ACCESS_ALL")) lAccess=FTP_ACCESS_ALL;
		p.second=lAccess;
	}
	ftpuser.dirAccess["/"]=p;

	if( (it=maps.find("maxup"))!=maps.end())
	{
		ftpuser.maxupratio=atol((*it).second.c_str());
		if(ftpuser.maxupratio<0) ftpuser.maxupratio=0;
	}else ftpuser.maxupratio=0;
	
	if( (it=maps.find("maxdw"))!=maps.end())
	{
		ftpuser.maxdwratio=atol((*it).second.c_str());
		if(ftpuser.maxdwratio<0) ftpuser.maxdwratio=0;
	}else ftpuser.maxdwratio=0;

	if( (it=maps.find("maxupfile"))!=maps.end())
	{
		ftpuser.maxupfilesize=atol((*it).second.c_str());
		if(ftpuser.maxupfilesize<0) ftpuser.maxupfilesize=0;
	}else ftpuser.maxupfilesize=0;
	
	if( (it=maps.find("maxlogin"))!=maps.end())
	{
		ftpuser.maxLoginusers=atol((*it).second.c_str());
		if(ftpuser.maxLoginusers<0) ftpuser.maxLoginusers=0;
	}else ftpuser.maxLoginusers=0;

	if( (it=maps.find("expired"))!=maps.end())
	{
		struct tm ltm; ::memset((void *)&ltm,0,sizeof(ltm));
		::sscanf((*it).second.c_str(),"%d-%d-%d",&ltm.tm_year,&ltm.tm_mon,&ltm.tm_mday);
		ltm.tm_year-=1900; //年份从1900start计数
		ltm.tm_mon-=1;//月份从0start计数
		if(ltm.tm_year>100 && ltm.tm_year<200 && 
			ltm.tm_mon>=0 && ltm.tm_mon<=11 && 
		    ltm.tm_mday>=1 && ltm.tm_mday<=31 )
			ftpuser.limitedTime= mktime(&ltm);
	}else ftpuser.limitedTime=0;

	if( (it=maps.find("maxdisksize"))!=maps.end())
	{
		long lsize=atol((*it).second.c_str());
		ftpuser.maxdisksize=(lsize<0)?0:lsize;
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr) ftpuser.curdisksize=atol(ptr+1);
		if(ftpuser.curdisksize<0) ftpuser.curdisksize=0;
	}else ftpuser.maxdisksize=ftpuser.curdisksize=0;
	
	if( (it=maps.find("hiddenfile"))!=maps.end())
		ftpuser.disphidden=((*it).second=="HIDE" || (*it).second=="0")?0:1;
	else ftpuser.disphidden=0;

	if( (it=maps.find("pswdmode"))!=maps.end())
	{
		if((*it).second=="OTP_MD4") 
			ftpuser.pswdmode=OTP_MD4;
		else if((*it).second=="OTP_MD5")
			ftpuser.pswdmode=OTP_MD5;
		else if((*it).second=="OTP_NONE")
			ftpuser.pswdmode=OTP_NONE;
		else ftpuser.pswdmode=atoi((*it).second.c_str());
	}else ftpuser.pswdmode=0;

	if( (it=maps.find("forbid"))!=maps.end())
	{
		if((*it).second=="true" || (*it).second=="1")
			ftpuser.forbid=1;
		else ftpuser.forbid=0;
	}else ftpuser.forbid=0;

	ftpuser.ipaccess=0;
	ftpuser.ipRules="";

	m_userlist[ftpuser.username]=ftpuser;
	return ;
}

//设置ftp服务的ip过滤规则或针对某个account的IP filter rules
//命令format:
//	iprules [account=<ftpaccount>] [access=0|1] ipaddr="<IP>,<IP>,..."
//account=<ftpaccount> : specified此过滤规则是针对某个account的还是针对所有account适用的
//						如果不设置此项则或ftpaccount设置为""或空则针对所有account适用
//access=0|1     : 对符合下列IP条件的是拒绝还是放行
//例如:
// iprules access=0 rules="192.168.0.*,192.168.1.10"
void ftpsvrEx :: docmd_iprules(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::string strUser;

	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("account"))!=maps.end())  strUser=(*it).second;

	std::map<std::string,TFTPUser>::iterator it1=m_userlist.find(strUser);
	if(it1==m_userlist.end()) return;
	TFTPUser &ftpuser=(*it1).second;
	
	if( (it=maps.find("access"))!=maps.end())
		ftpuser.ipaccess=atoi((*it).second.c_str());
	
	if( (it=maps.find("ipaddr"))!=maps.end())
		ftpuser.ipRules=(*it).second;

	return;
}

//添加modifydeleteftpaccount的虚目录
//命令format: 
//	vpath account=<ftpaccount> vdir=<虚目录> rdir=<真实目录> [access=<虚目录访问permissions>]
//account=<ftpaccount> : 必须项. 要设置虚目录的ftpaccount。
//vdir=<虚目录>     : 必须项. 要添加或modify的虚目录，每个虚目录必须以/start，例如/aa
//		    不能用此目录添加modifydelete虚根目录。注意:虚目录区分size写
//rdir=<真实目录>   : 此虚目录对应的真实目录path,必须是绝对path,
//		    如果<真实目录>中包含空格则要用""将<真实目录>括起,例如"c:\temp test\aa"
//		    如果没有设置rdir或<真实目录>等于空字符串例如rdir= 或rdir=""则意味着delete此虚path
//access=<虚目录访问permissions> : specified对此虚目录的访问permissions。如果不设置则default具有ACCESS_ALL访问permissions。设置format和含义如下
//		 <虚目录访问permissions> : <FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL|DIR_NOINHERIT>
//		 ACCESS_ALL=FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL
//		 FILE_READ : 允许读文件 FILE_WRITE : 允许写文件 FILE_DEL : 允许delete file FILE_EXEC : 允许执行文件
//		 DIR_LIST : 允许目录文件list DIR_MAKE : 允许create directory DIR_DEL : 允许delete directory
//		 DIR_NOINHERIT : 是否允许此虚目录对应的真实path下的子目录继承用户specified的目录访问permissions。如果用户specified了此项则禁止继承，
//		    那么defaultftp用户是无法对其子目录进行任何的读写操作，要想对其下的子目录具有操作，必须将其下的子目录映射为虚目录单独specified访问permissions
//		 注意:如果设定了FILE_EXEC ，则用户可通过扩展FTP命令EXEC,remote执行此虚目录下的文件。
void ftpsvrEx :: docmd_vpath(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	std::string strvdir;

	if( (it=maps.find("account"))!=maps.end())  strvdir=(*it).second;
	std::map<std::string,TFTPUser>::iterator it1=m_userlist.find(strvdir);
	if(it1==m_userlist.end()) return;
	TFTPUser &ftpuser=(*it1).second;

	std::pair<std::string,long> p("",0); strvdir="";
	if( (it=maps.find("vdir"))!=maps.end())  strvdir=(*it).second;
	if( strvdir=="") return;

	if( (it=maps.find("rdir"))!=maps.end()) p.first=(*it).second;
	if( (it=maps.find("access"))!=maps.end() && (*it).second!="")
	{
		const char *ptr=(*it).second.c_str();
		p.second=atol(ptr); 
		if(strstr(ptr,"FILE_READ")) p.second|=FTP_ACCESS_FILE_READ;
		if(strstr(ptr,"FILE_WRITE")) p.second|=FTP_ACCESS_FILE_WRITE;
		if(strstr(ptr,"FILE_DEL")) p.second|=FTP_ACCESS_FILE_DELETE;
		if(strstr(ptr,"FILE_EXEC")) p.second|=FTP_ACCESS_FILE_EXEC;
		if(strstr(ptr,"DIR_LIST")) p.second|=FTP_ACCESS_DIR_LIST;
		if(strstr(ptr,"DIR_MAKE")) p.second|=FTP_ACCESS_DIR_CREATE;
		if(strstr(ptr,"DIR_DEL")) p.second|=FTP_ACCESS_DIR_DELETE;
		if(strstr(ptr,"DIR_NOINHERIT")) p.second|=FTP_ACCESS_SUBDIR_INHERIT;
		if(strstr(ptr,"ACCESS_ALL")) p.second=FTP_ACCESS_ALL;
	}

	ftpuser.dirAccess[strvdir]=p;
	return;
}
