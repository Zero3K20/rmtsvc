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
		//send UDP proxy negotiation request (UDP proxy is only supported by the SOCKS5 proxy protocol)
		//returns the opened UDP port (>0) on success, otherwise failure
		//[out] svrIP/svrPort returns the port and IP address opened by the SOCKS service
		SOCKSRESULT UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout=-1);
		PROXYTYPE proxyType() const { return m_proxytype; }
		bool setProxy(PROXYTYPE proxytype,const char *proxyhost,int proxyport,const char *user,const char *pwd);
		void setProxy(socketProxy &socks);

	private: //disallow copy and assignment
		socketProxy(socketProxy &sockProxy){ return; }
		socketProxy & operator = (socketProxy &sockProxy) { return *this; }

	private:
		//send proxy connect request, returns SOCKSERR_OK on success (0)
		SOCKSRESULT sendReq_Connect(const char *host,int port,time_t lWaitout);
		//send proxy BIND request; returns SOCKSERR_OK on success (0)
		SOCKSRESULT sendReq_Bind(std::string &svrIP,int &svrPort,time_t lWaitout);
		//send UDP proxy negotiation request; returns SOCKSERR_OK on success (0) (UDP proxy is only supported by the SOCKS5 proxy protocol)
		SOCKSRESULT sendReq_UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout);
		//socks5negotiateauthentication,successreturntrueotherwisereturnfalse
		bool socks5_Negotiation(time_t lWaitout);
	private:
		PROXYTYPE m_proxytype;//proxy type
		std::string m_proxyhost;//proxy server host
		int m_proxyport;//proxy server port
		std::string m_proxyuser;//account for connecting to the proxy server
		std::string m_proxypwd;//password for connecting to the proxy server
		int m_dnsType;//domain name resolution mode: 0: server-side resolution, 1: local resolution, 2: try local resolution first, then try server-side resolution
			//for SOCKS4, only local-side resolution is possible; for HTTPS proxy, always server-side resolution

		cThread m_thread; //Bind command waiting for second response thread
	};
};//namespace net4cpp21

#endif


