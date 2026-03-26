/*******************************************************************
   *	cTelnet.h
   *    DESCRIPTION:Telnet class definition for Windows
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/
   
#ifndef __YY_CTELNET_H__
#define __YY_CTELNET_H__

#include "socketSvr.h"
	
namespace net4cpp21
{
	class cTelnet //telnet handler class
	{
		std::string m_telUser;
		std::string m_telPwd;
		bool m_bTelAuthentication;//whether this telnet service requires authentication
		long m_telClntnums;//number of currently connected telnet clients
	protected:
		std::string m_telHello;//prompt info when a client connects
		std::string m_telTip;//command line prompt
		char m_cmd_prefix; //extended command prefix

		virtual void onCommand(const char *strcmd,socketTCP *psock){ return ; }//received a user input command
		virtual bool onLogin(){ return false; }//a user Telnet login succeeded; if returns true, directly create a cmd shell
		void onConnect(socketTCP *psock);//a user has connected
	public:
		cTelnet();
		virtual ~cTelnet(){}
		//set the Telnet access account; if user==NULL, no authorization is required; otherwise authorization is required
		void setTelAccount(const char *user,const char *pwd);
		const char *getTelAccount() { return m_telUser.c_str(); }
		const char *getTelPassword() { return m_telPwd.c_str(); }
		bool bTelAuthentication() { return m_bTelAuthentication;}
		int telClntnums() { return m_telClntnums; }
		void setTelHello(const char *strHello){
			if(strHello) m_telHello.assign(strHello);
			return;
		}
		void setTelTip(const char *strTips){
			if(strTips) m_telTip.assign(strTips);
			return;
		}
	private:
		//returns true if no error occurred
		bool getInput(socketTCP *psock,std::string &strRet,int bEcho,int timeout);
	};
	
	class telServer : public socketSvr,cTelnet
	{
	public:
		telServer();
		virtual ~telServer();
	protected:
		//triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock){ cTelnet::onConnect(psock); }
		virtual bool onLogin(){ return true; }
	};

}//namespace net4cpp21

#endif
