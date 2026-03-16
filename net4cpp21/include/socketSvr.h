/*******************************************************************
   *	socketSvr.h
   *    DESCRIPTION:Definition of the TCP asynchronous server class
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-01
   *	
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SOCKET_SVR_H__
#define __YY_SOCKET_SVR_H__

#include "socketTcp.h"
#include "IPRules.h"
#include "cThread.h"

namespace net4cpp21
{
	class socketSvr : public socketTCP
	{
	public:
		socketSvr();
		virtual ~socketSvr();
		//Start the service
		SOCKSRESULT Listen(int port,BOOL bReuseAddr=FALSE,const char *bindIP=NULL);
		SOCKSRESULT Listen(int startport,int endport,BOOL bReuseAddr=FALSE,const char *bindIP=NULL);
		const char *svrname() { return m_strSvrname.c_str();} //Returns the service name
		iprules &rules() { return m_srcRules;}
		void maxConnection(unsigned long ulMax) { m_maxConnection=ulMax; return; }
		unsigned long maxConnection() const { return m_maxConnection; }
		unsigned long curConnection() const { return m_curConnection; }
		BOOL GetReuseAddr() const { return m_bReuseAddr; } //Get port reuse status
		
	private: //Disable copy and assignment
		socketSvr(socketSvr &sockSvr){ return; }
		socketSvr & operator = (socketSvr &sockSvr) { return *this; }	
	protected:
		cThreadPool m_threadpool;//Service thread pool
		std::string m_strSvrname;//Service name
		
		//Triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock){ return; }
		//Triggered when the current connection count exceeds the configured maximum
		virtual void onTooMany(socketTCP *psock) { return; }
		virtual void onIdle(void) { return; } //This event only fires when m_lAcceptTimeOut async timeout is configured

		
	private:
		iprules m_srcRules;//Default access filter rule object; source IP filter rules
		unsigned long m_curConnection; //Current concurrent connection count
		unsigned long m_maxConnection; //Maximum allowed concurrent connections; 0 means unlimited
		long	m_lAcceptTimeOut; //Async Accept timeout; ==-1 means wait indefinitely

		BOOL m_bReuseAddr; //Service port reuse status; value: SO_REUSEADDR/SO_EXCLUSIVEADDRUSE/FALSE
		static void transThread(socketTCP *psock); //yyc add 2007-03-29
		static void doRedirectTask(socketTCP *psock); //yyc add 2007-03-29

		static void listenThread(socketSvr *psvr);
		static void doacceptTask(socketTCP *psock);
	};
}//?namespace net4cpp21

#endif

