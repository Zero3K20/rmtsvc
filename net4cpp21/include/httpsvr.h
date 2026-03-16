/*******************************************************************
   *	httpsvr.h
   *    DESCRIPTION:HTTP protocol server declaration
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:
   *
   *	net4cpp 2.1
   *	HTTP/1.1 transfer protocol
   *******************************************************************/

#ifndef __YY_HTTP_SERVER_H__
#define __YY_HTTP_SERVER_H__

#include "httpdef.h"
#include "httpreq.h"
#include "httprsp.h"
#include "socketSvr.h"

namespace net4cpp21
{
	
	class httpSession
	{
		char m_sessionID[24];
		std::map<std::string,std::string> m_maps;
	public:
		time_t m_startTime;//start time when session was created
		time_t m_lastTime;//time of last access to this session
	public:
		static const long SESSION_VALIDTIME;
		static const char SESSION_IDNAME[];
		httpSession();
		virtual ~httpSession(){}
		bool isValid(time_t tNow) { return (tNow-m_lastTime)<SESSION_VALIDTIME; }
		const char *sessionID() { return m_sessionID;}
		bool SetSessionID(const char *strID);
		std::string& operator[](const std::string& key) { return m_maps[key]; }
	};

	class httpServer : public socketSvr
	{
	public:
		httpServer();
		virtual ~httpServer();
		//set web service root directory and virtual directories
		bool setvpath(const char *vpath,const char *rpath,long lAccess);
	protected:
		virtual bool onHttpReq(socketTCP *psock,httpRequest &httpreq,httpSession &session,
			std::map<std::string,std::string>& application,httpResponse &httprsp){ return false; }

		//triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock);
		//triggered when current connection count exceeds the configured maximum connections
		virtual void onTooMany(socketTCP *psock);
		virtual void onIdle(void);
	protected:
		void httprsp_fileNoFind(socketTCP *psock,httpResponse &httprsp);
		void httprsp_listDenied(socketTCP *psock,httpResponse &httprsp);
		void httprsp_accessDenied(socketTCP *psock,httpResponse &httprsp);
		void httprsp_listDir(socketTCP *psock,std::string &strPath,httpRequest &httpreq,httpResponse &httprsp);

		void httprsp_Redirect(socketTCP *psock,httpResponse &httprsp,const char *url);
		void httprsp_Redirect(socketTCP *psock,httpResponse &httprsp,const char *url,int iSeconds);
		void httprsp_NotModify(socketTCP *psock,httpResponse &httprsp);
		httpSession *GetSession(const char *sessionID)
		{ 
			if(sessionID==NULL) return NULL;
			std::map<std::string,httpSession *>::iterator it=m_sessions.find(sessionID);
			return (it!=m_sessions.end())?(*it).second:NULL;
		}
	private:
		long cvtVPath2RPath(std::string &vpath);

		cMutex m_mutex;
		std::map<std::string,httpSession *> m_sessions;
		std::map<std::string,std::string> m_application;

		std::map<std::string,std::pair<std::string,long> > m_dirAccess;//directory访问permissions
			//first --- string : http的虚directorypath，例如/ or /aa/，虚directorynot区分size写全部convert to lowercase
			//second --- pair : 此http虚directorycorresponding实际directoryanddirectory的访问permissions，实际directory必须为\结尾(winplatform)
	};
}//?namespace net4cpp21

#endif
