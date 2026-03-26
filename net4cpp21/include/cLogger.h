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
		LOGTYPE_STDOUT,//output to the standard output device
		LOGTYPE_FILE,//output to file
		LOGTYPE_HWND,//output to a Windows window
		LOGTYPE_SOCKET,//output to a socket pipe
	}LOGTYPE;
	class cLogger
	{
		LOGTYPE m_logtype;//log outputtype
		LOGLEVEL m_loglevel;//log level
		long m_hout;//output handle; for LOGTYPE_FILE corresponds to FILE*, for LOGTYPE_HWND corresponds to HWND, for LOGTYPE_SOCKET corresponds to socket handle
		TCHAR m_fileopenType[4]; //default non-overwrite open mode "w"
		bool m_bOutStdout;//yesnosynchronizeoutput to console
		bool m_bOutputTime;//whether to print output time
		CB_LOG_OUTPUT *m_pcallback;
		long	m_lcbparam; //callbackfunctionuserparameter
		static cLogger *plogger;//pointer to static log class instance
		cLogger(); //not allowed to create an instance of this class
		
		void _dump(LPCTSTR buf,size_t len);
	public:
		~cLogger();
		static cLogger & getInstance(){ return *plogger; }
		//set whether to print time
		void setPrintTime(bool bPrint){ m_bOutputTime=bPrint; return; }
		void setOutStdout(bool b) { m_bOutStdout=b; return; }
		LOGLEVEL setLogLevel(LOGLEVEL ll){ 
			LOGLEVEL lOld=m_loglevel; m_loglevel=ll; return lOld; }
		//set log file open mode
		void setOpenfileType(TCHAR c) { m_fileopenType[0]=c; return; }
		void setCallback(CB_LOG_OUTPUT *pfunc,long param)
		{
			m_pcallback=pfunc; m_lcbparam=param;
		}
		//whether output of the specified log level is allowed
		bool ifOutPutLog(LOGLEVEL ll) { return ( (unsigned int)m_loglevel<=(unsigned int)ll ); }
		LOGTYPE LogType() { return m_logtype; }
		LOGTYPE setLogType(LOGTYPE lt,long lParam);
		void flush(){ 
			if(m_logtype==LOGTYPE_FILE && m_hout)
				::fflush((FILE *)m_hout); return;}
		void dump(LOGLEVEL ll,LPCTSTR fmt,...);
		void dump(LOGLEVEL ll,size_t len,LPCTSTR buf);
		//output DEBUG level log
		void debug(LPCTSTR fmt,...);
		void debug(size_t len,LPCTSTR buf);
		void dumpBinary(const char *buf,size_t len);
		void dumpMaps(std::map<std::string,std::string> &maps,const char *desc);
		void printTime(); //print current time
	};
}//?namespace net4cpp21

//set to print time before printing messages
#define RW_LOG_SETPRINTTIME(b) \
{ \
	cLogger::getInstance().setPrintTime(b); \
}
//set log output level
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
//set log output to the specified file
#define RW_LOG_SETFILE(filename) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_FILE,filename); \
}
//set file log to append mode instead of overwrite mode
#define RW_LOG_OPENFILE_APPEND() \
{ \
	cLogger::getInstance().setOpenfileType('a'); \
}
//set log output to the specified window
#define RW_LOG_SETHWND(hWnd) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_HWND,hWnd); \
}
//set log output to the specified TCP socket
#define RW_LOG_SETSOCK(sockfd) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_SOCKET,sockfd); \
}
//set log output to the standard output device stdout
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
//check whether output of the specified log level is allowed
#define RW_LOG_CHECK cLogger::getInstance().ifOutPutLog
//log output
#define RW_LOG_PRINT cLogger::getInstance().dump
#define RW_LOG_PRINTBINARY cLogger::getInstance().dumpBinary
#define RW_LOG_PRINTMAPS cLogger::getInstance().dumpMaps
//print current time
#define RW_LOG_PRINTTIME cLogger::getInstance().printTime
//set whether to synchronize output to console
#define RW_LOG_OUTSTDOUT cLogger::getInstance().setOutStdout

#define RW_LOG_DEBUG if(cLogger::getInstance().ifOutPutLog(LOGLEVEL_DEBUG)) cLogger::getInstance().debug
#endif


