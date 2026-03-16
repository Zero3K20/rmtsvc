/*******************************************************************
   *	socketdef.h
   *    DESCRIPTION:Definitions of constants, structures, and enums used by the socket class
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_CSOCKETDEF_H__
#define __YY_CSOCKETDEF_H__

//#define IPPROTO_IPV6

#ifdef WIN32 //Windows platform
	//socket errno
	#ifndef ENOTSOCK
	#define        ENOTSOCK        WSAEOPNOTSUPP
	#endif
	#ifndef ECONNRESET
	#define        ECONNRESET      WSAECONNRESET
	#endif
	#ifndef ENOTCONN
	#define        ENOTCONN        WSAENOTCONN
	#endif
	#ifndef EINPROGRESS
	#define	       EINPROGRESS	WSAEINPROGRESS
	#endif
	//#define      EBADF           WSAENOTSOCK
	//#define      EPIPE            WSAESHUTDOWN
	//#define	       MSG_NOSIGNAL    0  //not defined on Windows  //defined in sysconfig.h
	#define        SOCK_M_GETERROR WSAGetLastError() //get wrong code
			
	#define socklen_t int
	#pragma comment(lib,"ws2_32") //Statically link the ws2_32.dll import library
#elif defined MAC //Not yet supported
	//....
#else  //Unix/Linux platform
	extern "C"
	{
		//Include socket communication library headers
		#include <unistd.h>  //::close(fd) --- close socket
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <netdb.h>
	}//?extern "C"
	//Compilation on Sun OS 5.8 reports INADDR_NONE undefined (missing header), so define it manually
	#ifndef INADDR_NONE
		#define INADDR_NONE             ((unsigned long int) 0xffffffff)
	#endif
	
	#define closesocket close
	#define ioctlsocket ioctl	
	#define SOCK_M_GETERROR errno //get wrong code
	#define WSAEACCES     EACCES		
	#define WSADATA long
#endif

#ifndef SD_SEND
#define SD_RECEIVE 0x00		//Disallow further data reception 
#define SD_SEND 0x01		//Disallow further data sending 
#define SD_BOTH 0x02
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE   (u_int)(~SO_REUSEADDR)
#endif

#define SSENDBUFFERSIZE 1460 //Optimal send buffer size
							 //In SOCK_STREAM mode, if a single send exceeds 1460 bytes, the system splits it into multiple datagrams,
							//and the receiver gets a byte stream; the application must add framing logic. The value can be changed via
							//the registry, but Microsoft considers 1460 optimal and recommends against changing it. 

#define MAXRATIOTIMEOUT 600000 //us rate-limit delay 600ms
#define SCHECKTIMEOUT 200000 //us,select timeout 200ms

#define SOCKSRESULT int
#define SOCKSERR_OK 0
#define SOCKSERR_ERROR -1 //System error occurred; use getErrcode to get the error code
#define SOCKSERR_INVALID -2 //Invalid socket handle
#define SOCKSERR_CLOSED -3 //The peer has closed the connection
#define SOCKSERR_HOST -4 //Invalid hostname or hostname cannot be resolved
#define SOCKSERR_BIND -5 //Bind error
#define SOCKSERR_SETOPT -6 //Error calling setsockopt
#define SOCKSERR_CONN -7 //Connection failed
#define SOCKSERR_LISTEN -8 //Listen failed
#define SOCKSERR_SNIFF -9 //sniffer failed
#define SOCKSERR_ZEROLEN -10 //Send or receive returned 0

#define SOCKSERR_PARAM -11 //Invalid parameter
#define SOCKSERR_BUFFER -12 //Buffer error
#define SOCKSERR_TIMEOUT -13 //Timeout
#define SOCKSERR_EACCES	-14 //The specified address is a broadcast address but the broadcast flag is not set
#define SOCKSERR_THREAD -15 //Failed to create thread for task execution
#define SOCKSERR_NOTSUPPORT -16 //This feature is not supported
#define SOCKSERR_MEMORY -17 //Memory allocation error
#define SOCKSERR_SSLASSCIATE -18 //SSL handshake failed
#define SOCKSERR_PARENT -19 //Parent socket closed or errored

#define SOCKS_TCP_IN 1    //Indicates the TCP connection direction

#include <vector>
#include <string>
#include <ctime>

namespace net4cpp21
{
	typedef enum //Socket handle type
	{
		SOCKS_NONE,//Not created; invalid socket handle
		SOCKS_TCP,
		SOCKS_UDP,
		SOCKS_RAW //Raw socket
	}SOCKETTYPE;
	typedef enum //Socket connection status
	{
		SOCKS_ERROR, //TCP connection error/problem
		SOCKS_CLOSED, //Closed
		SOCKS_LISTEN, //TCP listening state
		SOCKS_CONNECTED,//TCP connection established
		SOCKS_OPENED//UDP or raw socket opened
	}SOCKETSTATUS;
	typedef enum
	{
		SOCKS_OP_PEEK,
		SOCKS_OP_READ,
		SOCKS_OP_WRITE,
		SOCKS_OP_ROOB,//Read OOB data
		SOCKS_OP_WOOB //Write OOB data
	}SOCKETOPMODE; //Socket operation mode
	typedef enum //SSL initialization type
	{
		SSL_INIT_NONE=0, //SSL not initialized
		SSL_INIT_SERV, //Server side
		SSL_INIT_CLNT  //Client side
	}SSL_INIT_TYPE;
	//Callback function type for blocking handling; if this function returns false, stop waiting (timeout)
	typedef bool (FUNC_BLOCK_HANDLER)(void *);

}//?namespace net4cpp21

#endif
