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
		long m_telClntnums;//currentconnect的telnetclient个数
	protected:
		std::string m_telHello;//当有clientconnect上后的提示info
		std::string m_telTip;//命令输入行提示符
		char m_cmd_prefix; //扩展命令前缀

		virtual void onCommand(const char *strcmd,socketTCP *psock){ return ; }//收到用户输入命令
		virtual bool onLogin(){ return false; }//有一个用户telnet登录success,返回真则直接createcmd shell
		void onConnect(socketTCP *psock);//有一个用户connect上来
	public:
		cTelnet();
		virtual ~cTelnet(){}
		//设置telnet的访问account,如果user==NULL则此无需授权访问,否则需要授权访问
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
		//如果没有error发生则返回真
		bool getInput(socketTCP *psock,std::string &strRet,int bEcho,int timeout);
	};
	
	class telServer : public socketSvr,cTelnet
	{
	public:
		telServer();
		virtual ~telServer();
	protected:
		//当有一个新的客户connect此服务触发此函数
		virtual void onAccept(socketTCP *psock){ cTelnet::onConnect(psock); }
		virtual bool onLogin(){ return true; }
	};

}//namespace net4cpp21

#endif
