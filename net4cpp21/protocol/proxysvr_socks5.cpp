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
#include "../include/socketUdp.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;


#define SOCKS5_MAX_PACKAGE_SIZE 1500
void transData_UDP(socketTCP *psock,socketUdp &sockUdp,unsigned long clntIP,int clntPort);
void cProxysvr :: doSock5req(socketTCP *psock)
{
	char buf[SOCKS5_MAX_PACKAGE_SIZE];
	int iret,recvlen=0; int istep=0; //SOCKS5 negotiation step
	PROXYACCOUNT *ptr_proa=NULL;
	while( (iret=psock->Receive(buf+recvlen,SOCKS5_MAX_PACKAGE_SIZE-recvlen,PROXY_MAX_RESPTIMEOUT))>0 )
	{
		if( (recvlen+=iret)<2 ) continue; //data not fully received yet
		if(istep==0) //step 1 SOCKS5 negotiation request
		{
			sock5req *psock5req=(sock5req *)buf;
			if(recvlen<(psock5req->nMethods+2)) continue; //data not fully received yet
			//fully received, start handling message
			sock5ans ans; ans.Ver =5;
			ans.Method=(m_bProxyAuthentication)?2:0;//2要求authentication,waitingpasswordauthenticationinfootherwisenot要求authentication
			psock->Send(2 /*sizeof(sock5ans)*/,(const char *)&ans,-1);
			istep=(m_bProxyAuthentication)?1:2; //步骤1waitingpassword verification
			recvlen=0; continue; //clearreceive的data,continue
		}
		else if(istep==1) //第二步socks5password verification
		{
			authreq *pauthreq=(authreq *)buf;
			if(pauthreq->Ver!=1) break; //error的protocoldata
			if( recvlen<4 ) continue; //data not fully received yet
			if(recvlen<(pauthreq->Ulen+3) ) continue; //data not fully received yet
			unsigned char plen=*(unsigned char *)(buf+2+pauthreq->Ulen); //getpasswordlength
			if(recvlen<(pauthreq->Ulen+plen+3) ) continue; //data not fully received yet
			//fully received, start handling message
			pauthreq->Name[pauthreq->Ulen]=0;
			char *strPass=(char *)(buf+2+pauthreq->Ulen+1);
			strPass[plen]=0;
//			pauthreq->Pass[pauthreq->PLen]=0; //error yyc modify 2007-01-10
			authans auans;auans.Ver=1;
			ptr_proa=ifAccess(psock,pauthreq->Name,strPass,NULL);
			if(ptr_proa) //authentication通过
				auans.Status=0;  //success //authentication通过
			else auans.Status=1; //authenticationnot通过
			psock->Send(2 /*sizeof(authans)*/,(const char *)&auans,-1);
			if(auans.Status!=0) break;
			istep=2; //步骤2
			recvlen=0; continue; //clearreceive的data,continue
		}//?else if(istep==1)
		else if(istep==2) //第三步handlecommandrequest
		{
			sock5req1 *psock5req1=(sock5req1 *)buf;
			char *hostip; int hostport=0;
			unsigned long IPAddr=INADDR_NONE;
			sock5ans1 ans1;ans1.Ver=5;ans1.Rsv=0;ans1.Atyp=1;
			if( recvlen<10 ) continue; //data not fully received yet
			if(psock5req1->Atyp==1) //IP address
			{
				IPAddr =*((unsigned long *)psock5req1->other);
				hostport=ntohs(*((unsigned short *)(psock5req1->other+4)));
				hostip=NULL;
			}
			else if(psock5req1->Atyp==3) //domain name
			{
				int datalen=6+psock5req1->other[0]+1;
				if( recvlen<datalen ) continue; //data not fully received yet
				hostport=ntohs(*((unsigned short *)(psock5req1->other+psock5req1->other[0]+1)));
				hostip=psock5req1->other+1; hostip[ psock5req1->other[0] ]=0;
			}
			else //unsupported address type
			{
				ans1.Rep=8;//Address type not supported
				psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
				break; 
			}
			socketProxy peer; peer.setParent(psock);
			if(m_bCascade){ //set了secondary proxy
				PROXYTYPE ptype=PROXY_SOCKS5;
				if((m_casProxytype & PROXY_SOCKS5)==0) //secondary proxynot supportedSOCKS5代理
				{
					if(m_casProxytype & PROXY_HTTPS)
						ptype=PROXY_HTTPS;
					else if(m_casProxytype & PROXY_SOCKS4)
						ptype=PROXY_SOCKS4;
				}
				std::pair<std::string,int> * p=GetCassvr(); //getsecondary proxy serviceset
				if(m_casProxyAuthentication)
					peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
				else
					peer.setProxy(ptype,p->first.c_str(),p->second,"","");
			}//?if(m_bCascade)
			if(psock5req1->Cmd ==1) //tcp connect
			{//connectspecified的remote主机,ifsuccess则建立dataforward对
				bool bAccessDest=true; //yesnoallow access目的service
				if(ptr_proa && ptr_proa->m_dstRules.rules()>0)
				{
					if(hostip) IPAddr=socketBase::Host2IP(hostip);
					bAccessDest=ptr_proa->m_dstRules.check(IPAddr,hostport,RULETYPE_TCP);
				}
				if(bAccessDest)
				{
//					RW_LOG_DEBUG("[ProxySvr] SOCKS5 - Connecting %s:%d ... \r\n",((hostip)?hostip:socketBase::IP2A(IPAddr)),hostport);

					if(m_bCascade) //set了secondary proxy
					{//通过secondary proxyconnectspecified的application service
						if(hostip==NULL){
							strcpy(buf,socketBase::IP2A(IPAddr));
							hostip=buf; }
						peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT); 
					}else{//域名/IPaddressparse
						if(hostip) IPAddr=socketBase::Host2IP(hostip);
						peer.SetRemoteInfo(IPAddr,hostport);
						if( IPAddr!=INADDR_NONE) 
							peer.socketTCP::Connect( NULL,0,PROXY_MAX_RESPTIMEOUT);
					}//?if(m_bCascade)...else...
				}
				if(peer.status()==SOCKS_CONNECTED){
					ans1.Rep=0;
					ans1.IPAddr=peer.getRemoteip();
					ans1.Port=htons(peer.getRemotePort());
				}else ans1.Rep=5;
			}//?if(psock5req1->Cmd ==1)
			else if(psock5req1->Cmd ==2) //tcp bind
			{//clientrequestatsocks server-side建立一个临时侦听service，waitingspecifiedhostip的connect到来
				if(m_bCascade) //set了secondary proxy
				{
					std::string svrip; int svrport=hostport;
					if(hostip) svrip.assign(hostip); 
					else svrip.assign(socketBase::IP2A(IPAddr));
					if( peer.Bind(svrip,svrport,PROXY_MAX_RESPTIMEOUT)>0 )
					{
						ans1.Rep=0;  ans1.Port=htons(svrport);
						ans1.IPAddr=peer.getRemoteip();//...//sendsuccessresponse 
						psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
						ans1.Port=0; ans1.IPAddr=0;
						ans1.Rep=1; //waiting对方connect，并send第二个response
						if(peer.WaitForBinded(PROXY_MAX_RESPTIMEOUT,false))
						{
							ans1.Port=htons(peer.getRemotePort()); //...可能should为实际的port/IP???
							ans1.IPAddr=peer.getRemoteip(); //...
							ans1.Rep=0; //connected successfully
						}
					}else ans1.Rep=1;
				}else if( (iret=peer.ListenX(0,FALSE,NULL)) > 0)
				{
					ans1.Rep=0;  ans1.Port=htons(iret);
					ans1.IPAddr=0; //psock->getLocalip();//sendsuccessresponse 
					psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
					ans1.Port=0; ans1.IPAddr=0;
					ans1.Rep=1; //waiting对方connect，并send第二个response
					if(peer.Accept(PROXY_MAX_RESPTIMEOUT,NULL)>0)
					{
						ans1.Port=htons(peer.getRemotePort());
						ans1.IPAddr=peer.getRemoteip();
						ans1.Rep=0; //connected successfully
					}
				}else ans1.Rep=1; //general error
			}//?else if(psock5req1->Cmd ==2)
			else if(psock5req1->Cmd ==3) //udp代理
			{
				socketUdp sockUdp; //open一个UDPport用来代理forward data
				SOCKSRESULT sr=sockUdp.Open(0,false,NULL);
				if(sr>0){ //UDPportopensuccess
					std::string svrip; int svrport=sr; //secondary proxyreturn的代理UDPportandIP
					if(!m_bCascade || peer.UdpAssociate(svrip,svrport,PROXY_MAX_RESPTIMEOUT)>0)
					{//if没有setsecondary proxy，or者向secondary proxysendUDP代理commandsuccess
						ans1.Rep=0; ans1.Port=htons(sr); //success
						ans1.IPAddr=psock->getLocalip();
						psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
						//getUDP代理clientIPandport，即clntIP,clntPort
						if(hostip) IPAddr=socketBase::Host2IP(hostip);
						if(IPAddr==INADDR_ANY || IPAddr==INADDR_NONE) IPAddr=psock->getRemoteip();
						if(m_bCascade){
							peer.setRemoteInfo(svrip.c_str(),svrport);
							sockUdp.setParent(&peer);
						}
						transData_UDP(psock,sockUdp,IPAddr,hostport); //startUDPdata代理forward
						break;
					}//?if(!m_bCascade ||
				}//?if(sr>0)
				ans1.Rep=1; //常见的Socks故障 //ans1.Rep=7;//暂时先not supported
			}//?else if(psock5req1->Cmd ==3) //udp代理
			else ans1.Rep=7;//not supported的command
			psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
			if(ans1.Rep==0) //SOCKS5commandsuccess,createforward对
				transData(psock,&peer,NULL,0);
			else RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] SOCKS5 failed - Cmd=%d, Atyp=%d, Rep=%d, host:port=%s:%d\r\n",
					psock5req1->Cmd,psock5req1->Atyp,ans1.Rep,((hostip)?hostip:peer.getRemoteIP()),hostport);
			break;//跳出循环,endhandle
		}//?else if(istep==2)
		else break; //error的步骤
	}//?while

	if(ptr_proa) ptr_proa->m_loginusers--;
}
//yyc 2007-02-12 增加socks5 UDP代理的handle
//UDPdataforward
#define UDPPACKAGESIZE 8192
#define SOCKS5UDP_SIZE 10  //sizeof(sock5udp)==12
void transData_UDP(socketTCP *psock,socketUdp &sockUdp,unsigned long clntIP,int clntPort)
{
	if(psock==NULL) return;
	socketTCP *psvr=(socketTCP *)psock->parent();
	if(psvr==NULL) return;
	//yesnospecified了secondary proxy,peer为connectsecondary proxy的socket
	socketProxy *peer=(socketProxy *)sockUdp.parent();
	
	char *buffer=new char[SOCKS5UDP_SIZE+UDPPACKAGESIZE];
	if(buffer==NULL) return;
	
	//startUDPdata的代理forward
	RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP has been started\r\n");
	if(peer) //specified了secondary proxy
	{   //getsecondary proxy开的UDP代理port
		int casUdpPort=peer->getRemotePort();
		unsigned long casUdpIP=peer->getRemoteip();
		RW_LOG_DEBUG("[ProxySvr] socks5-UDP : Cascade UDP %s:%d\r\n",socketBase::IP2A(casUdpIP),casUdpPort);
		while(psvr->status()==SOCKS_LISTEN)
		{
			//checkSocket会判断connectsecondary proxy的connectyesno异常
			int iret=sockUdp.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret==0){
				if(psock->checkSocket(0,SOCKS_OP_READ)<0) break;
				else if(peer->checkSocket(0,SOCKS_OP_READ)<0) break;
				else continue;
			}
			if(iret<0) break; //发生error
			//read data sent by client
			iret=sockUdp.Receive(buffer,UDPPACKAGESIZE,-1);
			if(iret<=0){//抛弃error，仅仅打印errorinfo
				RW_LOG_DEBUG("[ProxySvr] socks5-UDP : Failed to Receive,(%d - %d)\r\n",iret,sockUdp.getErrcode());
				continue; 
			}
			//ifyes从代理client过来的UDPdata则forward给secondary proxy，
			if(sockUdp.getRemoteip()==clntIP && sockUdp.getRemotePort()==clntPort)
				sockUdp.SetRemoteInfo(casUdpIP,casUdpPort);
			else //otherwiseforward给代理client
				sockUdp.SetRemoteInfo(clntIP,clntPort);
			iret=sockUdp.Send(iret,buffer,-1);

//			RW_LOG_DEBUG("CasCade - len=%d data:\r\n:%s.\r\n",iret,buffer+sizeof(socks5udp));
		}//?while
	}
	else //没有specifiedsecondary proxy
	{
		char *buf=buffer+SOCKS5UDP_SIZE;
		socks5udp *pudp_pack=(socks5udp *)buffer;
		pudp_pack->Rsv=0; pudp_pack->Frag=0;
		pudp_pack->Atyp=0x01; //IPV4
		while(psvr->status()==SOCKS_LISTEN)
		{
			int iret=sockUdp.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret==0){
				if(psock->checkSocket(0,SOCKS_OP_READ)<0) break;
				else continue;
			}
			if(iret<0){
				RW_LOG_DEBUG("[ProxySvr] socks5-UDP : checkSocket() return %d\r\n",iret);
				break; //发生error
			}
			//read data sent by client
			iret=sockUdp.Receive(buf,UDPPACKAGESIZE,-1);
			if(iret<=0){//抛弃error，仅仅打印errorinfo
				RW_LOG_DEBUG("[ProxySvr] socks5-UDP : Failed to Receive,(%d - %d)\r\n",iret,sockUdp.getErrcode());
				continue; 
			}
			//ifyes从代理client过来的UDPdata则forward给secondary proxy，
			if(sockUdp.getRemoteip()==clntIP && sockUdp.getRemotePort()==clntPort)
			{//从UDPdatapacket中parse出实际的IPaddressandport
				unsigned long IPAddr=INADDR_NONE; int hostport=0;
				socks5udp *pudp=(socks5udp *)buf; long udppackLen;
				if(pudp->Frag!=0) //非独立UDPpacket需进行碎片重组
				{//暂时notimplementation碎片重组功能，抛弃此packet
					RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP : Received UDP frag,discard it\r\n");
					continue;
				}
				if(pudp->Atyp==1) //IP address
				{
					IPAddr =pudp->IPAddr;
					hostport=ntohs(pudp->Port);
					udppackLen=SOCKS5UDP_SIZE;
				}
				else if(pudp->Atyp==3) //domain name
				{
					char *other=(char *)&pudp->IPAddr;
					udppackLen=7+other[0]; //4+1+other[0]+2;
					hostport=ntohs(*((unsigned short *)(other+other[0]+1)));
					other[other[0]+1]='\0'; other++; //other pointer to domain name
					IPAddr=socketBase::Host2IP(other);
				}else{
					RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP : Not surpport Address Type\r\n");
					break; //unsupported address type
				}
				sockUdp.SetRemoteInfo(IPAddr,hostport);
				iret=sockUdp.Send(iret-udppackLen,buf+udppackLen,-1);
//				RW_LOG_DEBUG("Client->UDP(%s:%d) - len=%d data:\r\n%s.\r\n",
//					socketBase::IP2A(IPAddr),hostport,iret-udppackLen,buf+udppackLen);
			}else{ //otherwise进行UDP封packet然后forward给代理client
				pudp_pack->IPAddr=sockUdp.getRemoteip();
				pudp_pack->Port=htons(sockUdp.getRemotePort());
				sockUdp.SetRemoteInfo(clntIP,clntPort);
				sockUdp.Send(iret+SOCKS5UDP_SIZE,buffer,-1);
//				RW_LOG_DEBUG("UDP->Client(%s:%d) - len=%d+%d data:\r\n%s.\r\n",
//					socketBase::IP2A(clntIP),clntPort,SOCKS5UDP_SIZE,iret,buf);
			}
		}//?while
	}

	sockUdp.Close(); delete[] buffer; 
	RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP has been ended\r\n");
	return;
}

