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
		// function功能：connect to SMTP server and send the specified email. Returns SOCKSERR_OK on success
		SOCKSRESULT sendMail(mailMessage &mms,const char *smtpsvr,int smtpport);
		SOCKSRESULT sendMail_MX(mailMessage &mms,const char *dnssvr,int dnsport);
		//****************************************
		// function功能：direct mail delivery. Returns SOCKSERR_OK on success
		//emlfile : 邮件formatfile，两种formatfile. email body前!开头的为comment行
		//iffirst行为Email body is base64 encoded，则代表yessmtpsvrreceive的要forward的邮件
		//otherwise为user编辑要send的邮件,format:
		//FROM: <sender>\r\n
		//TO: <收件人>,<收件人>,...\r\n
		//Attachs: <attachment>,<attachment>,...\r\n
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
		SMTPAUTH_TYPE m_authType;//smtpserviceyesno需要authentication,目前仅仅支持LOGINauthentication方式
		std::string m_strAccount;//account and password for LOGIN authentication
		std::string m_strPwd;
		time_t m_lTimeout;//maximum wait timeout return in seconds
		std::vector<std::string> m_errors; //记录send时的error
		std::string m_ehloName;
	};
}//?namespace net4cpp21

#endif

