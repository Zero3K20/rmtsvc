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

using namespace std;
using namespace net4cpp21;

#define SOCKS4_MAX_PACKAGE_SIZE 64
void cProxysvr :: doSock4req(socketTCP *psock)
{
	char buf[SOCKS4_MAX_PACKAGE_SIZE];
	int iret,recvlen=0;
	if(m_bProxyAuthentication) return; //socks4代理协议not supported认证,但proxy service要求认证

	while( (iret=psock->Receive(buf+recvlen,SOCKS4_MAX_PACKAGE_SIZE-recvlen,PROXY_MAX_RESPTIMEOUT))>0 )
	{
		recvlen+=iret;
		sock4req *psock4req=(sock4req *)buf;
		if(recvlen>1 && psock4req->CD!=0x01 && psock4req->CD!=0x02) break;
		if( recvlen<9 ) continue; //data未receive完
		if((psock4req->IPAddr&0x00ffffff)==0) //socks4A请求
		{//支持socks4A协议 [version] [命令] [port] [0.0.0.X] [0] [hostdomain] [0]
			if(recvlen<(buf[8]+9)) continue; //data未receive完
		}
		//否则receive了完整SOCKS4的协议data,start协议handle
		sock4ans ans; ans.CD =91; ans.VN =0;
		ans.Port =0;ans.IPAddr =0;

		char *hostip=NULL; int hostport=0; //要connect的目的服务ip和port
		if((psock4req->IPAddr&0x00ffffff)==0) hostip=(char *)psock4req+9; //socks4A
		
		socketProxy peer; peer.setParent(psock);
		if(m_bCascade){ //设置了secondary proxy
			PROXYTYPE ptype=PROXY_SOCKS4;
			if((m_casProxytype & PROXY_SOCKS4)==0) //secondary proxynot supportedSOCKS4代理
			{
				if(m_casProxytype & PROXY_HTTPS)
					ptype=PROXY_HTTPS;
				else if(m_casProxytype & PROXY_SOCKS5)
					ptype=PROXY_SOCKS5;
			}
			std::pair<std::string,int> * p=GetCassvr(); //获取二级proxy service设置
			if(m_casProxyAuthentication)
				peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
			else
				peer.setProxy(ptype,p->first.c_str(),p->second,"","");
		}//?if(m_bCascade)
		if(psock4req->CD==1) //CONNECT请求
		{//connectspecified的remote主机,如果success则建立data转发对
			hostport=ntohs(psock4req->Port);
			
//			RW_LOG_DEBUG("[ProxySvr] SOCKS4 - Connecting %s:%d ... \r\n",((hostip)?hostip:socketBase::IP2A(psock4req->IPAddr)),hostport);
			
			if(m_bCascade) //设置了secondary proxy
			{//通过secondary proxyconnectspecified的application service
				if(hostip==NULL){
					strcpy(buf,socketBase::IP2A(psock4req->IPAddr));
					hostip=buf; }
				peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT);
			}else{
				if(hostip) psock4req->IPAddr=socketBase::Host2IP(hostip); //域名/IPaddressparse
				peer.SetRemoteInfo(psock4req->IPAddr,hostport);
				if( psock4req->IPAddr!=INADDR_NONE) 
					peer.socketTCP::Connect( NULL,0,PROXY_MAX_RESPTIMEOUT);
			}
			ans.CD=(peer.status()==SOCKS_CONNECTED)?90:92;
		}//?if(psock4req->CD==1)
		else if(psock4req->CD==2) //BIND请求
		{//client请求在socks 服务端建立一个临时侦听服务，等待specifiedhostip的connect到来
			if(m_bCascade) //设置了secondary proxy
			{
				std::string svrip; int svrport=hostport;
				if(hostip) svrip.assign(hostip); 
				else svrip.assign(socketBase::IP2A(psock4req->IPAddr));
				if( peer.Bind(svrip,svrport,PROXY_MAX_RESPTIMEOUT)>0 )
				{
					ans.CD=90; 
					ans.Port =htons(svrport); ans.IPAddr =peer.getRemoteip();
					psock->Send(8 /*sizeof(sock4ans)*/,(const char *)&ans,-1);
					ans.Port =0;ans.IPAddr =0;
					ans.CD=91; //等待对方connect，并send第二个响应
					if(peer.WaitForBinded(PROXY_MAX_RESPTIMEOUT,false))
						ans.CD=90; //connected successfully
					else ans.CD=92;
				}else ans.CD=92;
			}else if( (iret=peer.ListenX(0,FALSE,NULL)) > 0)
			{
				ans.CD=90; 
				ans.Port =htons(iret); ans.IPAddr =0; //psock->getLocalip();
				psock->Send(8 /*sizeof(sock4ans)*/,(const char *)&ans,-1);
				ans.Port =0;ans.IPAddr =0;
				ans.CD=91; //等待对方connect，并send第二个响应
				if(peer.Accept(PROXY_MAX_RESPTIMEOUT,NULL)>0)
					ans.CD=90; //connected successfully
			}else ans.CD=92;
		}//?else if(psock4req->CD==2)

		psock->Send(8 /*sizeof(sock4ans)*/,(const char *)&ans,-1);
		if(ans.CD==90) //SOCKS4命令success,create转发对
			transData(psock,&peer,NULL,0);
		else RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] SOCKS4 failed - CD=%d, host:port=%s:%d\r\n",
					psock4req->CD,((hostip)?hostip:peer.getRemoteIP()),hostport);
		break;//跳出循环
	}//?while
	return;
}
