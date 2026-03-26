/*******************************************************************
   *	smtpsvr.h
   *    DESCRIPTION:SMTP protocol server declaration
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	Simple Mail Transfer Service (SMTP)
   *******************************************************************/

#ifndef __YY_SMTP_SERVER_H__
#define __YY_SMTP_SERVER_H__

#include "smtpdef.h"
#include "socketSvr.h"

namespace net4cpp21
{
	
	class smtpServer : public socketSvr
	{
		class cSmtpSession
		{
		public:
			bool m_bAccess; //whether this client session has passed authentication
			time_t m_tmLogin;//login time
			std::string m_ehlo;
			std::string m_fromemail;//email sender
			std::vector<std::string> m_recp;//email recipient
			cSmtpSession():m_bAccess(false){}
			~cSmtpSession(){}
		};
	public:
		smtpServer();
		virtual ~smtpServer();
		//set path for receiving email
		const char * setRecvPath(const char *recvpath)
		{
			if(recvpath)
				m_receivedpath.assign(recvpath);
			if(m_receivedpath!="" && m_receivedpath[m_receivedpath.length()-1]!='\\')
				m_receivedpath.append("\\");
			return m_receivedpath.c_str();
		}
//		void setHelloTip(const char *strTip){
//			if(strTip) m_helloTip.assign(strTip);
//			return;
//		}
	protected:
		virtual bool onAccess(const char *struser,const char *strpwd) {return true;}
		virtual void onReceive(const char *emlfile,cSmtpSession &clientSession)
		{
			return;
		}
	private:
		//triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock);
		//triggered when current connection count exceeds the configured maximum connections
		virtual void onTooMany(socketTCP *psock);

		void parseCommand(cSmtpSession &clientSession,socketTCP *psock,const char *ptrCommand);
		void docmd_ehlo(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_auth(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_mailfrom(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_rcptto(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_data(cSmtpSession &clientSession,socketTCP *psock);
		void docmd_quit(socketTCP *psock);
		void resp_unknowed(socketTCP *psock);
		void resp_OK(socketTCP *psock);
	private:
		SMTPAUTH_TYPE m_authType;//whether the SMTP service requires authentication
		std::string m_receivedpath;//path to store received emails, ending with \
		std::string m_helloTip;
	};
}//?namespace net4cpp21

#endif
