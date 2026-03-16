/*******************************************************************
   *	socketUdp.cpp
   *    DESCRIPTION:UDP socket class implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-02
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"
#include "include/socketUdp.h"
#include "include/cLogger.h"

using namespace std;
using namespace net4cpp21;


SOCKSRESULT socketUdp::Open(int port,bool bReuse,const char *bindIP)
{
	if(!create(SOCKS_UDP)) SOCKSERR_INVALID;
	BOOL bReuseAddr=(bReuse)?SO_REUSEADDR:FALSE;
	SOCKSRESULT sr=Bind(port,bReuseAddr,bindIP);
	if(sr<=0) 
		Close();
	else m_sockstatus=SOCKS_OPENED;
	return sr;
}
/*
//Open in multicast mode
SOCKSRESULT socketUdp::OpenAsMulti(int port,const char *strMultiIP)
{
	if(strMultiIP==NULL || strMultiIP[0]==0) return SOCKSERR_PARAM;
	SOCKSRESULT sr=Open(port,true,NULL);
	if(sr<=0) return sr;
	m_strMultiIP.assign(strMultiIP);
	if( EnableMulticast(true) ) return sr;
	m_strMultiIP="";
	Close(); return SOCKSERR_SETOPT;
}

bool socketUdp :: EnableMulticast(bool bEnabled) //Enable or disable multicast
{
    struct ip_mreq mreq;

    mreq.imr_interface.s_addr = INADDR_ANY;
    mreq.imr_multiaddr.s_addr = inet_addr(m_strMultiIP.c_str());
    return (setsockopt(m_sockfd,IPPROTO_IP, ((bEnabled)?IP_ADD_MEMBERSHIP:IP_DROP_MEMBERSHIP) ,
		(char*)&mreq,sizeof(struct ip_mreq)) ==0);
}
*/

socketUdpAnsy :: ~socketUdpAnsy()
{
	Close();
	m_thread.join();
}

SOCKSRESULT socketUdpAnsy::Open(int port,bool bReuse,const char *bindIP)
{
	if(!create(SOCKS_UDP)) SOCKSERR_INVALID;
	BOOL bReuseAddr=(bReuse)?SO_REUSEADDR:FALSE;
	SOCKSRESULT sr=Bind(port,bReuseAddr,bindIP);
	if(sr>0)
		if(!m_thread.start((THREAD_CALLBACK *)&recvThread,(void *)this)) sr=SOCKSERR_THREAD;

	if(sr<=0) 
		Close();
	else m_sockstatus=SOCKS_OPENED;
	return sr;
}

void socketUdpAnsy :: recvThread(socketUdpAnsy *psock)
{
	if(psock==NULL) return;
	
	RW_LOG_DEBUG(0,"[socketUdpAnsy] recvThread has been started\r\n");
	while(psock->m_sockstatus==SOCKS_OPENED)
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret==0) continue;
		if(iret<0) break; //An error occurred
		psock->onData(); //Data has arrived
	}//?while(
 
	RW_LOG_DEBUG(0,"[socketUdpAnsy] recvThread has been ended\r\n");
	return;
}



