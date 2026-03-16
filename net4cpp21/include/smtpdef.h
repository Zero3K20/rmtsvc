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

typedef enum         //define支持的SMTPauthenticationtype
{
	SMTPAUTH_NONE=0,
	SMTPAUTH_LOGIN=1, //等效于 PLAIN，只为了与 SMTP authentication的预标准implementation相兼容。缺省情况下，此机制仅可由 SMTP 使用
	SMTPAUTH_CRAM_MD5=2, //一种询问/responseauthentication机制，class似于 APOP，但也适合与其他protocol配合使用。at RFC 2195 中已define
	SMTPAUTH_DIGEST_MD5=4, //RFC 2831 中define的询问/responseauthentication机制
	SMTPAUTH_8BITMIME=8,
	SMTPAUTH_PLAIN=16 //PLAIN 此机制通过network传递user的纯文本口令，atnetwork上很容易窃听口令
}SMTPAUTH_TYPE;


#include <fstream>
#include <vector>
#include <string>
class mailMessage
{
	int m_contentType; //"text/plain" "text/html"
	std::string m_strSubject;
	std::string m_strBody;
	std::string m_strBodyCharset;//email body的encodingcharacter集，default为utf-8
	std::vector<std::string> m_attachs;//要send的attachment

	std::string m_strName;//send者的name
	std::string m_strFrom;//send者的mailbox
	std::vector<std::pair<std::string,std::string> > m_vecTo; //send,first --recipientemail，second recipient昵称
	std::vector<std::pair<std::string,std::string> > m_vecCc; //抄送,first --recipientemail，second recipient昵称
	std::vector<std::pair<std::string,std::string> > m_vecBc; //暗送,first --recipientemail，second recipient昵称
	
	std::string m_strMailFile;//生成的邮件体file pathname
	long m_lMailFileStartPos;//邮件体file中邮件体正文的起始position
	bool m_bDeleteFile;//objectrelease时whetherdeletem_strMailFilefile
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
	
	//setemail subject，正文
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
	//生成Base64 encoding的邮件体file
	//bDelete -- 指示whenmailMessageobjectrelease时whetherdelete生成的file
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
211 systemstatusorsystem帮助response 
　　　214 帮助info 
　　　220 service就绪 
　　　221 serviceclosetransfer信道 
　　　250 要求的邮件操作complete 
　　　251 user非local，将forward向 
　　　354 start邮件input，以.end 
　　　421 service未就绪，closetransfer信道（when必须close时，此应答可以作为对任何command的response） 
　　　450 要求的邮件操作未complete，mailboxnot可用（例如，mailbox忙） 
　　　451 放弃要求的操作；handle过程中出错 
　　　452 system存储not足，要求的操作未执行 
　　　500 format error，commandnot可识别（此error也packet括command行过长） 
　　　501 parameterformat error 
　　　502 commandnot可implementation 
　　　503 error的command序列 
　　　504 commandparameternot可implementation 
　　　550 要求的邮件操作未complete，mailboxnot可用（例如，mailbox未found，ornot可访问） 
　　　551 user非local，请尝试 
　　　552 过量的存储分配，要求的操作未执行 
　　　553 mailbox名not可用，要求的操作未执行（例如mailboxformat error） 
　　　554 operation failed 
*/

