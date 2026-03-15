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
		//sendsocks BIND请求到specified的socks服务上开一个侦听port
		//success返回开立的port>0 否则failure
		//[out] svrIP/svrPort返回socks服务开的port和IPaddress
		//[in] svrIP,告诉socks服务开的port等待并允许那个源IPconnect，svrPort无意义
		SOCKSRESULT Bind(std::string &svrIP,int &svrPort,time_t lWaitout=-1);
		bool WaitForBinded(time_t lWaitout,bool BindedEvent);
		//sendUDP代理协商请求 (仅仅socks5代理协议支持UDP代理)
		//success返回开立的UDPport>0 否则failure
		//[out] svrIP/svrPort返回socks服务开的port和IPaddress
		SOCKSRESULT UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout=-1);
		PROXYTYPE proxyType() const { return m_proxytype; }
		bool setProxy(PROXYTYPE proxytype,const char *proxyhost,int proxyport,const char *user,const char *pwd);
		void setProxy(socketProxy &socks);

	private: //禁止copy和赋值
		socketProxy(socketProxy &sockProxy){ return; }
		socketProxy & operator = (socketProxy &sockProxy) { return *this; }

	private:
		//send代理connect请求，success返回SOCKSERR_OK(0)
		SOCKSRESULT sendReq_Connect(const char *host,int port,time_t lWaitout);
		//send代理BIND请求，success返回SOCKSERR_OK(0)
		SOCKSRESULT sendReq_Bind(std::string &svrIP,int &svrPort,time_t lWaitout);
		//sendUDP代理协商请求，success返回SOCKSERR_OK(0) (仅仅socks5代理协议支持UDP代理)
		SOCKSRESULT sendReq_UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout);
		//socks5协商认证,success返回真否则返回假
		bool socks5_Negotiation(time_t lWaitout);
	private:
		PROXYTYPE m_proxytype;//代理type
		std::string m_proxyhost;//proxy service器主机
		int m_proxyport;//proxy service器port
		std::string m_proxyuser;//connectproxy service器的account
		std::string m_proxypwd;//connectproxy service器的password
		int m_dnsType;//域名parse方式 0:服务端parse 1：local端parse  2:先尝试local端parse，然后在尝试服务端parse
			//对于socks4只能local端parse,https代理方式总是server端parse

		cThread m_thread; //Bind命令等待第二次响应线程
	};
};//namespace net4cpp21

#endif


