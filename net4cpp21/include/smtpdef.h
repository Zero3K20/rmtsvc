/*******************************************************************
   *	smtpdef.h
   *    DESCRIPTION:constants, structures and enum definitions for the SMTP protocol
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_SMTPDEF_H__
#define __YY_SMTPDEF_H__

#define SMTP_SERVER_PORT	25 //default SMTP service port
#define SMTP_MAX_PACKAGE_SIZE 128 //send/receive SMTP command buffer size
#define SMTP_MAX_RESPTIMEOUT 10 //s maximum response delay


#define SOCKSERR_SMTP_RESP -301 //command response error
#define SOCKSERR_SMTP_CONN SOCKSERR_SMTP_RESP-1 //connectserverfailure
#define SOCKSERR_SMTP_AUTH SOCKSERR_SMTP_RESP-2 //SMTP authentication failure
#define SOCKSERR_SMTP_SUPPORT SOCKSERR_SMTP_RESP-3 //unsupported SMTP authentication method
#define SOCKSERR_SMTP_RECIPIENT SOCKSERR_SMTP_RESP-4 //no email recipient specified
#define SOCKSERR_SMTP_FAILED SOCKSERR_SMTP_RESP-5 //general error
#define SOCKSERR_SMTP_EMLFILE SOCKSERR_SMTP_RESP-6 //invalid email format
#define SOCKSERR_SMTP_EMAIL SOCKSERR_SMTP_RESP-7 //invalid email address
#define SOCKSERR_SMTP_DNSMX SOCKSERR_SMTP_RESP-8 //MX domain name parse failure
#define SOCKSERR_SMTP_4XX   SOCKSERR_SMTP_RESP-9

typedef enum         //define supported SMTP authentication types
{
	SMTPAUTH_NONE=0,
	SMTPAUTH_LOGIN=1, //equivalent to PLAIN, only for compatibility with pre-standard SMTP authentication implementations. By default, this mechanism can only be used by SMTP
	SMTPAUTH_CRAM_MD5=2, //a challenge/response authentication mechanism, similar to APOP, but also suitable for use with other protocols. Defined in RFC 2195
	SMTPAUTH_DIGEST_MD5=4, //challenge/response authentication mechanism defined in RFC 2831
	SMTPAUTH_8BITMIME=8,
	SMTPAUTH_PLAIN=16 //PLAIN: this mechanism transmits the user's plaintext password over the network, making it easy to eavesdrop
}SMTPAUTH_TYPE;


#include <fstream>
#include <vector>
#include <string>
class mailMessage
{
	int m_contentType; //"text/plain" "text/html"
	std::string m_strSubject;
	std::string m_strBody;
	std::string m_strBodyCharset;//character set encoding for the email body; default is utf-8
	std::vector<std::string> m_attachs;//attachments to send

	std::string m_strName;//sender name
	std::string m_strFrom;//sender mailbox
	std::vector<std::pair<std::string,std::string> > m_vecTo; //recipients; first -- recipient email, second -- recipient nickname
	std::vector<std::pair<std::string,std::string> > m_vecCc; //CC; first -- recipient email, second -- recipient nickname
	std::vector<std::pair<std::string,std::string> > m_vecBc; //BCC; first -- recipient email, second -- recipient nickname
	
	std::string m_strMailFile;//generated mail body file pathname
	long m_lMailFileStartPos;//starting position of the mail body in the mail body file
	bool m_bDeleteFile;//whether to delete the m_strMailFile file when the object is released
public:
	enum RECIPIENT_TYPE { TO, CC, BCC };
	enum EMAILBODY_TYPE { TEXT_BODY,HTML_BODY };
	
	mailMessage():m_contentType(TEXT_BODY),m_strBodyCharset("utf-8"),m_bDeleteFile(false),m_lMailFileStartPos(0){}
	~mailMessage();
	const char *from() { return m_strFrom.c_str(); }
	std::string &body() { return m_strBody;}
	std::vector<std::pair<std::string,std::string> > & vecTo(RECIPIENT_TYPE rt=TO)
	{
		if(rt==CC)
			return m_vecCc;
		if(rt==BCC)
			return m_vecBc;
		return m_vecTo;
	}
	void setFrom(const char *from,const char *name)
	{
		if(from) m_strFrom.assign(from);
		if(name) m_strName.assign(name);
	}
	
	//set email subject and body
	void setBody(const char *strSubject,const char *strBody,EMAILBODY_TYPE bt=TEXT_BODY)
	{
		if(strSubject) m_strSubject.assign(strSubject);
		if(strBody) m_strBody.assign(strBody);
		m_contentType=bt; m_strMailFile="";
	}
	//add attachment
	bool AddAtach(const char *filename,const char *filepath,const char *contentID);
	//addrecipient
	bool AddRecipient(const char *email,const char *nick,RECIPIENT_TYPE rt=TO);
	//generate a Base64-encoded mail body file
	//bDelete -- indicates whether to delete the generated file when the mailMessage object is released
	const char * createMailFile(const char *file,bool bDelete);
	long MailFileStartPos() const { return m_lMailFileStartPos; }
	void setBody(const char *mailfile,long startPos)
	{
		m_strMailFile.assign(mailfile);
		m_bDeleteFile=false;
		m_lMailFileStartPos=startPos;
		return;
	}
	int initFromemlfile(const char *emlfile);
};

#endif

/* complete protocol trace
[56 202.108.9.193:25-->127.0.0.1:1159] 220 Coremail SMTP(Anti Spam) System (163c
om[20050206])
.
[10 127.0.0.1:1159-->202.108.9.193:25] EHLO yyc
.
[96 202.108.9.193:25-->127.0.0.1:1159] 250-smtp14
250-PIPELINING
250-AUTH LOGIN PLAIN NTLM
250-AUTH=LOGIN PLAIN NTLM
250 8BITMIME
.
[12 127.0.0.1:1159-->202.108.9.193:25] AUTH LOGIN
.
[18 202.108.9.193:25-->127.0.0.1:1159] 334 VXNlcm5hbWU6
.
[10 127.0.0.1:1159-->202.108.9.193:25] eXljbmV0
.
[18 202.108.9.193:25-->127.0.0.1:1159] 334 UGFzc3dvcmQ6
.
[10 127.0.0.1:1159-->202.108.9.193:25] cYY6d8N6
.
[31 202.108.9.193:25-->127.0.0.1:1159] 235 Authentication successful
.
[29 127.0.0.1:1159-->202.108.9.193:25] MAIL FROM: <yycnet@163.com>
.
[8 202.108.9.193:25-->127.0.0.1:1159] 250 Ok
.
[33 127.0.0.1:1159-->202.108.9.193:25] RCPT TO: <yycmail@263.sina.com>
.
[8 202.108.9.193:25-->127.0.0.1:1159] 250 Ok
.
[6 127.0.0.1:1159-->202.108.9.193:25] Data
.
[37 202.108.9.193:25-->127.0.0.1:1159] 354 End data with <CR><LF>.<CR><LF>
.
[662 127.0.0.1:1159-->202.108.9.193:25] From: "yyc" <yycnet@163.com>
To: yycmail@263.sina.com <yycmail@263.sina.com>
Subject: hi,test
Organization: xxxx
X-mailer: Foxmail 4.2 [cn]
Mime-Version: 1.0
Content-Type: text/plain;
      charset="GB2312"
Content-Transfer-Encoding: quoted-printable
Date: Mon, 8 Aug 2005 10:47:32 +0800

yycmail=A3=AC=C4=FA=BA=C3=A3=A1

=09         aaaaaaaaaaaaaaaaaa

=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=D6=C2
=C0=F1=A3=A1
 =09=09=09=09

=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1yyc
=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1yycnet@163.com
=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A12005-08-08


.
.
[38 202.108.9.193:25-->127.0.0.1:1159] 250 Ok: queued as 2cB_NXrE9kIrXhAE.1
.
[6 127.0.0.1:1159-->202.108.9.193:25] QUIT
.
[9 202.108.9.193:25-->127.0.0.1:1159] 221 Bye
.
*/
/*
211 system status or system help response 
   214 help info 
   220 service ready 
   221 service closing transfer channel 
   250 requested mail operation complete 
   251 user not local, will forward 
   354 start mail input, end with . 
   421 service not ready, closing transfer channel (when it must close, this response can be a reply to any command) 
   450 requested mail operation incomplete, mailbox unavailable (e.g., mailbox busy) 
   451 requested action aborted; error in processing 
   452 insufficient system storage, requested action not taken 
   500 format error, command unrecognized (this error also includes command line too long) 
   501 parameter format error 
   502 command not implemented 
   503 bad sequence of commands 
   504 command parameter not implemented 
   550 requested mail operation incomplete, mailbox unavailable (e.g., mailbox not found, or not accessible) 
   551 user not local, please try 
   552 exceeded storage allocation, requested action not taken 
   553 mailbox name unavailable, requested action not taken (e.g., mailbox format error) 
   554 operation failed 
*/

