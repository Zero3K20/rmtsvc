//sysconfig.h
#ifndef __YY_SYSCONFIG_H__
#define __YY_SYSCONFIG_H__

#ifdef WIN32 //Windows system platform
	#pragma warning(disable:4786)
	#pragma warning(disable:4503)
	#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#endif
	#ifndef _CRT_NONSTDC_NO_WARNINGS
	#define _CRT_NONSTDC_NO_WARNINGS
	#endif
	#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#endif
	#include <winsock2.h> //must be included before windows.h to avoid AF_IPX/AF_MAX macro redefinition (C4005)
	#include <ws2tcpip.h>
	#include <windows.h> //include Windows header files
	#define	MSG_NOSIGNAL    0  //this definition does not exist on Windows
	
	#define strcasecmpW _wcsicmp
	#define strncasecmpW _wcsnicmp
	#define vsnprintfW _vsnwprintf
	#define stringlenW wcslen
	#define strprintfW swprintf
	#define fileopenW _wfopen

	#ifdef UNICODE
	#define strcasecmp strcasecmpW
	#define strncasecmp strncasecmpW
	#define stringlen stringlenW
	#define strprintf strprintfW
	#define fileopen fileopenW
	#else	
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
	#define stringlen strlen
	#define strprintf sprintf
	#define fileopen fopen
	#endif
#elif defined MAC //temporarily not supported
	typedef unsigned short WCHAR;
	//...
#else  //unix/linux platform
	//Sun unix��û�ж���˳�����linux����/usr/include/bits/socket.h�ж����д˳���
	//EPIPE  The local end has been shut down on a connection oriented socket.  
	//In this case the  process  will  also receive a SIGPIPE unless MSG_NOSIGNAL is set.
	//�粻�����������³�����broken pipe����
	#define MSG_NOSIGNAL 0x4000	

	typedef wchar_t WCHAR;
	typedef unsigned __int64 DWORD64;
	typedef __int64 LONG64;
	
#endif //#ifdef WIN32 ...#else...

#endif //?#ifndef __YY_SYSCONFIG_H__

