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
	if(m_bProxyAuthentication) return; //SOCKS4 proxy protocol does not support authentication, but the proxy service requires it

	while( (iret=psock->Receive(buf+recvlen,SOCKS4_MAX_PACKAGE_SIZE-recvlen,PROXY_MAX_RESPTIMEOUT))>0 )
	{
		recvlen+=iret;
		sock4req *psock4req=(sock4req *)buf;
		if(recvlen>1 && psock4req->CD!=0x01 && psock4req->CD!=0x02) break;
		if( recvlen<9 ) continue; //data not fully received yet
		if((psock4req->IPAddr&0x00ffffff)==0) //socks4Arequest
		{//supports socks4A protocol: [version] [command] [port] [0.0.0.X] [0] [hostdomain] [0]
			if(recvlen<(buf[8]+9)) continue; //data not fully received yet
		}
		//otherwise complete SOCKS4 protocol data has been received, start protocol handling
		sock4ans ans; ans.CD =91; ans.VN =0;
		ans.Port =0;ans.IPAddr =0;

		char *hostip=NULL; int hostport=0; //destination service IP and port to connect to
		if((psock4req->IPAddr&0x00ffffff)==0) hostip=(char *)psock4req+9; //socks4A
		
		socketProxy peer; peer.setParent(psock);
		if(m_bCascade){ //a secondary proxy has been set
			PROXYTYPE ptype=PROXY_SOCKS4;
			if((m_casProxytype & PROXY_SOCKS4)==0) //secondary proxy does not support SOCKS4 proxy
			{
				if(m_casProxytype & PROXY_HTTPS)
					ptype=PROXY_HTTPS;
				else if(m_casProxytype & PROXY_SOCKS5)
					ptype=PROXY_SOCKS5;
			}
			std::pair<std::string,int> * p=GetCassvr(); //getsecondary proxy serviceset
			if(m_casProxyAuthentication)
				peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
			else
				peer.setProxy(ptype,p->first.c_str(),p->second,"","");
		}//?if(m_bCascade)
		if(psock4req->CD==1) //CONNECTrequest
		{//connect to the specified remote host; if successful, create a data forwarding pair
			hostport=ntohs(psock4req->Port);
			
//			RW_LOG_DEBUG("[ProxySvr] SOCKS4 - Connecting %s:%d ... \r\n",((hostip)?hostip:socketBase::IP2A(psock4req->IPAddr)),hostport);
			
			if(m_bCascade) //a secondary proxy has been set
			{//connect to the specified application service via secondary proxy
				if(hostip==NULL){
					strcpy(buf,socketBase::IP2A(psock4req->IPAddr));
					hostip=buf; }
				peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT);
			}else{
				if(hostip) psock4req->IPAddr=socketBase::Host2IP(hostip); //domain name/IPaddressparse
				peer.SetRemoteInfo(psock4req->IPAddr,hostport);
				if( psock4req->IPAddr!=INADDR_NONE) 
					peer.socketTCP::Connect( NULL,0,PROXY_MAX_RESPTIMEOUT);
			}
			ans.CD=(peer.status()==SOCKS_CONNECTED)?90:92;
		}//?if(psock4req->CD==1)
		else if(psock4req->CD==2) //BINDrequest
		{//client requests the socks server to create a temporary listening service, waiting for a connection from the specified host IP
			if(m_bCascade) //a secondary proxy has been set
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
					ans.CD=91; //waiting for peer to connect, and send the second response
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
				ans.CD=91; //waiting for peer to connect, and send the second response
				if(peer.Accept(PROXY_MAX_RESPTIMEOUT,NULL)>0)
					ans.CD=90; //connected successfully
			}else ans.CD=92;
		}//?else if(psock4req->CD==2)

		psock->Send(8 /*sizeof(sock4ans)*/,(const char *)&ans,-1);
		if(ans.CD==90) //SOCKS4 command success, create forwarding pair
			transData(psock,&peer,NULL,0);
		else RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] SOCKS4 failed - CD=%d, host:port=%s:%d\r\n",
					psock4req->CD,((hostip)?hostip:peer.getRemoteIP()),hostport);
		break;//exit loop
	}//?while
	return;
}
