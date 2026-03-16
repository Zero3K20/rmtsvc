/*******************************************************************
   *	cLogger.h
   *    DESCRIPTION:logging object
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CLOGGER_H__
#define __YY_CLOGGER_H__

#include <cstdio>
#include <string>
#include <map>

typedef bool (CB_LOG_OUTPUT)(const char *,long,long);
namespace net4cpp21
{
	typedef enum //debug output level
	{
		LOGLEVEL_NONE=-1,//do not output this message
		LOGLEVEL_DEBUG=0,//debug information
		LOGLEVEL_WARN,//output warning info
		LOGLEVEL_INFO,//output user log info
		LOGLEVEL_ERROR,//output error info
		LOGLEVEL_FATAL //output fatal error info
	}LOGLEVEL;
	typedef enum //log outputtype
	{
		LOGTYPE_NONE,
		LOGTYPE_STDOUT,//输出到标准输出设备上
		LOGTYPE_FILE,//output to file
		LOGTYPE_HWND,//输出到一个windows的窗体上
		LOGTYPE_SOCKET,//输出到一个socket管道
	}LOGTYPE;
	class cLogger
	{
		LOGTYPE m_logtype;//log outputtype
		LOGLEVEL m_loglevel;//log level
		long m_hout;//输出句柄，对于LOGTYPE_FILEtype对应FILE *,LOGTYPE_HWNDtype对应HWND，LOGTYPE_SOCKETtype对应socket句柄
		TCHAR m_fileopenType[4]; //default未覆盖open方式"w"
		bool m_bOutStdout;//yesnosynchronizeoutput to console
		bool m_bOutputTime;//yesno打印输出time
		CB_LOG_OUTPUT *m_pcallback;
		long	m_lcbparam; //callbackfunctionuserparameter
		static cLogger *plogger;//静态logclass实例pointer
		cLogger(); //not允许create此class实例
		
		void _dump(LPCTSTR buf,size_t len);
	public:
		~cLogger();
		static cLogger & getInstance(){ return *plogger; }
		//setyesno打印time
		void setPrintTime(bool bPrint){ m_bOutputTime=bPrint; return; }
		void setOutStdout(bool b) { m_bOutStdout=b; return; }
		LOGLEVEL setLogLevel(LOGLEVEL ll){ 
			LOGLEVEL lOld=m_loglevel; m_loglevel=ll; return lOld; }
		//setlogfileopen方式
		void setOpenfileType(TCHAR c) { m_fileopenType[0]=c; return; }
		void setCallback(CB_LOG_OUTPUT *pfunc,long param)
		{
			m_pcallback=pfunc; m_lcbparam=param;
		}
		//yesno允许输出specified级别的log
		bool ifOutPutLog(LOGLEVEL ll) { return ( (unsigned int)m_loglevel<=(unsigned int)ll ); }
		LOGTYPE LogType() { return m_logtype; }
		LOGTYPE setLogType(LOGTYPE lt,long lParam);
		void flush(){ 
			if(m_logtype==LOGTYPE_FILE && m_hout)
				::fflush((FILE *)m_hout); return;}
		void dump(LOGLEVEL ll,LPCTSTR fmt,...);
		void dump(LOGLEVEL ll,size_t len,LPCTSTR buf);
		//输出DEBUG级别的log
		void debug(LPCTSTR fmt,...);
		void debug(size_t len,LPCTSTR buf);
		void dumpBinary(const char *buf,size_t len);
		void dumpMaps(std::map<std::string,std::string> &maps,const char *desc);
		void printTime(); //打印currenttime
	};
}//?namespace net4cpp21

//setat打印message前先打印time
#define RW_LOG_SETPRINTTIME(b) \
{ \
	cLogger::getInstance().setPrintTime(b); \
}
//setlog output级别
#define RW_LOG_SETLOGLEVEL(ll) cLogger::getInstance().setLogLevel(ll);
//#define RW_LOG_SETLOGLEVEL(ll) \
//{ \
//	cLogger::getInstance().setLogLevel(ll); \
//}
//setlog outputcallback
#define RW_LOG_CALLBACK(pfunc,param) \
{ \
	cLogger::getInstance().setCallback(pfunc,param); \
}
//setlog output到specified的file
#define RW_LOG_SETFILE(filename) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_FILE,filename); \
}
//setfilelog为追加方式，而notyes覆盖方式
#define RW_LOG_OPENFILE_APPEND() \
{ \
	cLogger::getInstance().setOpenfileType('a'); \
}
//setlog output到specified的窗口
#define RW_LOG_SETHWND(hWnd) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_HWND,hWnd); \
}
//setlog output到specified的tcp socket
#define RW_LOG_SETSOCK(sockfd) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_SOCKET,sockfd); \
}
//setlog output到标准输出设备stdout
#define RW_LOG_SETSTDOUT() \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_STDOUT,0); \
}
//notoutput log
#define RW_LOG_SETNONE() \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_NONE,0); \
}

#define RW_LOG_FFLUSH() \
{ \
	cLogger::getInstance().flush(); \
}

#define RW_LOG_LOGTYPE cLogger::getInstance().LogType
//checkyesno允许输出specified级别的log
#define RW_LOG_CHECK cLogger::getInstance().ifOutPutLog
//log output
#define RW_LOG_PRINT cLogger::getInstance().dump
#define RW_LOG_PRINTBINARY cLogger::getInstance().dumpBinary
#define RW_LOG_PRINTMAPS cLogger::getInstance().dumpMaps
//打印currenttime
#define RW_LOG_PRINTTIME cLogger::getInstance().printTime
//setyesnosynchronizeoutput to console
#define RW_LOG_OUTSTDOUT cLogger::getInstance().setOutStdout

#define RW_LOG_DEBUG if(cLogger::getInstance().ifOutPutLog(LOGLEVEL_DEBUG)) cLogger::getInstance().debug
#endif


