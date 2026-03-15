/*******************************************************************
   *	socketBase.h
   *    DESCRIPTION:Base class wrapper definition for socket
   *				Supports IPV4 and IPV6; define IPPROTO_IPV6 to enable IPV6 support
   *				To create an IPV6 socket, call SetIpv6(true);
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_CSOCKETBASE_H__
#define __YY_CSOCKETBASE_H__

#include "socketdef.h"

namespace net4cpp21
{
	class socketBase //Base class for socket operations
	{
	public:
		socketBase();
		socketBase(socketBase &sockB);
		socketBase & operator = (socketBase &sockB);
		virtual ~socketBase();
		virtual void Close();
		socketBase *parent() const { return m_parent; }
		void setParent(socketBase * psock) { m_parent=psock; return; }
		long getErrcode() const { return m_errcode;}
		int  getFlag() const { return m_sockflag; }
		SOCKETTYPE type() const { return m_socktype; }
		SOCKETSTATUS status() const { return m_sockstatus; }
		int getLocalPort() const { return ntohs(m_localAddr.sin_port); }
		int getRemotePort() const { return ntohs(m_remoteAddr.sin_port); }
		const char *getLocalIP() const { return inet_ntoa(m_localAddr.sin_addr); }
		const char *getRemoteIP() const { return inet_ntoa(m_remoteAddr.sin_addr); }
		unsigned long getLocalip() const { return m_localAddr.sin_addr.S_un.S_addr; }
		unsigned long getRemoteip() const { return m_remoteAddr.sin_addr.S_un.S_addr; }
		time_t getStartTime() const { return m_tmOpened; } //Returns the socket open start time
		SOCKSRESULT setLinger(bool bEnabled,time_t iTimeout=5); //s
		
		SOCKSRESULT setRemoteInfo(const char *host,int port);
		void SetRemoteInfo(unsigned long ipAddr,int port)
		{
			m_remoteAddr.sin_port=htons(port);
			m_remoteAddr.sin_addr.s_addr=ipAddr;
		}
		
		unsigned long getMaxSendRatio() const { return m_maxSendRatio; }
		unsigned long getMaxRecvRatio() const { return m_maxRecvRatio; }
		//Set send or receive rate limit in Bytes/s
		void setSpeedRatio(unsigned long sendRatio,unsigned long recvRatio)
		{
			m_maxSendRatio=sendRatio;
			m_maxRecvRatio=recvRatio;
			return;
		}
		//Check if this socket is readable/writable.
		//Returns 1 if readable or writable, 0 on timeout, negative on error (-1 means system error)
		int checkSocket(time_t wait_usec,SOCKETOPMODE opmode)
		{
			if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
			if(m_parent && m_parent->m_sockstatus<=SOCKS_CLOSED)
				return SOCKSERR_PARENT;
			int fd=m_sockfd;
			int iret=checkSocket(&fd,1,wait_usec,opmode);
			if(iret==SOCKET_ERROR) m_errcode=SOCK_M_GETERROR;
			return iret;
		}
		//Receive data (TCP/UDP/RAW); returns the received data length
		SOCKSRESULT Receive(char *buf,size_t buflen,time_t lWaitout)
		{
			return _Receive(buf,buflen,lWaitout,SOCKS_OP_READ);
		}
		SOCKSRESULT Peek(char *buf,size_t buflen,time_t lWaitout)
		{
			return _Receive(buf,buflen,lWaitout,SOCKS_OP_PEEK);
		}
		//Receive out-of-band data
		SOCKSRESULT RecvOOB(char *buf,size_t buflen,time_t lWaitout)
		{
			return _Receive(buf,buflen,lWaitout,SOCKS_OP_ROOB);
		}
		//Send data to the destination,Returns the size of data sent; if < 0 an error occurred
		SOCKSRESULT Send(size_t buflen,const char *buf,time_t lWaitout=-1);
		SOCKSRESULT Send(LPCTSTR fmt,...);
		SOCKSRESULT SendOOB(size_t buflen,const char *buf);
		//Get local host IP; returns the number of local IPs found
		static long getLocalHostIP(std::vector<std::string> &vec);
		static const char *getLocalHostIP();

		//Resolve the specified domain name, only for IPV4
		static unsigned long Host2IP(const char *host);
		static const char *IP2A(unsigned long ipAddr)
		{
			struct in_addr in;
			in.S_un.S_addr =ipAddr;
			return inet_ntoa(in);
		}

	protected:
		bool m_ipv6; //Whether IPV6 is supported
		time_t m_tmOpened;//Time when this socket was opened, used for rate limiting

		socketBase *m_parent; //Pointer to the associated parent socket
		long m_errcode;//Error code: system error code or custom error code
		int m_sockfd;//Socket access handle
		int m_sockflag; //Additional socket flags; bit 0 indicates TCP connection direction
		SOCKETTYPE m_socktype;//Socket handle type
		SOCKETSTATUS m_sockstatus;//Socket handle status
		SOCKADDR_IN m_localAddr;//Local port and IP bound to this socket
		SOCKADDR_IN m_remoteAddr;//Remote port and IP this socket is connected to (only for TCP)
							//For non-TCP sockets, stores the source port and IP of received data
							//or the destination port and IP for outgoing data
		unsigned long m_recvBytes;//Total bytes received
		unsigned long m_sendBytes;//Total bytes sent
		unsigned long m_maxSendRatio;//Maximum send rate in Bytes/second; 0 means unlimited
		unsigned long m_maxRecvRatio;//Maximum receive rate in Bytes/second; 0 means unlimited

		int getAF();
		int getSocketInfo();
		static int checkSocket(int *sockfds,size_t len,time_t wait_usec,SOCKETOPMODE opmode);
	protected:

		SOCKSRESULT setNonblocking(bool bNb);
		bool create(SOCKETTYPE socktype);
		SOCKSRESULT Bind(int port,BOOL bReuseAddr,const char *bindip);
		SOCKSRESULT Bind(int startport,int endport,BOOL bReuseAddr,const char *bindip);
		SOCKSRESULT _Receive(char *buf,size_t buflen,time_t lWaitout,SOCKETOPMODE opmode);
		SOCKSRESULT _Send(const char *buf,size_t buflen,time_t lWaitout);
		virtual size_t v_read(char *buf,size_t buflen);
		virtual size_t v_peek(char *buf,size_t buflen);
		virtual size_t v_write(const char *buf,size_t buflen);
		size_t v_writeto(const char *buf,size_t buflen,SOCKADDR_IN &addr);
	};

	//Initialize the Windows network environment----------------------------
	class NetEnv
	{
		WSADATA m_wsadata;
		bool m_bState;
	public:
		NetEnv();
		~NetEnv();
		bool getState(){return m_bState;}
		static NetEnv &getInstance();
	};
	//Initialize the Windows network environment----------------------------
}//?namespace net4cpp21

#endif