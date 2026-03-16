/*******************************************************************
   *	socketBase.cpp
   *    DESCRIPTION:Base class wrapper definition for socket
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"
#include "include/socketBase.h"
#include "include/cLogger.h"

#include <cstdarg>
#include <cerrno>

using namespace std;
using namespace net4cpp21;

socketBase::socketBase()
{
	m_sockfd=INVALID_SOCKET;
	m_socktype=SOCKS_NONE;
	m_sockstatus=SOCKS_CLOSED;
	m_sockflag=0;
	m_ipv6=false;
	memset((void *)&m_remoteAddr,0,sizeof(SOCKADDR_IN));
	memset((void *)&m_localAddr,0,sizeof(SOCKADDR_IN));
	m_recvBytes=0;
	m_sendBytes=0;
	m_maxSendRatio=m_maxRecvRatio=0;
	m_tmOpened=0;
	m_errcode=SOCKSERR_OK;
	m_parent=NULL;
}

socketBase :: socketBase(socketBase &sockB)
{
	m_sockfd=sockB.m_sockfd;
	m_socktype=sockB.m_socktype;
	m_sockstatus=sockB.m_sockstatus;
	m_sockflag=sockB.m_sockflag;
	m_ipv6=sockB.m_ipv6;
	m_remoteAddr=sockB.m_remoteAddr;
	m_localAddr=sockB.m_localAddr;
	m_recvBytes=sockB.m_recvBytes;
	m_sendBytes=sockB.m_sendBytes;
	m_maxSendRatio=sockB.m_maxSendRatio;
	m_maxRecvRatio=sockB.m_maxRecvRatio;
	m_tmOpened=sockB.m_tmOpened;
	m_errcode=sockB.m_errcode;
	m_parent=sockB.m_parent;
	
	sockB.m_sockfd=INVALID_SOCKET;
	sockB.m_socktype=SOCKS_NONE;
	sockB.m_sockstatus=SOCKS_CLOSED;
	memset((void *)&sockB.m_localAddr,0,sizeof(SOCKADDR_IN));
}

socketBase& socketBase:: operator = (socketBase &sockB)
{
	m_sockfd=sockB.m_sockfd;
	m_socktype=sockB.m_socktype;
	m_sockstatus=sockB.m_sockstatus;
	m_sockflag=sockB.m_sockflag;
	m_ipv6=sockB.m_ipv6;
	m_remoteAddr=sockB.m_remoteAddr;
	m_localAddr=sockB.m_localAddr;
	m_recvBytes=sockB.m_recvBytes;
	m_sendBytes=sockB.m_sendBytes;
	m_maxSendRatio=sockB.m_maxSendRatio;
	m_maxRecvRatio=sockB.m_maxRecvRatio;
	m_tmOpened=sockB.m_tmOpened;
	m_errcode=sockB.m_errcode;
	m_parent=sockB.m_parent;
	
	sockB.m_sockfd=INVALID_SOCKET;
	sockB.m_socktype=SOCKS_NONE;
	sockB.m_sockstatus=SOCKS_CLOSED;
	memset((void *)&sockB.m_localAddr,0,sizeof(SOCKADDR_IN));
	return *this;
}

socketBase::~socketBase()
{
	Close(); 
}

void socketBase :: Close()
{
	if(m_socktype==SOCKS_NONE) return;
	m_socktype=SOCKS_NONE;
	m_sockflag=0;
	if ( m_sockfd!=INVALID_SOCKET ){
//If SO_LINGER is set (i.e., the l_onoff field in the linger struct is non-zero, see sections 2.4, 4.1.7 and 4.1.21) and a zero timeout is configured,
//closesocket() executes immediately without blocking, regardless of queued data. This is called a "hard" or "abortive" close,
//because the virtual circuit is reset immediately and unsent data is lost. The remote recv() call will fail with WSAECONNRESET.
//If SO_LINGER is set with a non-zero timeout, closesocket() blocks until all remaining data is sent or the timeout expires.
//This is called a "graceful" close. Note that if the socket is non-blocking and SO_LINGER has a non-zero timeout, closesocket() will
//return with WSAEWOULDBLOCK error.
//Windows defaults to SO_DONTLINGER, i.e., graceful close
//		::shutdown(m_sockfd,SD_BOTH); 
		::closesocket(m_sockfd);
		m_sockfd=INVALID_SOCKET;
	}//?if ( m_sockfd!=INVALID_SOCKET ){
	m_sockstatus=SOCKS_CLOSED;
	m_localAddr.sin_port=0;
	m_localAddr.sin_addr.s_addr=INADDR_ANY;
	m_recvBytes=m_sendBytes=0;
	//yyc remove 2006-02-15, do not clear remote address info on close
	//set in advance via setRemoteInfo to prevent it from being cleared on create
//	m_remoteAddr.sin_port=0;
//	m_remoteAddr.sin_addr.s_addr=INADDR_ANY;
	return;
}

//Set the remote host info for connecting or UDP sending
SOCKSRESULT socketBase :: setRemoteInfo(const char *host,int port)
{
	if(host==NULL || port<=0) return SOCKSERR_PARAM;
	//Check if the string is a valid IP address
	unsigned long ipAddr=socketBase::Host2IP(host);
	if(ipAddr==INADDR_NONE) return SOCKSERR_HOST;
	m_remoteAddr.sin_port=htons(port);
	m_remoteAddr.sin_addr.s_addr=ipAddr;
	return SOCKSERR_OK;
}

//Returns the size of data sent; if < 0 an error occurred
SOCKSRESULT socketBase :: Send(LPCTSTR fmt,...)
{
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_INVALID;
	TCHAR buf[2048];
	va_list args;
	va_start(args,fmt);
	int len=vsnprintf(buf,sizeof(buf)-1,fmt,args);
	va_end(args);
	if(len==-1) return SOCKSERR_BUFFER; //len=sizeof(buf)-1; 
	buf[len]=0;
	return (len>0)?_Send((const char *)buf,len*sizeof(TCHAR),-1):0;
}
SOCKSRESULT socketBase :: Send(size_t buflen,const char * buf,time_t lWaitout)
{
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_INVALID;
	if(buf==NULL) return SOCKSERR_PARAM;
	if(buflen==0) if( (buflen=strlen(buf))==0) return SOCKSERR_PARAM;
	return (buflen>0)?_Send((const char *)buf,buflen,lWaitout):0;
}
//Send out-of-band data
SOCKSRESULT socketBase :: SendOOB(size_t buflen,const char *buf)
{
	if(m_sockstatus!=SOCKS_CONNECTED) return SOCKSERR_INVALID;
	if(buf==NULL) return SOCKSERR_PARAM;
	if(buflen==0) if( (buflen=strlen(buf))==0) return SOCKSERR_PARAM;
	buflen=::send(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_OOB);
	if(buflen>=0)  return buflen; 
	
	m_errcode=SOCK_M_GETERROR;
	return SOCKSERR_ERROR;//A system error occurred; use SOCK_M_GETERROR to get the error code
}

//--------------------------static function---------------------------------------

//Get local host IP; returns the number of local IPs found
long socketBase :: getLocalHostIP(vector<string> &vec)
{
	char buf[64];
	gethostname(buf,sizeof(buf));
	struct hostent * p=gethostbyname(buf);
	if(p==NULL) return 0;
	int i;
	for(i=0;p->h_addr_list[i];i++)
		vec.push_back( (char *)inet_ntoa(*((struct in_addr *)p->h_addr_list[i])) );
	return i;
}
const char *socketBase :: getLocalHostIP()
{
	char buf[64];
	gethostname(buf,sizeof(buf));
	struct hostent * p=gethostbyname(buf);
	if(p==NULL) return 0;
	return inet_ntoa(*((struct in_addr *)p->h_addr_list[0]));
}

//Resolve the specified domain name, only for IPV4
unsigned long socketBase :: Host2IP(const char *host)
{
	unsigned long ipAddr=inet_addr(host);
	if(ipAddr!=INADDR_NONE) return ipAddr;
	//The specified string is not a valid IP address; may be a hostname
	struct hostent * p=gethostbyname(host);
	if(p==NULL) return INADDR_NONE;
	ipAddr=(*((struct in_addr *)p->h_addr)).s_addr;
	return ipAddr;
}

//On Windows, the default FD_SETSIZE is only 64!!! The number of sockets to check in checkSocket must not exceed this value
//!!!!!#define FD_SETSIZE      64 
//Check whether the specified sockets are readable or writable
//sockfds --- array of socket handles to check; len --- number of socket handles
//wait_usec --- timeout wait time in microseconds
//opmode --- check if socket is readable/writable/has OOB data
//Returns --- the number of readable/writable socket handles
//	--- if the return value is less than 0, an error occurred
//	This function modifies the sockfds array: 1 if readable/writable, 0 otherwise
//Differences in FD_SET implementation between Windows/Linux/Unix:
//On Windows the default FD_SETSIZE is 64; the FD_SET macro iterates the SOCKETs array: if it finds
//the fd handle already in SOCKETs it exits; otherwise appends the fd and increments the count
//On Linux/Unix the default FD_SETSIZE is 65535; the FD_SET macro inserts the socket handle at the index equal to fd
//in the SOCKETs array. Therefore on Linux/Unix check fd<FD_SETSIZE before calling FD_SET.
//On Linux/Unix a socket fd value >= FD_SETSIZE is invalid
int socketBase :: checkSocket(int *sockfds,size_t len,time_t wait_usec,SOCKETOPMODE opmode)
{
	int i,retv=0;
	struct timeval to;
	fd_set fds; FD_ZERO(&fds);
	for(i=0;i<(int)len;i++)
	{
		FD_SET(sockfds[i], &fds);
	}
	if ( wait_usec>0)
    {
		to.tv_sec =(long)(wait_usec/1000000L);
		to.tv_usec =wait_usec%1000000L;
    }
	else
	{
		to.tv_sec = 0;
		to.tv_usec = 0;
	}
	if(opmode<=SOCKS_OP_READ) //Check if data is available for reading
		retv = select(FD_SETSIZE, &fds, NULL, NULL, &to);
	else if(opmode==SOCKS_OP_WRITE) //Check if socket is writable
		retv = select(FD_SETSIZE, NULL, &fds, NULL, &to);
	else if(opmode==SOCKS_OP_ROOB) //Check if OOB data is available
		retv = select(FD_SETSIZE, NULL, NULL, &fds, &to);
	else return 1; //Other operations always return true, e.g., writing OOB data!!!!!
	if(retv==0) return 0; // timeout occurred
	if(retv==SOCKET_ERROR){ //An error occurred
		if(SOCK_M_GETERROR==EINPROGRESS) return 0;//Operation in progress, not an error
	}
	//For other errors, return directly and let the caller handle them
	//A handle is readable or writable
	for(i=0;i<(int)len;i++)
	{
		sockfds[i]=(FD_ISSET(sockfds[i], &fds))?1:0;
	}
	return retv;
}

//--------------------------private function---------------------------------------

inline int socketBase :: getAF()
{
	int af=AF_INET;
#ifdef IPPROTO_IPV6
	if(m_ipv6) af=AF_INET6;
#endif
	return af;
}

//Get the local IP and port bound to this socket; returns the bound local port on success
//Returns <= 0 if an error occurred
int socketBase :: getSocketInfo()
{
//	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	//getsockname() retrieves the local name of a bound or connected socket s; the local address is returned.
	//This call is especially useful when connect() is called without a prior bind():
	//only getsockname() can reveal the system-assigned local address.
	//On return, the namelen parameter contains the actual byte length of the name.
	//If a socket is bound to INADDR_ANY (meaning it accepts any host address),
	//getsockname() will not return any host IP address information unless connect() or accept() has been called.
	//Unless the socket is connected, Windows socket applications must not assume the IP address will change from INADDR_ANY.
	//In a multi-homed environment, the IP address used by an unconnected socket is indeterminate.
	int addr_len=sizeof(m_localAddr);
	m_localAddr.sin_family=getAF();
	m_localAddr.sin_port=0; m_localAddr.sin_addr.s_addr=0;
	getsockname(m_sockfd,(struct sockaddr *) &m_localAddr,(socklen_t *)&addr_len);
	return ntohs(m_localAddr.sin_port);
}

//Bind the specified port and IP address
//bReuse -- specifies whether the port can be reused; bindip --- IP to bind to; NULL/"" binds to all interfaces
//Returns the actually bound port (>0) on success
SOCKSRESULT socketBase :: Bind(int port,BOOL bReuseAddr,const char *bindip)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	if(bReuseAddr==SO_REUSEADDR){//Binding with port reuse allowed
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on,sizeof(on));
	}else if(bReuseAddr==SO_EXCLUSIVEADDRUSE){ //Binding with port reuse forbidden
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &on,sizeof(on));
	}//Otherwise port reuse is not configured; this binding uses the default reuse setting

	SOCKADDR_IN addr; memset(&addr, 0, sizeof(addr));
	addr.sin_family = getAF();
	addr.sin_port =(port<=0)?0:htons(port);
	if(bindip && bindip[0]!=0) addr.sin_addr.s_addr=inet_addr(bindip);
	//Otherwise bind to INADDR_ANY (all local interfaces)

	if (bind(m_sockfd, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR ) 
	{
		m_errcode=SOCK_M_GETERROR;
		return SOCKSERR_BIND;
	}
	return getSocketInfo();
}
//Bind a port randomly selected from [startport, endport]
//startport>=0, endport<=65535. If endport<=0 then endport=65535
//Returns the actually bound port
SOCKSRESULT socketBase :: Bind(int startport,int endport,BOOL bReuseAddr,const char *bindip)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	if(bReuseAddr==SO_REUSEADDR){//Binding with port reuse allowed
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on,sizeof(on));
	}else if(bReuseAddr==SO_EXCLUSIVEADDRUSE){ //Binding with port reuse forbidden
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &on,sizeof(on));
	}//Otherwise port reuse is not configured; this binding uses the default reuse setting
	
	SOCKADDR_IN addr; memset(&addr, 0, sizeof(addr));
	addr.sin_family = getAF();
	if(bindip && bindip[0]!=0) addr.sin_addr.s_addr=inet_addr(bindip);
	//Otherwise bind to INADDR_ANY (all local interfaces)
	if(startport<1) startport=1;
	if(endport<1 || endport>65535) endport=65535;
	if(endport<startport){ int iswap=startport; startport=endport; endport=iswap; }
	
	int icount=endport-startport;
	int i;
	if(icount<10)
	{
		for(i=0;i<=icount;i++)
		{
			addr.sin_port=htons(startport+i);
			if (bind(m_sockfd, (struct sockaddr *) &addr, sizeof(addr)) != SOCKET_ERROR )
				break;
		}
		if(i>icount) return SOCKSERR_BIND;
	}
	else
	{
		for(i=1;i<=10;i++)
		{
			int port=startport+rand()*icount/RAND_MAX;
			addr.sin_port=htons(port);
			if (bind(m_sockfd, (struct sockaddr *) &addr, sizeof(addr)) != SOCKET_ERROR )
				break;
		}
		if(i>10) return SOCKSERR_BIND;
	}
	return getSocketInfo();
}
//Enable/disable SO_LINGER. If enabled, closing the socket must wait until buffered data is sent
//iTimeout seconds --- if SO_LINGER is enabled, the maximum time to wait before forcing close
SOCKSRESULT socketBase::setLinger(bool bEnabled,time_t iTimeout)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_OK;
	int sr=SOCKSERR_OK;
	//Enable SO_LINGER
	//When  enabled,  a  close(2) or shutdown(2) will not return until all queued messages for the socket have
    //      been successfully sent or the linger timeout has been reached. Otherwise, the call  returns  immediately
    //     and  the  closing  is  done  in the background.  When the socket is closed as part of exit(2), it always
    //      lingers in the background.
	struct linger lg;
	if(bEnabled)
		lg.l_onoff=1;
	else
		lg.l_onoff=0;
	lg.l_linger=(u_short)iTimeout;//Set maximum timeout in seconds
	if(setsockopt(m_sockfd,SOL_SOCKET,SO_LINGER,(const char *)&lg,sizeof(lg))!=0)
		return SOCKSERR_SETOPT;
	return SOCKSERR_OK;
}
//Set socket to blocking/non-blocking mode
//Returns SOCKSERR_OK on success
SOCKSRESULT socketBase::setNonblocking(bool bNb)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	int sr=SOCKSERR_OK;
#ifdef WIN32
	unsigned long l = bNb ? 1 : 0; //1 -- non-blocking
	sr=ioctlsocket(m_sockfd,FIONBIO,&l); //Set socket to blocking/non-blocking mode
	if(sr!=0) sr=SOCKSERR_ERROR;
#else
	if (bNb)
	{
		sr=fcntl(s, F_SETFL, O_NONBLOCK);
	}
	else
	{
		sr=fcntl(s, F_SETFL, 0);
	}
#endif
	//A system error occurred; use SOCK_M_GETERROR to get the error code
	if(sr==SOCKSERR_ERROR) m_errcode=SOCK_M_GETERROR;
	return sr;
}

//Create a socket handle of the specified type
//Returns true on success, false otherwise
bool socketBase :: create(SOCKETTYPE socktype) 
{
	if( type()!=SOCKS_NONE ) Close();
	m_localAddr.sin_family=getAF();
	m_remoteAddr.sin_family=getAF();
	int af=m_localAddr.sin_family;
	switch(socktype)
	{
		case SOCKS_TCP:
			if( (m_sockfd=::socket(af,SOCK_STREAM,0))!=INVALID_SOCKET )
				m_socktype=SOCKS_TCP;
			break;
		case SOCKS_UDP:
			if( (m_sockfd=::socket(af,SOCK_DGRAM,0))!=INVALID_SOCKET )
			{
				m_socktype=SOCKS_UDP;
				int on=1; //Allows transmission of broadcast messages on the socket.
						// Enable broadcast; destination 255.255.255.255 sends to all machines on the local subnet
				setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST, (char *) &on,sizeof(on));
			}
			break;
		case SOCKS_RAW:
			break;
	}//?switch(socktype)
	if(m_socktype==SOCKS_NONE) return false;
	m_tmOpened=time(NULL);
	return true;
}
//Note: for message-oriented sockets, a single send must not exceed the socket send buffer size, otherwise SOCKET_ERROR is returned
//The system default socket send buffer size is SO_MAX_MSG_SIZE 
/*inline size_t socketBase :: v_write(const char *buf,size_t buflen)
{
	size_t len=0;
	if(m_socktype==SOCKS_TCP)
		len=::send(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	else
		len=::sendto(m_sockfd,buf,buflen,MSG_NOSIGNAL,(struct sockaddr *)&m_remoteAddr,sizeof(SOCKADDR_IN));
	return len;
}*/
inline size_t socketBase :: v_write(const char *buf,size_t buflen)
{
	return ::send(m_sockfd,buf,buflen,MSG_NOSIGNAL);
}
inline size_t socketBase :: v_writeto(const char *buf,size_t buflen,SOCKADDR_IN &addr)
{
	return ::sendto(m_sockfd,buf,buflen,MSG_NOSIGNAL,(struct sockaddr *)&addr,sizeof(SOCKADDR_IN));
}

inline size_t socketBase :: v_read(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_socktype==SOCKS_TCP)
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	else
	{
		m_remoteAddr.sin_addr.S_un.S_addr=INADDR_ANY;
		m_remoteAddr.sin_port=0;
		int addrlen=sizeof(m_remoteAddr);
		len=::recvfrom(m_sockfd,buf,buflen,MSG_NOSIGNAL,
			(struct sockaddr *) &m_remoteAddr, (socklen_t *)&addrlen);
	}	
	return len;
}
inline size_t socketBase :: v_peek(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_socktype==SOCKS_TCP)
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_PEEK);
	else
	{
		m_remoteAddr.sin_addr.S_un.S_addr=INADDR_ANY;
		m_remoteAddr.sin_port=0;
		int addrlen=sizeof(m_remoteAddr);
		len=::recvfrom(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_PEEK,
			(struct sockaddr *) &m_remoteAddr, (socklen_t *)&addrlen);
	}
		
	return len;
}

//Receive data packet
//Returns the length of received data; if < 0 an error occurred
//If return == 0, the receive rate exceeds the configured rate limit; pause receiving
//bPeek --- whether to only peek at data without removing it from the receive buffer
//==-1 indicates a system error; use SOCK_M_GETERROR to get the error code. SOCKETOPMODE
SOCKSRESULT socketBase :: _Receive(char *buf,size_t buflen,time_t lWaitout,SOCKETOPMODE opmode)
{
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_INVALID;
	if(buf==NULL || buflen==0) return SOCKSERR_PARAM;
	int iret=1; //whether data is available for reading
	if(lWaitout>=0)
	{
		time_t t=time(NULL);
		while( (iret=checkSocket(SCHECKTIMEOUT,opmode))== 0 )
		{//Check if the handle is readable
			if( (time(NULL)-t)>lWaitout ) break; //Check if timeout has been reached
			if(m_parent && m_parent->m_sockstatus<=SOCKS_CLOSED) 
				return SOCKSERR_PARENT;
		}//?while
	}//?if(lWaitout>=0)
	if(iret!=1)
	{
		if(iret==-1) {m_errcode=SOCK_M_GETERROR; return SOCKSERR_ERROR;}
		return SOCKSERR_TIMEOUT; //Timeout
	}
	//When the peer gracefully closes, recvfrom returns 0 immediately regardless of buffered data
	//recv continues to retrieve buffered data if available, otherwise also returns 0
	if(opmode==SOCKS_OP_PEEK)
		iret=v_peek(buf,buflen);
	else if(opmode==SOCKS_OP_READ)
	{
		if(m_maxRecvRatio!=0) //If receive rate limiting is active, check whether the rate is exceeded
		{
			time_t tNow=time(NULL);
			if( (m_recvBytes/(tNow-m_tmOpened+1))>m_maxRecvRatio) return 0; //Rate limit exceeded
		}//?if(m_maxRecvRatio!=0)
		if( (iret=v_read(buf,buflen))>0 ) m_recvBytes+=iret;
	}
	else if(opmode==SOCKS_OP_ROOB && m_socktype==SOCKS_TCP) 
	{//Read out-of-band data, only for TCP
		iret=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_OOB);
	}
	else return 0;
	if(iret>0) return iret;

	if(iret==0) return SOCKSERR_CLOSED; //If return value is 0, the connection has been closed by the peer (only for TCP)
	m_errcode=SOCK_M_GETERROR;
	return SOCKSERR_ERROR;//A system error occurred; use SOCK_M_GETERROR to get the error code 
}

//Send data to the destination
//Returns the size of data sent; if < 0 an error occurred
//If return == 0, the send rate exceeds the configured rate limit; pause sending
//For non-TCP connections, call setRemoteInfo to set the destination host and port before sending.
//Note: for non-TCP connections, Receive writes the source IP and port of received data into the remote address fields.
//
//When called, send first compares the data length len with the socket s send buffer size,
//if len exceeds the send buffer size SOCKET_ERROR is returned; if len <= the send buffer size,
//send checks whether the protocol is currently sending buffered data; if so it waits until done,
//if the protocol has not started sending or the buffer is empty, send compares the remaining send
//buffer space with len; if len exceeds remaining space, send waits for the protocol to finish sending
//the buffer; if len <= remaining space, send copies buf into remaining space (note: send does not
//transmit data to the other end -- the protocol does; send only copies buf into the send buffer
//remaining space). If send successfully copies the data, it returns bytes copied; if copy fails,
//send returns SOCKET_ERROR; if the network disconnects while send waits for the protocol, send
//also returns SOCKET_ERROR. Note that after send copies buf data into the send buffer remaining space,
//it returns, but the data is not necessarily transmitted to the other end immediately.
//Note: on Unix, if the network disconnects while send is waiting for the protocol, the calling process receives a
//SIGPIPE signal; the default handler for this signal terminates the process.
inline SOCKSRESULT socketBase :: _Send(const char *buf,size_t buflen,time_t lWaitout)
{
	SOCKADDR_IN addr; memcpy((void *)&addr,(const void *)&m_remoteAddr,sizeof(addr));
	//Return error if no destination is specified for UDP
//	if(m_socktype==SOCKS_UDP && m_remoteAddr.sin_port==0) return SOCKSERR_HOST;
	
	time_t tNow=time(NULL);
	if(m_maxSendRatio!=0) //If send rate limiting is active, check whether the rate is exceeded
		if( (m_sendBytes/(tNow-m_tmOpened+1))>m_maxSendRatio) return 0; //Rate limit exceeded
	while(lWaitout>=0)
	{
		int iret=checkSocket(SCHECKTIMEOUT,SOCKS_OP_WRITE);
		if(iret<0) { m_errcode=SOCK_M_GETERROR; return SOCKSERR_ERROR; }
		if(iret>0) break; //socket is writable
		if(m_parent && m_parent->m_sockstatus<=SOCKS_CLOSED) 
			return SOCKSERR_PARENT;
		if( (time(NULL)-tNow)>lWaitout ) return SOCKSERR_TIMEOUT; //Timeout
	}//?while
	
	int iret=(m_socktype==SOCKS_UDP)?v_writeto(buf,buflen,addr):v_write(buf,buflen);
	if(iret>=0) { m_sendBytes+=iret; return iret; }//Return the actual number of bytes sent
	
	m_errcode=SOCK_M_GETERROR;
	if(iret==SOCKET_ERROR && m_errcode==WSAEACCES) 
		return SOCKSERR_EACCES; //The specified address is a broadcast address but the broadcast flag is not set
	//Otherwise iret should be -1, indicating send/sendto encountered an error
	return SOCKSERR_ERROR;//A system error occurred; use SOCK_M_GETERROR to get the error code 
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
NetEnv netEnv;

NetEnv::NetEnv()
{
	m_bState=true;
#ifdef WIN32
	if(WSAStartup(MAKEWORD(2,2),&m_wsadata)!=0)
		m_bState=false;
#endif
}

NetEnv::~NetEnv()
{
#ifdef WIN32
	if(m_bState) WSACleanup();
#endif
}

NetEnv &getInstance(){ return netEnv; }
