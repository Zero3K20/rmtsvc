/*******************************************************************
   *	socketSvr.cpp
   *    DESCRIPTION:Implementation of the TCP asynchronous server class
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-01
   *	
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"
#include "include/socketSvr.h"
#include "utils/utils.h"
#include "include/cLogger.h"

using namespace std;
using namespace net4cpp21;

socketSvr :: socketSvr()
{
	m_strSvrname="Listen-server";//Listen server
	m_curConnection=0;
	m_maxConnection=0;
	m_lAcceptTimeOut=500; //500ms async Accept timeout; sets the default accept wait delay
	//==-1 blocks until a connection arrives; testing shows that with -1, if the listening socket has a
	//connection that the peer has not closed, closing the Listen Socket still leaves accept blocking.
	//Therefore m_lAcceptTimeOut should not be set to -1
	m_bReuseAddr=FALSE;
}

socketSvr :: ~socketSvr()
{
	Close();
	m_threadpool.join();//Do not wait for thread completion at Close to avoid calling Close from within a callback
}

//Start the service
SOCKSRESULT socketSvr::Listen(int port,BOOL bReuseAddr/*=FALSE*/,const char *bindIP/*=NULL*/)
{
	m_bReuseAddr=bReuseAddr;
	SOCKSRESULT sr=socketTCP::ListenX(port,m_bReuseAddr,bindIP);
	if(sr<=0){
		RW_LOG_DEBUG("Failed to Listen at %d,error=%d\r\n",port,sr);
		return sr;
	}
	//Service port started successfully; launching Accept service thread
	if( m_threadpool.addTask((THREAD_CALLBACK *)&listenThread,(void *)this,0)!=0 )
		return sr;
	
	Close();//Service thread failed to start
	return SOCKSERR_THREAD;
}

//Start the service
SOCKSRESULT socketSvr::Listen(int startport,int endport,BOOL bReuseAddr/*=FALSE*/,const char *bindIP/*=NULL*/)
{
	m_bReuseAddr=bReuseAddr;
	SOCKSRESULT sr=(startport==endport)?
				socketTCP::ListenX(startport,m_bReuseAddr,bindIP):
				socketTCP::ListenX(startport,endport,m_bReuseAddr,bindIP);
	if(sr<=0){
		RW_LOG_DEBUG("Failed to Listen at %d-%d,error=%d\r\n",startport,endport,sr);
		return sr;
	}
	//Service port started successfully; launching Accept service thread
	if( m_threadpool.addTask((THREAD_CALLBACK *)&listenThread,(void *)this,0)!=0 )
		return sr;
	
	Close();//Service thread failed to start
	return SOCKSERR_THREAD;
}

//Listen thread
void socketSvr :: listenThread(socketSvr *psvr)
{
	if(psvr==NULL) return;
	int svrPort=psvr->getLocalPort();
#ifdef _SURPPORT_OPENSSL_
	RW_LOG_DEBUG("[socketSvr] %s%s has been started,port=%d\r\n",
				psvr->m_strSvrname.c_str(),(psvr->ifSSL())?"(ssl)":"",svrPort);
#else
	RW_LOG_DEBUG("[socketSvr] %s has been started,port=%d\r\n",
				psvr->m_strSvrname.c_str(),svrPort);
#endif
	//yyc add 2007-03-29 If this service bound the port with SO_REUSEADDR
	//and bound to a specific IP (not 127.0.0.1), redirect inaccessible visitors
	//to the old service at 127.0.0.1:<local port>
	bool bReirectOldSvr=( psvr->getLocalip()!=INADDR_ANY && psvr->m_bReuseAddr==SO_REUSEADDR)?true:false;
	socketTCP *psock=NULL;
	while(psvr->m_sockstatus==SOCKS_LISTEN)
	{
		if(psock==NULL)
		{
			if( (psock=new socketTCP())==NULL) 
			{
				RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[listenThread] failed to new socketTCP()\r\n");
				break;
			}
		}
		SOCKSRESULT sr=psvr->Accept(psvr->m_lAcceptTimeOut,psock);
		if(sr==SOCKSERR_TIMEOUT){ psvr->onIdle(); continue; } //Timeout
		if(sr<0) break; //An error occurred

		RW_LOG_DEBUG("[socketSvr] one connection (%s:%d) coming in\r\n",psock->getRemoteIP(),psock->getRemotePort());
		if(psvr->m_srcRules.check(psock->getRemoteip(),psock->getRemotePort(),RULETYPE_TCP))
		{
			if( psvr->m_threadpool.addTask((THREAD_CALLBACK *)&doacceptTask,(void *)psock,THREADLIVETIME)!=0 )
			{	psock=NULL; continue; }
		}
		else if(bReirectOldSvr) //Redirect the specified request to the reused service
		{
			if( psvr->m_threadpool.addTask((THREAD_CALLBACK *)&doRedirectTask,(void *)psock,THREADLIVETIME)!=0 )
			{	psock=NULL; continue; }
		}
		//yyc add 2007-03-29 ---------------------------------------
		psock->Close();
	}//?while
	
	if(psock) delete psock;
	RW_LOG_DEBUG("[socketSvr] %s has been ended,port=%d!\r\n",psvr->m_strSvrname.c_str(),svrPort);
}

//Task for handling a new incoming connection
void socketSvr :: doacceptTask(socketTCP *psock)
{
	if(psock==NULL) return;
	socketSvr *psvr=(socketSvr *)psock->parent();
#ifdef _SURPPORT_OPENSSL_
	if(psock->ifSSL()) psock->SSL_Associate();
#endif
	if(psvr){
		psvr->m_curConnection++;
		if(psvr->m_maxConnection!=0 &&  psvr->m_curConnection > psvr->m_maxConnection)
			psvr->onTooMany(psock); 
		else
			psvr->onAccept(psock);
		psvr->m_curConnection--;
	}
	delete psock; return;
}

//yyc add 2007-03-29 Allow reuse of a system service port
#define MAX_TRANSFER_BUFFER 2048
#define MAX_CONNECT_TIMEOUT 5
void socketSvr :: doRedirectTask(socketTCP *psock)
{
	if(psock==NULL) return;
	socketSvr *psvr=(socketSvr *)psock->parent();
	//Connect to the reused service port at address: 127.0.0.1
	socketTCP *ppeer=new socketTCP;
	if(ppeer==NULL) { delete psock; return; }
	if( ppeer->Connect("127.0.0.1",psvr->getLocalPort(),MAX_CONNECT_TIMEOUT)>0)
		ppeer->setParent(psock);
	else { delete psock; delete ppeer; return; }

	RW_LOG_DEBUG("[socketSvr] Redirect old server(127.0.0.1:%d)\r\n",psvr->getLocalPort());

	if( psvr->m_threadpool.addTask((THREAD_CALLBACK *)&transThread,(void *)ppeer,THREADLIVETIME)==0 )
	{
		delete psock; delete ppeer;
		return;
	}

	char buf[MAX_TRANSFER_BUFFER]; //Start forwarding
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//Read data sent by the client
		iret=psock->Receive(buf,MAX_TRANSFER_BUFFER-1,-1);
		if(iret<0) break; //==0 indicates the receive rate exceeds the limit
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		iret=ppeer->Send(iret,buf,-1);
		if(iret<0) break;
	}//?while
	ppeer->Close(); //Wait for the transThread thread to finish
	while(ppeer->parent()!=NULL) cUtils::usleep(SCHECKTIMEOUT);
	
	delete psock; delete ppeer;
	return;
}

//Forwarding thread
void socketSvr :: transThread(socketTCP *psock)
{
	if(psock==NULL) return;
	socketTCP *ppeer=(socketTCP *)psock->parent();
	if(ppeer==NULL) return;
//	socketSvr *psvr=(socketSvr *)ppeer->parent();

	char buf[MAX_TRANSFER_BUFFER]; //Start forwarding
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//Read data sent by the client
		iret=psock->Receive(buf,MAX_TRANSFER_BUFFER-1,-1);
		if(iret<0) break; //==0 indicates the receive rate exceeds the limit
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		iret=ppeer->Send(iret,buf,-1);
		if(iret<0) break;
	}//?while
	ppeer->Close(); 
	psock->setParent(NULL); //Used to determine whether the forwarding thread has ended
	return;
}
