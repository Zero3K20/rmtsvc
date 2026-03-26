/*******************************************************************
   *	proxysvr.cp
   *    DESCRIPTION:proxy server implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-08-24
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/proxyclnt.h"
#include "../include/proxysvr.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

#include "../include/httpreq.h"
#include "../include/ftpclnt.h"

using namespace std;
using namespace net4cpp21;

static const char refuseMsg[]=
	"HTTP/1.0 407 Proxy Authentication required\r\nServer: YYProxy\r\nProxy-Authenticate: Basic realm=\"Proxy Authorization\"\r\nCache-control: no-cache\r\n\r\n";
static const char okMsg[]="HTTP/1.0 200 Connection established\r\n\r\n";
static const char errMsg[]="HTTP/1.0 504 Can not connect host.\r\n\r\n";

inline bool bAccessDest(PROXYACCOUNT *ptr_proa,const char *hostip,int hostport)
{
	if(hostip==NULL || hostport<=0) return false;
	if(ptr_proa==NULL || ptr_proa->m_dstRules.rules()<=0) return true;
	unsigned long IPAddr=socketBase::Host2IP(hostip);
	if( ptr_proa->m_dstRules.check(IPAddr,hostport,RULETYPE_TCP) ) return true;
	ptr_proa->m_loginusers--; return false;
}

void cProxysvr :: doHttpsreq(socketTCP *psock)
{
	httpRequest httpreq;
	httpreq.ifParseParams(false); //do not parse parameters, cookies, etc.
	SOCKSRESULT reqtype=httpreq.recv_reqH(psock,PROXY_MAX_RESPTIMEOUT);
	if(reqtype<=HTTP_REQ_UNKNOWN) return;

	PROXYACCOUNT *ptr_proa=NULL;
	if(m_bProxyAuthentication){ //authentication required: ParseAuthorizationBasic
		std::string user,pswd; int errCode;
		const char *ptr=httpreq.Header("Proxy-Authorization");
		if(ptr) httpreq.ParseAuthorizationBasic(ptr,user,pswd);
		ptr_proa=ifAccess(psock,user.c_str(),pswd.c_str(),&errCode);
		if(ptr_proa==NULL){
			if(errCode!=SOCKSERR_PROXY_CONNECTIONS) //cause IE browser to pop up account password dialog
				psock->Send(sizeof(refuseMsg)-1,refuseMsg,-1);
			return;
		}
	}//?if(m_bProxyAuthentication)
	
	int hostport=80;//authentication succeeded, get connect host info
	const char *hostip=httpreq.Header("PHost");
	const char *ptr=(hostip)?strstr(hostip,"://"):NULL;
	if(ptr){
		if(strncasecmp(hostip,"http://",7)!=0) hostip=NULL;//only HTTP proxy supported
		else hostip=ptr+3;
	}
	ptr=(hostip)?strchr(hostip,':'):NULL;
	if(ptr){ hostport=atoi(ptr+1); *(char *)ptr=0; }
	if(!bAccessDest(ptr_proa,hostip,hostport)){ if(ptr_proa) ptr_proa->m_loginusers--; return; }
	
	socketProxy peer; peer.setParent(psock);
	if(m_bCascade) //a secondary proxy has been set
	{
		PROXYTYPE ptype=PROXY_HTTPS;
		if((m_casProxytype & PROXY_HTTPS)==0) //secondary proxy does not support HTTPS proxy
		{
			if(m_casProxytype & PROXY_SOCKS4) ptype=PROXY_SOCKS4;
			else if(m_casProxytype & PROXY_SOCKS5) ptype=PROXY_SOCKS5;
		}
		std::pair<std::string,int> * p=GetCassvr(); //getsecondary proxy serviceset
		if(m_casProxyAuthentication)
			peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
		else
			peer.setProxy(ptype,p->first.c_str(),p->second,"","");
		peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT); //connect to the specified application service via secondary proxy
	}else{
		unsigned long IPAddr=socketBase::Host2IP(hostip);
		peer.SetRemoteInfo(IPAddr,hostport);
		if( IPAddr!=INADDR_NONE) peer.socketTCP::Connect( NULL,0,PROXY_MAX_RESPTIMEOUT);
	}
	if(peer.status()!=SOCKS_CONNECTED) //connection failed
	{
		RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] can not connect %s:%d\r\n",hostip,hostport);
		psock->Send(sizeof(errMsg)-1,errMsg,-1);
	}else if(reqtype==HTTP_REQ_CONNECT){
		psock->Send(sizeof(okMsg)-1,okMsg,-1);
		transData(psock,&peer,NULL,0); //success, create forwarding pair
	}else{//http GET/POST/HEAD and other proxy types	
		std::map<std::string,std::string> &mapHeader=httpreq.Header();
		mapHeader.erase("PHost"); //remove PHost proxy info from header
		mapHeader.erase("Proxy-Authorization");//remove proxy authentication info from header
		mapHeader.erase("Proxy-Connection"); 
		httpreq.send_req(NULL,NULL);//assemble HTTP request header but do not send
		cBuffer &b=httpreq.get_contentData();//get the data to send
		transData(psock,&peer,b.str(),b.len()); //success, create forwarding pair
	}
	if(ptr_proa) ptr_proa->m_loginusers--;
	return;
}

//request to access FTP service via HTTP proxy
/*void cProxysvr :: doHttp_ftpreq(const char *ftphost,const char * ftpurl,socketTCP *psock)
{
	psock->Send(sizeof(errMsg)-1,errMsg,-1);
	return;
}
*/
/*
void cProxysvr :: doHttpsreq(socketTCP *psock)
{
	httpRequest httpreq; std::string strHost;
	int hostport=80; const char *ptr,*hostip=NULL;
	PROXYACCOUNT *ptr_proa=NULL;

	httpreq.ifParseParams(false); //do not parse parameters, cookies, etc.
	SOCKSRESULT sr=httpreq.recv_reqH(psock,PROXY_MAX_RESPTIMEOUT);
	if(sr<=HTTP_REQ_UNKNOWN) return;
	if(m_bProxyAuthentication) //authentication required: ParseAuthorizationBasic
	{//getauthenticationinfo
		std::string user,pswd; int errCode;
		ptr=httpreq.Header("Proxy-Authorization");
		if(ptr) httpreq.ParseAuthorizationBasic(ptr,user,pswd);
		ptr_proa=ifAccess(psock,user.c_str(),pswd.c_str(),&errCode);
		if(ptr_proa==NULL)
		{
			if(errCode!=SOCKSERR_PROXY_CONNECTIONS) //cause IE browser to pop up account password dialog
				psock->Send(sizeof(refuseMsg)-1,refuseMsg,-1);
			return;
		}
	}//?if(m_bProxyAuthentication)
	//authentication succeeded, start protocol handling...
	if(sr==HTTP_REQ_CONNECT)
	{//CONNECTproxy request
		hostip=httpreq.Header("PHost");//get the connect host info
		if(hostip==NULL){ if(ptr_proa) ptr_proa->m_loginusers--; return; }
		if( (ptr=strchr(hostip,':')) ){ hostport=atoi(ptr+1); *(char *)ptr=0; }
	}//?if(sr==HTTP_REQ_CONNECT)
	else //GET POST and other proxy requests
	{ //for SSL-encrypted HTTP requests, the browser (IE) first uses CONNECT to connect to the proxy server, then performs SSL communication
		std::map<std::string,std::string> &mapHeader=httpreq.Header();
		mapHeader.erase("Proxy-Authorization"); //remove proxy authentication info from header
		std::map<std::string,std::string>::iterator it=mapHeader.find("PHost");
		if(it==mapHeader.end()){ if(ptr_proa) ptr_proa->m_loginusers--; return; }
		strHost=(*it).second; mapHeader.erase(it); //remove PHost header from header map
		if( (it=mapHeader.find("Proxy-Connection"))!=mapHeader.end()) mapHeader.erase(it);
		
		ptr=strstr(strHost.c_str(),"://"); //malformed proxy request data
		if(ptr==NULL) { if(ptr_proa) ptr_proa->m_loginusers--; return; }
		if(strncasecmp(ptr-3,"ftp",3)==0){//FTP proxy request
			doHttp_ftpreq(strHost.c_str(),httpreq.url().c_str(),psock);
			if(ptr_proa) ptr_proa->m_loginusers--; return;
		}else hostip=ptr+3;
		if( (ptr=strchr(hostip,':')) ){ hostport=atoi(ptr+1); *(char *)ptr=0; }
	}
	
	socketProxy peer; peer.setParent(psock);
	
	bool bAccessDest=true; //whether to allow access to the destination service
	if(ptr_proa && ptr_proa->m_dstRules.rules()>0)
	{
		unsigned long IPAddr=socketBase::Host2IP(hostip);
		bAccessDest=ptr_proa->m_dstRules.check(IPAddr,hostport,RULETYPE_TCP);
	}
	if(bAccessDest)
	{
//		RW_LOG_DEBUG("[ProxySvr] HTTPS - Connecting %s:%d ... \r\n",hostip,hostport);
		if(m_bCascade) //a secondary proxy has been set
		{
			//setsecondary proxytype
			PROXYTYPE ptype=PROXY_HTTPS;
			if((m_casProxytype & PROXY_HTTPS)==0) //secondary proxy does not support HTTPS proxy
			{
				if(m_casProxytype & PROXY_SOCKS4)
					ptype=PROXY_SOCKS4;
				else if(m_casProxytype & PROXY_SOCKS5)
					ptype=PROXY_SOCKS5;
			}
			
			std::pair<std::string,int> * p=GetCassvr(); //getsecondary proxy serviceset
			if(m_casProxyAuthentication)
				peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
			else
				peer.setProxy(ptype,p->first.c_str(),p->second,"","");
			peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT); //connect to the specified application service via secondary proxy
		}else{
			unsigned long IPAddr=socketBase::Host2IP(hostip);
			peer.SetRemoteInfo(IPAddr,hostport);
			if( IPAddr!=INADDR_NONE) peer.socketTCP::Connect( NULL,0,PROXY_MAX_RESPTIMEOUT);
		}
	}//?if( bAccessDest )

	if(peer.status()==SOCKS_CONNECTED) //connected successfully
	{
		if(sr==HTTP_REQ_CONNECT) 
			psock->Send(sizeof(okMsg)-1,okMsg,-1);
		else httpreq.send_req(&peer,NULL);//sendrequestdata
		transData(psock,&peer,NULL,0); //success, create forwarding pair
	}else{
		RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] can not connect %s:%d\r\n",hostip,hostport);
		psock->Send(sizeof(errMsg)-1,errMsg,-1);
	}

	if(ptr_proa) ptr_proa->m_loginusers--;
	return;
}
*/
