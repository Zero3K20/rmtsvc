/*******************************************************************
   *	proxyclnt.h
   *    DESCRIPTION:proxy protocol client declaration
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	supports HTTP, SOCKS4, SOCKS5
   *******************************************************************/

#ifndef __YY_PROXY_CLINET_H__
#define __YY_PROXY_CLINET_H__

#include "proxydef.h"
#include "socketTcp.h"
#include "cThread.h"

namespace net4cpp21
{
	class socketProxy : public socketTCP
	{
	public:
		socketProxy();
		virtual ~socketProxy();
		//connect to specified TCP service. Returns local socket port on success, error otherwise
		virtual SOCKSRESULT Connect(const char *host,int port,time_t lWaitout=-1);
		//send SOCKS BIND request to the specified SOCKS service to open a listening port
		//returns the opened port (>0) on success, otherwise failure
		//[out] svrIP/svrPort returns the port and IP address opened by the SOCKS service
		//[in] svrIP, tells the SOCKS service which source IP is allowed to connect to the opened port; svrPort is meaningless
		SOCKSRESULT Bind(std::string &svrIP,int &svrPort,time_t lWaitout=-1);
		bool WaitForBinded(time_t lWaitout,bool BindedEvent);
		//sendUDP代理negotiaterequest (UDP proxy is only supported by the SOCKS5 proxy protocol)
		//successreturn开立的UDPport>0 otherwisefailure
		//[out] svrIP/svrPort returns the port and IP address opened by the SOCKS service
		SOCKSRESULT UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout=-1);
		PROXYTYPE proxyType() const { return m_proxytype; }
		bool setProxy(PROXYTYPE proxytype,const char *proxyhost,int proxyport,const char *user,const char *pwd);
		void setProxy(socketProxy &socks);

	private: //禁止copyand赋值
		socketProxy(socketProxy &sockProxy){ return; }
		socketProxy & operator = (socketProxy &sockProxy) { return *this; }

	private:
		//send proxy connect request，returns SOCKSERR_OK on success(0)
		SOCKSRESULT sendReq_Connect(const char *host,int port,time_t lWaitout);
		//send代理BINDrequest，returns SOCKSERR_OK on success(0)
		SOCKSRESULT sendReq_Bind(std::string &svrIP,int &svrPort,time_t lWaitout);
		//sendUDP代理negotiaterequest，returns SOCKSERR_OK on success(0) (UDP proxy is only supported by the SOCKS5 proxy protocol)
		SOCKSRESULT sendReq_UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout);
		//socks5negotiateauthentication,successreturntrueotherwisereturnfalse
		bool socks5_Negotiation(time_t lWaitout);
	private:
		PROXYTYPE m_proxytype;//代理type
		std::string m_proxyhost;//proxy service器主机
		int m_proxyport;//proxy service器port
		std::string m_proxyuser;//connectproxy service器的account
		std::string m_proxypwd;//connectproxy service器的password
		int m_dnsType;//domain nameparse方式 0:server-sideparse 1：local端parse  2:先尝试local端parse，thenat尝试server-sideparse
			//对于socks4只能local端parse,https代理方式totalyesserver端parse

		cThread m_thread; //Bindcommandwaiting第二次responsethread
	};
};//namespace net4cpp21

#endif


