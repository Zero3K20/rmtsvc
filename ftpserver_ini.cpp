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
			else if(strncasecmp(pstart,"vpath ",6)==0) //add virtual directory to account
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
	//-------------------------save account info------------------------
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
//command format: 
//	sets [autorun=0|1] [port=<serviceport>] [bindip=<local machine IP for this service>] [maxuser=<max concurrent connections>]
//port=<service port>    : set service port, default is 21 if not specified
//bindip=<local machine IP for this service> : set the local machine IP to bind, default binds all IPs if not specified
//maxuser=<max concurrent connections> : set max concurrent users, no limit if not specified
void ftpsvrEx :: docmd_sets(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("autorun"))!=maps.end())
	{//set service port
		if(atoi((*it).second.c_str())==1)
			m_settings.autoStart=true;
		else m_settings.autoStart=false;
	}
	if( (it=maps.find("port"))!=maps.end())
	{//set service port
		m_settings.svrport=atoi((*it).second.c_str());
		if(m_settings.svrport<0) m_settings.svrport=0;
	}
	if( (it=maps.find("bindip"))!=maps.end())
	{//set service binding IP
		m_settings.bindip=(*it).second;
	}
	if( (it=maps.find("maxuser"))!=maps.end())
	{//set service max concurrent connections
		m_settings.maxUsers=atoi((*it).second.c_str());
		if(m_settings.maxUsers<0) m_settings.maxUsers=0;
	}
	return;
}
//set service SSL information
//command format: 
//	ssls [enable=<true|false>] [cacert=<SSL certificate file>] [cakey=<SSL private key file>] [capwd=<private key password>]
//enable=<true|false> : indicates whether the service is SSL-encrypted; true means SSL-encrypted, default is non-SSL
//the following three items specify the SSL negotiation certificate, private key and password; if either is omitted, the built-in certificate and private key are used
//cacert=<SSLcertificatefile>: specifies the SSL negotiation certificate filename (.pem format). Can be relative or absolute path
//cakey=<SSL private key file> : specifies the SSL negotiation private key filename (.pem format). Can be relative or absolute path
//capwd=<private key password>    : password for the specified private key
void ftpsvrEx :: docmd_ssls(const char *strParam)
{
#ifdef _SUPPORT_TLSCLIENT_
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::string strCert,strKey,strPwd;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("cacert"))!=maps.end()) //set SSL certificate
		strCert=(*it).second;
	if( (it=maps.find("cakey"))!=maps.end()) //set SSL certificate private key
		strKey=(*it).second;
	if( (it=maps.find("capwd"))!=maps.end()) //set SSL certificate private key password
		strPwd=(*it).second;
	if(strCert!="" && strKey!="")
	{
		getAbsoluteFilePath(strCert);
		getAbsoluteFilePath(strKey);
		setCacert(strCert.c_str(),strKey.c_str(),strPwd.c_str(),false); //use the user-specified certificate
	}
	else setCacert(NULL,NULL,NULL,true); //use the built-in certificate
	if( (it=maps.find("enable"))!=maps.end()) //set SSL certificate
		if(strcasecmp((*it).second.c_str(),"true")==0)
			m_settings.ifSSLsvr=true;
		else m_settings.ifSSLsvr=false;
#endif
	return;
}

//set FTP service welcome message and data transfer port
//command format: 
//	ftps [dataport=<start port:end port>] [tips="<welcome message>"] [logevent=LOGIN|LOGOUT|DELETE|RMD|UPLOAD|DWLOAD|SITE]
//dataport=<start port:end port>: set the PASV data transfer port for this FTP service; if not specified, assigned randomly by the system, otherwise within the specified range
//	[start port:end port] closed interval; port 0 means no limit. e.g. [500,0] means port>=500, [0,600] means port<=600
//tips="<welcome message>" : specifies the welcome message shown to FTP clients upon connection; use quotes if it contains spaces
//	multiple welcome lines can be written; each line must end with "\n" and start with "220-"
//logevent=LOGIN|LOGOUT|DELETE|RMD|UPLOAD|DWLOAD|SITE : specifies which INFO events to log
//		    when log level is DEBUG or INFO, specifies which events to log; default logs LOGIN, LOGOUT, SITE events
//		    to specify multiple events, use | to connect them. e.g. logevent=LOGIN|LOGOUT|DELETE
//		    LOGIN event  -- log a user's login time and IP
//		    LOGOUT event -- log a user's logout time and IP
//		    DELETEevent -- log when a user deletes a file
//		    RMDevent    -- log when a user deletes a directory
//		    UPLOADevent -- log when a user uploads a file
//		    DWLOADevent -- log when a user downloads a file
//		    SITEevent   -- log when a user executes an FTP SITE extension command (excluding SITE INFO) and the result
//example: ftps tips="220-Welcome to XXX FTP service\n220-test line.\n"
void ftpsvrEx :: docmd_ftps(const char *strParam)
{
	if(strParam==NULL) return;
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("dataport"))!=maps.end())
	{//set service data transfer port range
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
	{//set which events to output to log
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
	{//replace "\n" with \r\n
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

//add new FTP account info; if account already exists, delete and re-add it
//command format: 
//	user account=<ftpaccount> pswd=<account password> root=<home directory> [pswdmode=<OTP_NONE|OTP_MD4|OTP_MD5>] [access=<home directory access permissions>] [hiddenfile=HIDE] [maxlogin=<max simultaneous logins>] [expired=<account expiry date>] [maxup=<max upload bandwidth>] [maxdw=<max download bandwidth>] [maxupfile=<max upload file size>] [maxdisksize=<max usable disk space>[:<current used disk space>]]
//account=<ftpaccount> : required. The FTP account to add.
//pswd=<account password>   : required. The password for the FTP account.
//					if account password is empty, no password is required; just the correct account name suffices
//root=<home directory>     : required. Specifies the root/home directory for this FTP account. Must be an absolute path on the local machine running the FTP service
//					if the home directory contains spaces, enclose it in quotes
//					if the home directory is empty, the account can access all disk directories
//pswdmode=<OTP_NONE|OTP_MD4|OTP_MD5> : password protection mode. OTP_NONE: transmit password in plaintext 
//					OTP_MD4: OTP S/Key MD4 encrypted password transmission. FTP client password protection should be set to md4 or OTP auto-detect mode
//					OTP_MD4: OTP S/Key MD5 encrypted password transmission. FTP client password protection should be set to md5 or OTP auto-detect mode
//					if not set, default is OTP_NONE (plaintext password).
//access=<home directory access permissions> : specifies the access permissions for this FTP account's home directory.
//						if not set, default is ACCESS_ALL permissions. Format and meaning as follows
//		<home directory access permissions> : <FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL|DIR_NOINHERIT>
//		ACCESS_ALL=FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL
//		FILE_READ : allow read FILE_WRITE: allow write FILE_DEL: allow delete FILE_EXEC: allow execute
//		DIR_LIST : allow directory listing DIR_MAKE: allow create directory DIR_DEL: allow delete directory
//		DIR_NOINHERIT : whether to allow subdirectories under this virtual directory's real path to inherit the user-specified access permissions. If this option is set, inheritance is disabled.
//		    then by default the FTP user cannot read/write subdirectories; to grant access to subdirectories, map them as virtual directories with separate permissions
//		note: if FILE_EXEC is set, users can remotely execute files in this virtual directory via the extended FTP EXEC command.
//hiddenfile=HIDE   : do not show hidden files. Default is to show hidden files/directories if not set
//maxlogin=<max simultaneous logins> : limit the number of simultaneous logins for this FTP account.
//					if not set, default is 0 (unlimited simultaneous logins for this account)
//expired=<account expiry date> : limit this account's usage period, format YYYY-MM-DD
//					if not set, the account never expires
//maxup=<max upload bandwidth> : limit this FTP account's maximum upload speed in Kb/s
//					if not set, default is 0 (no upload speed limit for this account)
//maxdw=<max download bandwidth> : limit this FTP account's maximum download speed in Kb/s
//					if not set, default is 0 (no download speed limit for this account)
//maxupfile=<max upload file size> : limit this FTP account's maximum upload file size in KBytes
//					if not set, default is 0 (no upload file size limit for this account)
//maxdisksize=<max usable disk space>[:<current used disk space>] : limit this account's maximum usable disk space in KBytes
//				<max usable disk space>: KBytes, sets the maximum usable disk space for this account. Default is 0 (unlimited) if not set
//				[:<current used disk space>] :KBytes, sets the current used disk space. Default is 0 (none used) if not set
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
		ltm.tm_year-=1900; //year counts from 1900
		ltm.tm_mon-=1;//month counts from 0
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

//set IP filter rules for the FTP service or for a specific account
//command format:
//	iprules [account=<ftpaccount>] [access=0|1] ipaddr="<IP>,<IP>,..."
//account=<ftpaccount> : specifies whether this filter rule applies to a specific account or to all accounts
//						if not set or FTP account is empty, the rule applies to all accounts
//access=0|1     : whether to deny or allow IPs matching the following conditions
//example:
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

//add, modify or delete an FTP account's virtual directory
//command format: 
//	vpath account=<ftpaccount> vdir=<virtual directory> rdir=<real directory> [access=<virtual directory access permissions>]
//account=<ftpaccount> : required. The FTP account for which to set the virtual directory.
//vdir=<virtual directory>     : required. The virtual directory to add or modify; each must start with /, e.g. /aa
//		    cannot use this to add, modify or delete the virtual root directory. Note: virtual directories are case-sensitive
//rdir=<real directory>   : the real directory path corresponding to this virtual directory; must be an absolute path
//		    if <real directory> contains spaces, enclose it in "", e.g. "c:\temp test\aa"
//		    if rdir is not set or <real directory> equals empty string, e.g. rdir= or rdir="", it means delete this virtual path
//access=<virtual directory access permissions> : specifies permissions for this virtual directory. If not set, default is ACCESS_ALL permissions. Format and meaning as follows
//		 <virtual directory access permissions> : <FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL|DIR_NOINHERIT>
//		 ACCESS_ALL=FILE_READ|FILE_WRITE|FILE_DEL|FILE_EXEC|DIR_LIST|DIR_MAKE|DIR_DEL
//		 FILE_READ : allow read FILE_WRITE: allow write FILE_DEL: allow delete FILE_EXEC: allow execute
//		 DIR_LIST : allow directory listing DIR_MAKE: allow create directory DIR_DEL: allow delete directory
//		 DIR_NOINHERIT : whether to allow subdirectories under this virtual directory's real path to inherit the user-specified access permissions. If this option is set, inheritance is disabled.
//		    then by default the FTP user cannot read/write subdirectories; to grant access to subdirectories, map them as virtual directories with separate permissions
//		 note: if FILE_EXEC is set, users can remotely execute files in this virtual directory via the extended FTP EXEC command.
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
