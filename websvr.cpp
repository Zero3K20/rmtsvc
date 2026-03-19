/*******************************************************************
   *	websvr.cpp 
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/

#include "rmtsvc.h"
#include <mutex>

webServer :: webServer():m_svrport(7778)
{
	m_bPowerOff=false;
#ifdef _SUPPORT_TLSCLIENT_
		setCacert(NULL,NULL,NULL,true); //load built-in certificate by default
#endif
		setRoot(NULL,HTTP_ACCESS_NONE,NULL);
		
		m_dwImgSize=0;

		m_defaultPage="index.htm";

		m_bSSLenabled=false;
		m_bSSLverify=false;
		m_bAnonymous=true;
}


// Save all non-expired remember-me tokens to a file next to the executable
void webServer::saveRememberTokens()
{
	std::string tokenfile("rmtsvc_tokens.dat");
	getAbsoluteFilePath(tokenfile);
	FILE *fp=::fopen(tokenfile.c_str(),"w");
	if(fp==NULL) return;
	time_t now=time(NULL);
	std::lock_guard<cMutex> lk(m_rememberMutex);
	for(std::map<std::string,RememberEntry>::iterator it=m_rememberTokens.begin();
		it!=m_rememberTokens.end(); ++it)
	{
		if(it->second.expires>now)
			::fprintf(fp,"%s %s %ld %lld\n",
				it->first.c_str(),
				it->second.user.c_str(),
				it->second.lAccess,
				(long long)it->second.expires);
	}
	::fclose(fp);
}

// Load remember-me tokens from file, discarding any that have expired
void webServer::loadRememberTokens()
{
	std::string tokenfile("rmtsvc_tokens.dat");
	getAbsoluteFilePath(tokenfile);
	FILE *fp=::fopen(tokenfile.c_str(),"r");
	if(fp==NULL) return;
	char token[64], user[256];
	long lAccess;
	long long expires;
	time_t now=time(NULL);
	std::lock_guard<cMutex> lk(m_rememberMutex);
	while(::fscanf(fp,"%63s %255s %ld %lld",token,user,&lAccess,&expires)==4)
	{
		if((time_t)expires>now){
			RememberEntry entry;
			entry.user.assign(user);
			entry.lAccess=lAccess;
			entry.expires=(time_t)expires;
			m_rememberTokens[std::string(token)]=entry;
		}
	}
	::fclose(fp);
}

//start service
bool webServer :: Start() 
{
	if(m_svrport==0) return true; //do not start service
	loadRememberTokens(); //restore persisted remember-me tokens
#ifdef _SUPPORT_TLSCLIENT_
	if(m_bSSLenabled) //start SSL service
	{
		if(g_strMyCert=="" || g_strMyKey=="")
			setCacert(NULL,NULL,NULL,true,NULL,NULL); //use built-in certificate
		else if(m_bSSLverify)
			setCacert(g_strMyCert.c_str(),g_strMyKey.c_str(),g_strKeyPswd.c_str(),false,
			g_strCaCert.c_str(),g_strCaCRL.c_str()); //use user-specified certificate and CRL
		else setCacert(g_strMyCert.c_str(),g_strMyKey.c_str(),g_strKeyPswd.c_str(),false,NULL,NULL);
		this->initSSL(true,NULL);
	}
#endif
	
	const char *ip=(m_bindip=="")?NULL:m_bindip.c_str();
	BOOL bReuseAddr=(ip)?SO_REUSEADDR:FALSE;//allow port reuse if an IP is bound
	SOCKSRESULT sr=Listen( ((m_svrport<0)?0:m_svrport) ,bReuseAddr,ip);
	return (sr>0)?true:false;
}

void webServer :: Stop()
{ 
	Close();
#ifdef _SUPPORT_TLSCLIENT_
	freeSSL();
#endif
	return;
}

void webServer :: setRoot(const char *rpath,long lAccess,const char *defaultPage)
{
	std::string spath; if(rpath) spath.assign(rpath);
	if(spath!="/"){
		if(spath.empty()) spath.assign("html");
		getAbsoluteFilePath(spath);
		if(spath[spath.length()-1]!='\\') spath.append("\\");
	}else spath="";
	this->setvpath("/",spath.c_str(),lAccess);
	if(defaultPage && defaultPage[0]!=0) m_defaultPage.assign(defaultPage);
	return;
}

//If onHttpReq returns true, this request has been handled; base class httpsvr does not need to process it further
//Otherwise hand off to base class httpsvr to continue processing
bool webServer :: onHttpReq(socketTCP *psock,httpRequest &httpreq,httpSession &session,
			std::map<std::string,std::string>& application,httpResponse &httprsp)
{
	if(m_bPowerOff && strcasecmp(httpreq.url().c_str(),"/command")==0)
	{//no permission required; anyone can remotely execute shutdown/restart
		const char *ptr_cmd=httpreq.Request("cmd");
		httprsp_command(psock,httprsp,ptr_cmd);
		return true;
	}
	if(strcasecmp(httpreq.url().c_str(),"/checkcode")==0)
	{//generate numeric verification code
		httprsp_checkcode(psock,httprsp,session);
		return true;
	}
	if(httpreq.url()=="/login") //user-submitted authentication request
	{
		if(httprsp_login(psock,httpreq,httprsp,session)) return true;
	}//?if(httpreq.url()=="/login")
	//download file directly via a specially constructed URL; download tools can be used //yyc add 2007-11-06
	if(strncasecmp(httpreq.url().c_str(),"/dwfiles/",9)==0)
	{
		const char *ptr=strchr(httpreq.url().c_str()+9,'/');
		if(ptr==NULL) return true; else *(char *)ptr=0;
		httpSession *psession=this->GetSession(httpreq.url().c_str()+9);
		if(psession==NULL) return true; else ptr++;//ptr points to filename
		long lqx=atol( (*psession)["lAccess"].c_str() );
		if((lqx & RMTSVC_ACCESS_FILE_VIEW)==0) return true;
		
		const char *ptr_path=httpreq.Request("path");
		if(ptr_path==NULL) return true;
		string filepath(ptr_path);
		if(filepath[filepath.length()-1]!='\\') filepath.append("\\");
		filepath.append(ptr);

		long lstartpos,lendpos;//get file range
		int iRangeNums=httpreq.get_requestRange(&lstartpos,&lendpos,0);
		if(iRangeNums>1){
			long *lppos=new long[iRangeNums*2];
			if(lppos==NULL) return true;
			for(int i=0;i<iRangeNums;i++) httpreq.get_requestRange(&lppos[i],&lppos[iRangeNums+i],i);
			httprsp.sendfile(psock,filepath.c_str(),MIMETYPE_UNKNOWED,&lppos[0],&lppos[iRangeNums],iRangeNums);
			delete[] lppos; return true;
		}else if(iRangeNums==0) { lstartpos=0; lendpos=-1; }
		httprsp.sendfile(psock,filepath.c_str(),MIMETYPE_UNKNOWED,lstartpos,lendpos);
		return true;
	}
	
	//check whether the user is authenticated and get their permissions
	long lAccess=atol(session["lAccess"].c_str());
	if(lAccess==RMTSVC_ACCESS_NONE){
		const char *rememberToken=httpreq.Cookies("rmtsvc_remember");
		if(rememberToken && !m_bAnonymous){
			bool bExpired=false;
			{
				std::lock_guard<cMutex> lk(m_rememberMutex);
				std::map<std::string,RememberEntry>::iterator it=m_rememberTokens.find(rememberToken);
				if(it!=m_rememberTokens.end()){
					if(time(NULL)<(*it).second.expires){
						session["user"]=(*it).second.user;
						char tmp[16]; sprintf(tmp,"%ld",(*it).second.lAccess);
						session["lAccess"]=std::string(tmp);
						lAccess=(*it).second.lAccess;
					}else{
						m_rememberTokens.erase(it); //clean up expired token
						bExpired=true;
					}
				}
			}
			if(bExpired) saveRememberTokens();
		}
	}
	if(lAccess==RMTSVC_ACCESS_NONE){
		if(m_bAnonymous){//if currently in anonymous login mode
			session["user"]=string("Anonymous");
			session["lAccess"]=string("-1");
		}else if(strcasecmp(httpreq.url().c_str(),"/login.htm")!=0)
		{//if no access permission and the current URL is not the login page, redirect to the login page
			this->httprsp_Redirect(psock,httprsp,"/login.htm"); 
			return true;
		}
	}//no access permission
	
	if(httpreq.url()=="/"){ //redirect to default page
		httpreq.url()+=m_defaultPage;
		this->httprsp_Redirect(psock,httprsp,httpreq.url().c_str());
		return true;
	}
//----------------------------------------------------------------------------
//-----------------------URL handling---------------------------------------------
	if(strcasecmp(httpreq.url().c_str(),"/capSetting")==0)
	{
		httprsp_capSetting(psock,httpreq,httprsp,session,((lAccess & RMTSVC_ACCESS_SCREEN_ALL)!=0) );
		return true;
	}
	else if(strcasecmp(httpreq.url().c_str(),"/version")==0)
	{
		httprsp_version(psock,httprsp);
		return true;
	}
	else if(strcasecmp(httpreq.url().c_str(),"/cmdpage")==0)
	{
		const char *ptr_cmd=httpreq.Request("cmd");
		httprsp_cmdpage(psock,httprsp,ptr_cmd);
		return true;
	}
	//if the user has screen view permission
	if((lAccess & RMTSVC_ACCESS_SCREEN_ALL)!=0)
	{
		if(strcasecmp(httpreq.url().c_str(),"/capWindow")==0)
		{//set whether to capture only the specified window
			httprsp_capWindow(psock,httpreq,httprsp,session);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/getPassword")==0)
		{
			httprsp_getpswdfromwnd(psock,httpreq,httprsp,session);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/capDesktop")==0)
		{
			httprsp_capDesktop(psock,httprsp,session);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/capStream")==0)
		{
			httprsp_capStream(psock,httprsp,session);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/getCursor")==0)
		{
			httprsp_getCursor(psock,httprsp);
			return true;
		}
		else if((lAccess & RMTSVC_ACCESS_SCREEN_ALL)==RMTSVC_ACCESS_SCREEN_ALL)
		{//if the user has full control permission
			if(strcasecmp(httpreq.url().c_str(),"/capAudio")==0)
			{
				httprsp_capAudio(psock,httprsp);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/getclipboard")==0)
			{
				httprsp_GetClipBoard(psock,httprsp);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/setclipboard")==0)
			{
				const char *ptr_val=httpreq.Request("val");
				httprsp_SetClipBoard(psock,httprsp,ptr_val);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/msevent")==0)
			{
				httprsp_msevent(psock,httpreq,httprsp,session);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/keyevent")==0)
			{
				httprsp_keyevent(psock,httpreq,httprsp);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/command")==0)
			{
				const char *ptr_cmd=httpreq.Request("cmd");
				httprsp_command(psock,httprsp,ptr_cmd);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/sysinfo")==0)
			{
				httprsp_sysinfo(psock,httprsp);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/usageimage")==0)
			{
				httprsp_usageimage(psock,httprsp);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/fport")==0)
			{
				httprsp_fport(psock,httprsp);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/plist")==0)
			{
				httprsp_plist(psock,httprsp);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/pkill")==0)
			{
				const char *strpid=httpreq.Request("pid");
				DWORD pid=(strpid)?((DWORD)atol(strpid)):0;
				httprsp_pkill(psock,httprsp,pid);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/mlist")==0)
			{
				const char *strpid=httpreq.Request("pid");
				DWORD pid=(strpid)?((DWORD)atol(strpid)):0;
				httprsp_mlist(psock,httprsp,pid);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/mdattach")==0)
			{
				const char *ptr=httpreq.Request("pid");
				DWORD pid=(ptr)?((DWORD)atol(ptr)):0;
				ptr=httpreq.Request("hmdl");
				HMODULE hmdl=(ptr)?((HMODULE)atol(ptr)):0;
				ptr=httpreq.Request("count");
				long lCount=(ptr)?atol(ptr):1;
				if(pid && hmdl) httprsp_mdattach(psock,httprsp,pid,hmdl,lCount);
				return true;
			}
		}
	}//?if((lAccess & RMTSVC_ACCESS_SCREEN_ALL)!=0)
	//if the user has registry management permission
	if((lAccess & RMTSVC_ACCESS_REGIST_ALL)!=0)
	{
		if(strcasecmp(httpreq.url().c_str(),"/reglist")==0)
		{
			int listWhat=1;
			const char *ptr_key=httpreq.Request("listwhat");
			if(ptr_key) listWhat=atoi(ptr_key);
			ptr_key=httpreq.Request("rkey");
			httprsp_reglist(psock,httprsp,ptr_key,listWhat);
			return true;
		}
		else if( (lAccess & RMTSVC_ACCESS_REGIST_ALL)==RMTSVC_ACCESS_REGIST_ALL)
		{
			if(strcasecmp(httpreq.url().c_str(),"/regkey_del")==0)
			{
				const char *ptr_path=httpreq.Request("rpath");
				const char *ptr_key=httpreq.Request("rkey");
				httprsp_regkey_del(psock,httprsp,ptr_path,ptr_key);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/regkey_add")==0)
			{
				const char *ptr_path=httpreq.Request("rpath");
				const char *ptr_key=httpreq.Request("rkey");
				httprsp_regkey_add(psock,httprsp,ptr_path,ptr_key);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/regitem_del")==0)
			{
				const char *ptr_path=httpreq.Request("rpath");
				const char *ptr_name=httpreq.Request("rname");
				httprsp_regitem_del(psock,httprsp,ptr_path,ptr_name);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/regitem_add")==0)
			{
				const char *ptr_path=httpreq.Request("rpath");
				const char *ptr_type=httpreq.Request("rtype");
				const char *ptr_name=httpreq.Request("rname");
				const char *ptr_value=httpreq.Request("rdata");
				httprsp_regitem_add(psock,httprsp,ptr_path,ptr_type,ptr_name,ptr_value);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/regitem_md")==0)
			{
				const char *ptr_path=httpreq.Request("rpath");
				const char *ptr_type=httpreq.Request("rtype");
				const char *ptr_name=httpreq.Request("rname");
				const char *ptr_value=httpreq.Request("rdata");
				httprsp_regitem_md(psock,httprsp,ptr_path,ptr_type,ptr_name,ptr_value);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/regitem_ren")==0)
			{
				const char *ptr_path=httpreq.Request("rpath");
				const char *ptr_name=httpreq.Request("rname");
				const char *ptr_newname=httpreq.Request("nname");
				httprsp_regitem_ren(psock,httprsp,ptr_path,ptr_name,ptr_newname);
				return true;
			}
		}//?else if( (lAccess & RMTSVC_ACCESS_REGIST_ALL)==RMTSVC_ACCESS_REGIST_ALL)
	}//?if((lAccess & RMTSVC_ACCESS_REGIST_ALL)!=0)
	//if the user has service management permission
	if((lAccess & RMTSVC_ACCESS_SERVICE_ALL)!=0)
	{
		if(strcasecmp(httpreq.url().c_str(),"/slist")==0)
		{
			const char *ptr_sname=httpreq.Request("sname");
			if(ptr_sname && //?perform action on the service
			  (lAccess & RMTSVC_ACCESS_SERVICE_ALL)==RMTSVC_ACCESS_SERVICE_ALL ) 
				sevent(ptr_sname,httpreq.Request("cmd"));
			httprsp_slist(psock,httprsp);
			return true;
		}
	}//?if((lAccess & RMTSVC_ACCESS_SERVICE_ALL)!=0) 
	//if the user has telnet permission
	if((lAccess & RMTSVC_ACCESS_TELNET_ALL)==RMTSVC_ACCESS_TELNET_ALL)
	{
		if(strcasecmp(httpreq.url().c_str(),"/telnet")==0)
		{
			httprsp_telnet(psock,httprsp,lAccess);
			return true;
		}
	}//?if((lAccess & RMTSVC_ACCESS_TELNET_ALL)==RMTSVC_ACCESS_TELNET_ALL)
	//if the user has FTP management/configuration permission
	if((lAccess & RMTSVC_ACCESS_FTP_ADMIN)==RMTSVC_ACCESS_FTP_ADMIN)
	{
		if(strcasecmp(httpreq.url().c_str(),"/ftpsets")==0)
		{
			httprsp_ftpsets(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/ftpusers")==0)
		{
			httprsp_ftpusers(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/ftpini")==0)
		{
			httprsp_ftpini(psock,httpreq,httprsp);
			return true;
		}
	}//?if((lAccess & RMTSVC_ACCESS_FTP_ADMIN)==RMTSVC_ACCESS_FTP_ADMIN)
	//if the user has file management permission
	if((lAccess & RMTSVC_ACCESS_FILE_ALL)!=0)
	{
		if(strcasecmp(httpreq.url().c_str(),"/filelist")==0)
		{
			int listWhat=1; bool bdsphide=false;
			const char *ptr_path=httpreq.Request("listwhat");
			if(ptr_path) listWhat=atoi(ptr_path);
			ptr_path=httpreq.Request("bdsph");
			if(ptr_path && strcmp(ptr_path,"true")==0) bdsphide=true;
			ptr_path=httpreq.Request("path");
			httprsp_filelist(psock,httprsp,ptr_path,listWhat,bdsphide);
			return true; 
		}
		else if(strcasecmp(httpreq.url().c_str(),"/profile_ver")==0)
		{
			const char *ptr_path=httpreq.Request("path");
			httprsp_profile_verinfo(psock,httprsp,ptr_path);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/profile")==0)
		{
			const char *ptr_prof=httpreq.Request("prof");
			const char *ptr_path=httpreq.Request("path");
			httprsp_profile(psock,httprsp,ptr_path,ptr_prof);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/profolder")==0)
		{
			const char *ptr_prof=httpreq.Request("prof");
			const char *ptr_path=httpreq.Request("path");
			httprsp_profolder(psock,httprsp,ptr_path,ptr_prof);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/prodrive")==0)
		{
			const char *ptr_volu=httpreq.Request("volu");
			const char *ptr_path=httpreq.Request("path");
			httprsp_prodrive(psock,httprsp,ptr_path,ptr_volu);
			return true;
		}
		else if(strncasecmp(httpreq.url().c_str(),"/download/",10)==0)
		{//download the specified file
			const char *ptr_path=httpreq.Request("path");
			if(ptr_path==NULL) return true;
			string filepath(ptr_path);
			if(filepath[filepath.length()-1]!='\\') filepath.append("\\");
			// url() returns the already URL-decoded path; extract filename after "/download/"
			const char *fname=httpreq.url().c_str()+10;
			filepath.append(fname);
			// Build a sanitized filename for Content-Disposition (strip CR/LF and quotes)
			string safeName;
			for(const char *p=fname;*p;p++){
				if(*p=='\r'||*p=='\n') continue;
				if(*p=='"') safeName.append("\\\"");
				else safeName+= *p;
			}
			string cdKey("Content-Disposition");
			string cdValue("attachment; filename=\"");
			cdValue.append(safeName);
			cdValue.append("\"");
			httprsp.AddHeader(cdKey,cdValue);
			httprsp.sendfile(psock,filepath.c_str());
			return true;
		}
		else if( (lAccess & RMTSVC_ACCESS_FILE_ALL)==RMTSVC_ACCESS_FILE_ALL)
		{
			if(strcasecmp(httpreq.url().c_str(),"/folder_del")==0)
			{
				bool bdsphide=false; const char *ptr_name=NULL;
				const char *ptr_path=httpreq.Request("bdsph");
				if(ptr_path && strcmp(ptr_path,"true")==0) bdsphide=true;
				ptr_path=httpreq.Request("path");
				ptr_name=httpreq.Request("fname");
				httprsp_folder_del(psock,httprsp,ptr_path,ptr_name,bdsphide);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/folder_new")==0)
			{
				bool bdsphide=false; const char *ptr_name=NULL;
				const char *ptr_path=httpreq.Request("bdsph");
				if(ptr_path && strcmp(ptr_path,"true")==0) bdsphide=true;
				ptr_path=httpreq.Request("path");
				ptr_name=httpreq.Request("fname");
				httprsp_folder_new(psock,httprsp,ptr_path,ptr_name,bdsphide);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/folder_ren")==0)
			{
				bool bdsphide=false; const char *ptr_name=NULL,*ptr_newname=NULL;
				const char *ptr_path=httpreq.Request("bdsph");
				if(ptr_path && strcmp(ptr_path,"true")==0) bdsphide=true;
				ptr_path=httpreq.Request("path");
				ptr_name=httpreq.Request("fname");
				ptr_newname=httpreq.Request("nname");
				httprsp_folder_ren(psock,httprsp,ptr_path,ptr_name,ptr_newname,bdsphide);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/file_del")==0)
			{
				bool bdsphide=false; const char *ptr_name=NULL;
				const char *ptr_path=httpreq.Request("bdsph");
				if(ptr_path && strcmp(ptr_path,"true")==0) bdsphide=true;
				ptr_path=httpreq.Request("path");
				ptr_name=httpreq.Request("fname");
				httprsp_file_del(psock,httprsp,ptr_path,ptr_name,bdsphide);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/file_ren")==0)
			{
				bool bdsphide=false; const char *ptr_name=NULL,*ptr_newname=NULL;
				const char *ptr_path=httpreq.Request("bdsph");
				if(ptr_path && strcmp(ptr_path,"true")==0) bdsphide=true;
				ptr_path=httpreq.Request("path");
				ptr_name=httpreq.Request("fname");
				ptr_newname=httpreq.Request("nname");
				httprsp_file_ren(psock,httprsp,ptr_path,ptr_name,ptr_newname,bdsphide);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/file_run")==0)
			{
				const char *ptr_path=httpreq.Request("path");
				httprsp_docommandEx(psock,httprsp,ptr_path);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/upload")==0)
			{
				httprsp_upload(psock,httpreq,httprsp,session);
				return true;
			}
			else if(strcasecmp(httpreq.url().c_str(),"/get_upratio")==0)
			{
				httprsp_get_upratio(psock,httprsp,session);
				return true;
			} 
		}//?else if( (lAccess & RMTSVC_ACCESS_FILE_ALL)==RMTSVC_ACCESS_REGIST_ALL)
	}//?if((lAccess & RMTSVC_ACCESS_FILE_ALL)!=0)
	//-----------------------------------Proxy------------------------------------
	//if the user has proxy management/configuration permission
	if((lAccess & RMTSVC_ACCESS_VIDC_ADMIN)==RMTSVC_ACCESS_VIDC_ADMIN)
	{
		if(strcasecmp(httpreq.url().c_str(),"/proxysets")==0)
		{
			httprsp_proxysets(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/proxyusers")==0)
		{
			httprsp_proxyusers(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/proxyini")==0)
		{
			httprsp_proxyini(psock,httpreq,httprsp);
			return true;
		}
	}//?if((lAccess & RMTSVC_ACCESS_FTP_ADMIN)==RMTSVC_ACCESS_FTP_ADMIN)
	//------------------------------------vIDC-------------------------------------
	//if the user has VIDC management/configuration permission
	if((lAccess & RMTSVC_ACCESS_VIDC_ADMIN)==RMTSVC_ACCESS_VIDC_ADMIN)
	{
		if(strcasecmp(httpreq.url().c_str(),"/mportL")==0)
		{
			httprsp_mportl(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/mportR")==0)
		{
			httprsp_mportr(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/vidcini")==0)
		{
			httprsp_vidcini(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/vidcsvr")==0)
		{
			httprsp_vidcsvr(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/vidccs")==0)
		{
			httprsp_vidccs(psock,httpreq,httprsp);
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/upnp")==0)
		{
			httprsp_upnp(psock,httpreq,httprsp); 
			return true;
		}
		else if(strcasecmp(httpreq.url().c_str(),"/upnpxml")==0)
		{
			httprsp_upnpxml(psock,httpreq,httprsp);
			return true;
		}
	}//?if((lAccess & RMTSVC_ACCESS_VIDC_ADMIN)==RMTSVC_ACCESS_VIDC_ADMIN)

	//For HTML, JavaScript, and CSS files, set no-cache header so the browser
	//always re-validates with the server and gets the latest version of the file.
	{
		const char *ext=strrchr(httpreq.url().c_str(),'.');
		if(ext && (strcasecmp(ext,".htm")==0 ||
		           strcasecmp(ext,".html")==0 ||
		           strcasecmp(ext,".js")==0 ||
		           strcasecmp(ext,".css")==0))
			httprsp.NoCache();
	}

	return false;
}

//Set the last modified date of the resource file 
#include "net4cpp21/utils/cTime.h"
bool webServer :: setLastModify(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	char buf[64]; 
	sprintf(buf,"%s %s",__DATE__,__TIME__); //format: Sep 11 2006 12:58:20
	cTime ct; ct.parseDate(buf);  //ct.parseDate1(buf);
	//check whether the file has been modified -------------- start---------------------
	cTime ct0; time_t t0=0,t1=1;
	const char *p=httpreq.Header("If-Modified-Since");
	if(p && ct0.parseDate(p) ){
		t0=ct0.Gettime();
		t1=ct.Gettime()+_timezone;//apply timezone adjustment
	}//?if(p)
	if(t1<=t0) //file has not been modified
	{
		this->httprsp_NotModify(psock,httprsp);
		return false;
	}
	//check whether the file has been modified --------------  end ---------------------
	ct.FormatGmt(buf,64,"%a, %d %b %Y %H:%M:%S GMT");
	httprsp.Header()["Last-Modified"]=string(buf);
	ct=cTime::GetCurrentTime();
	ct.FormatGmt(buf,64,"%a, %d %b %Y %H:%M:%S GMT");
	httprsp.Header()["Date"]=string(buf);
	return true;
}

