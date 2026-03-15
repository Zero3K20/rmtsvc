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
	httpreq.ifParseParams(false); //不parse参数，cookie等
	SOCKSRESULT reqtype=httpreq.recv_reqH(psock,PROXY_MAX_RESPTIMEOUT);
	if(reqtype<=HTTP_REQ_UNKNOWN) return;

	PROXYACCOUNT *ptr_proa=NULL;
	if(m_bProxyAuthentication){ //要求认证 ParseAuthorizationBasic
		std::string user,pswd; int errCode;
		const char *ptr=httpreq.Header("Proxy-Authorization");
		if(ptr) httpreq.ParseAuthorizationBasic(ptr,user,pswd);
		ptr_proa=ifAccess(psock,user.c_str(),pswd.c_str(),&errCode);
		if(ptr_proa==NULL){
			if(errCode!=SOCKSERR_PROXY_CONNECTIONS) //让IE浏览器弹出account password输入框
				psock->Send(sizeof(refuseMsg)-1,refuseMsg,-1);
			return;
		}
	}//?if(m_bProxyAuthentication)
	
	int hostport=80;//authentication succeeded，获取connect主机info
	const char *hostip=httpreq.Header("PHost");
	const char *ptr=(hostip)?strstr(hostip,"://"):NULL;
	if(ptr){
		if(strncasecmp(hostip,"http://",7)!=0) hostip=NULL;//只支持HTTP代理
		else hostip=ptr+3;
	}
	ptr=(hostip)?strchr(hostip,':'):NULL;
	if(ptr){ hostport=atoi(ptr+1); *(char *)ptr=0; }
	if(!bAccessDest(ptr_proa,hostip,hostport)){ if(ptr_proa) ptr_proa->m_loginusers--; return; }
	
	socketProxy peer; peer.setParent(psock);
	if(m_bCascade) //设置了secondary proxy
	{
		PROXYTYPE ptype=PROXY_HTTPS;
		if((m_casProxytype & PROXY_HTTPS)==0) //secondary proxynot supportedHTTPS代理
		{
			if(m_casProxytype & PROXY_SOCKS4) ptype=PROXY_SOCKS4;
			else if(m_casProxytype & PROXY_SOCKS5) ptype=PROXY_SOCKS5;
		}
		std::pair<std::string,int> * p=GetCassvr(); //获取二级proxy service设置
		if(m_casProxyAuthentication)
			peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
		else
			peer.setProxy(ptype,p->first.c_str(),p->second,"","");
		peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT); //通过secondary proxyconnectspecified的application service
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
		transData(psock,&peer,NULL,0); //success,create转发对
	}else{//http GET/POST/HEAD等代理	
		std::map<std::string,std::string> &mapHeader=httpreq.Header();
		mapHeader.erase("PHost"); //从头中去掉代理PHostinfo
		mapHeader.erase("Proxy-Authorization");//从头中去掉proxy authenticationinfo
		mapHeader.erase("Proxy-Connection"); 
		httpreq.send_req(NULL,NULL);//合成HTTP请求头，但不send
		cBuffer &b=httpreq.get_contentData();//获取要send的data
		transData(psock,&peer,b.str(),b.len()); //success,create转发对
	}
	if(ptr_proa) ptr_proa->m_loginusers--;
	return;
}

//通过http代理访问FTP服务的请求
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

	httpreq.ifParseParams(false); //不parse参数，cookie等
	SOCKSRESULT sr=httpreq.recv_reqH(psock,PROXY_MAX_RESPTIMEOUT);
	if(sr<=HTTP_REQ_UNKNOWN) return;
	if(m_bProxyAuthentication) //要求认证 ParseAuthorizationBasic
	{//获取认证info
		std::string user,pswd; int errCode;
		ptr=httpreq.Header("Proxy-Authorization");
		if(ptr) httpreq.ParseAuthorizationBasic(ptr,user,pswd);
		ptr_proa=ifAccess(psock,user.c_str(),pswd.c_str(),&errCode);
		if(ptr_proa==NULL)
		{
			if(errCode!=SOCKSERR_PROXY_CONNECTIONS) //让IE浏览器弹出account password输入框
				psock->Send(sizeof(refuseMsg)-1,refuseMsg,-1);
			return;
		}
	}//?if(m_bProxyAuthentication)
	//authentication succeeded，start协议handle...
	if(sr==HTTP_REQ_CONNECT)
	{//CONNECT代理请求
		hostip=httpreq.Header("PHost");//获取connect主机info
		if(hostip==NULL){ if(ptr_proa) ptr_proa->m_loginusers--; return; }
		if( (ptr=strchr(hostip,':')) ){ hostport=atoi(ptr+1); *(char *)ptr=0; }
	}//?if(sr==HTTP_REQ_CONNECT)
	else //GET POST等代理请求
	{ //对于SSL加密的HTTP request，浏览器(IE)会先用CONNECTconnectproxy service器，然后进行SSL通讯
		std::map<std::string,std::string> &mapHeader=httpreq.Header();
		mapHeader.erase("Proxy-Authorization"); //从头中去掉proxy authenticationinfo
		std::map<std::string,std::string>::iterator it=mapHeader.find("PHost");
		if(it==mapHeader.end()){ if(ptr_proa) ptr_proa->m_loginusers--; return; }
		strHost=(*it).second; mapHeader.erase(it); //从头中去掉PHost头
		if( (it=mapHeader.find("Proxy-Connection"))!=mapHeader.end()) mapHeader.erase(it);
		
		ptr=strstr(strHost.c_str(),"://"); //error的代理请求data
		if(ptr==NULL) { if(ptr_proa) ptr_proa->m_loginusers--; return; }
		if(strncasecmp(ptr-3,"ftp",3)==0){//FTP 代理请求
			doHttp_ftpreq(strHost.c_str(),httpreq.url().c_str(),psock);
			if(ptr_proa) ptr_proa->m_loginusers--; return;
		}else hostip=ptr+3;
		if( (ptr=strchr(hostip,':')) ){ hostport=atoi(ptr+1); *(char *)ptr=0; }
	}
	
	socketProxy peer; peer.setParent(psock);
	
	bool bAccessDest=true; //是否allow access目的服务
	if(ptr_proa && ptr_proa->m_dstRules.rules()>0)
	{
		unsigned long IPAddr=socketBase::Host2IP(hostip);
		bAccessDest=ptr_proa->m_dstRules.check(IPAddr,hostport,RULETYPE_TCP);
	}
	if(bAccessDest)
	{
//		RW_LOG_DEBUG("[ProxySvr] HTTPS - Connecting %s:%d ... \r\n",hostip,hostport);
		if(m_bCascade) //设置了secondary proxy
		{
			//设置secondary proxytype
			PROXYTYPE ptype=PROXY_HTTPS;
			if((m_casProxytype & PROXY_HTTPS)==0) //secondary proxynot supportedHTTPS代理
			{
				if(m_casProxytype & PROXY_SOCKS4)
					ptype=PROXY_SOCKS4;
				else if(m_casProxytype & PROXY_SOCKS5)
					ptype=PROXY_SOCKS5;
			}
			
			std::pair<std::string,int> * p=GetCassvr(); //获取二级proxy service设置
			if(m_casProxyAuthentication)
				peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
			else
				peer.setProxy(ptype,p->first.c_str(),p->second,"","");
			peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT); //通过secondary proxyconnectspecified的application service
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
		else httpreq.send_req(&peer,NULL);//send请求data
		transData(psock,&peer,NULL,0); //success,create转发对
	}else{
		RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] can not connect %s:%d\r\n",hostip,hostport);
		psock->Send(sizeof(errMsg)-1,errMsg,-1);
	}

	if(ptr_proa) ptr_proa->m_loginusers--;
	return;
}
*/
