/*******************************************************************
   *	smtpclnt.h
   *    DESCRIPTION:SMTP protocol client declaration
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	Simple Mail Transfer Protocol (SMTP)
   *******************************************************************/

#ifndef __YY_SMTP_CLINET_H__
#define __YY_SMTP_CLINET_H__

#include "smtpdef.h"
#include "proxyclnt.h"

namespace net4cpp21
{
	
	class smtpClient : public socketProxy
	{
	public:
		smtpClient(const char *ehlo=NULL);
		virtual ~smtpClient();
		void setTimeout(time_t s){ if((m_lTimeout=s)<1) m_lTimeout=SMTP_MAX_RESPTIMEOUT; }
		//set SMTP service authentication type and account
		void setSMTPAuth(SMTPAUTH_TYPE authType,const char *strAccount,const char *strPwd);
		// 函数功能：connectsmtpserver ，sendspecified邮件。success返回SOCKSERR_OK
		SOCKSRESULT sendMail(mailMessage &mms,const char *smtpsvr,int smtpport);
		SOCKSRESULT sendMail_MX(mailMessage &mms,const char *dnssvr,int dnsport);
		//****************************************
		// 函数功能：邮件直投。success返回SOCKSERR_OK
		//emlfile : 邮件format文件，两种format文件. email body前!开头的为comment行
		//如果第一行为Email body is base64 encoded，则代表是smtpsvrreceive的要转发的邮件
		//否则为用户编辑要send的邮件,format:
		//FROM: <发件人>\r\n
		//TO: <收件人>,<收件人>,...\r\n
		//Attachs: <附件>,<附件>,...\r\n
		//Subject: <主题>\r\n
		//Bodytype: <TEXT|HTML>\r\n
		//\r\n
		//...
		//****************************************
		SOCKSRESULT sendMail(const char *emlfile,const char *smtpsvr,int smtpport,const char *from);
		SOCKSRESULT sendMail_MX(const char *emlfile,const char *dnssvr,int dnsport);
		std::vector<std::string> &errors() { return m_errors; }
	private:
		//connectspecified的smtpserver
		SOCKSRESULT ConnectSvr(const char *smtpsvr,int smtpport);
		SOCKSRESULT Auth_LOGIN();
		SOCKSRESULT _sendMail(mailMessage &mms,const char *toemail);
		bool sendCommand(int response_expected,char *buf,int buflen,int maxbuflen);

	private:
		SMTPAUTH_TYPE m_authType;//smtp服务是否需要authentication,目前仅仅支持LOGINauthentication方式
		std::string m_strAccount;//LOGINauthentication的account和password
		std::string m_strPwd;
		time_t m_lTimeout;//最大等待timeout返回s
		std::vector<std::string> m_errors; //记录send时的error
		std::string m_ehloName;
	};
}//?namespace net4cpp21

#endif

