/*******************************************************************
   *	smtpclnt.cpp
   *    DESCRIPTION:SMTP protocol client implementation
   *				supports direct mail delivery (bypasses mail relay server, delivers directly to destination mailbox)
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	Simple Mail Transfer Protocol (SMTP)
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/smtpclnt.h"
#include "../include/dnsclnt.h"
#include "../include/cCoder.h"
#include "../utils/cTime.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

//MX domain name parse temporary cache, first mailbox domain
typedef struct _MXINFO
{
	std::vector<std::string> mxhosts;
	time_t tStart; //updatetime
	time_t tExpire; //validtime
	void setmx(const char *ptrmx)
	{
		if(ptrmx==NULL) return;
		const char *ptr=strchr(ptrmx,',');
		while(true){
			while(*ptrmx==' ') ptrmx++; //remove leading spaces
			if(ptr) *(char *)ptr=0;
			if(ptrmx[0]!=0) mxhosts.push_back(ptrmx);
			if(ptr==NULL) break;
			*(char *)ptr=','; ptrmx=ptr+1;
			ptr=strchr(ptrmx,',');
		}//?while(true)
	}
}MXINFO;
static std::map<std::string,MXINFO> map_mxhost;
//get MX mailbox name
MXINFO *dnsMX(const char *dnssvr,int dnsport,const char *domainMX)
{
	if(domainMX==NULL || domainMX[0]==0) return NULL;
	std::string mxname,dm; dm.assign(domainMX);
	::strlwr((char *)dm.c_str());//convert to lowercase
	std::map<std::string,MXINFO>::iterator it=map_mxhost.find(dm);
	if(it!=map_mxhost.end())
	{
		MXINFO &mxinfo=(*it).second;
		if(mxinfo.tExpire==-1 || (time(NULL)-mxinfo.tStart)<mxinfo.tExpire)
			return &mxinfo;
	}//?if(it!=map_mxhost.end())
	
	int icount=0;//attempt DNS domain name parse count
	dnsClient dnsc; DNS_RDATA_MX mxRdata;
	while(icount++<2)
	{
		if(dnsc.status()!=SOCKS_CLOSED) dnsc.Close();
		if(dnsc.Open(0)<=0) return NULL;
		RW_LOG_DEBUG("[smtpclnt] Begin to Query MX of %s\r\n",domainMX);
		SOCKSRESULT sr=dnsc.Query_MX(domainMX,dnssvr,dnsport);
		PDNS_HEADER pdnsh=dnsc.resp_dnsh();
		if(sr==DNS_RCODE_ERR_OK) //parse MX mailbox domain name success
		{
			if( pdnsh->answers<=0) continue; //continue trying
			int i;
			for(i=0;i<pdnsh->answers;i++)
			{
				PDNS_RESPONSE pdnsr=dnsc.resp_dnsr(i);
				if(pdnsr->type==T_MX)
				{
					dnsc.parse_rdata_MX(&mxRdata);
					RW_LOG_DEBUG("\t %d [%d] %s\r\n",i+1,mxRdata.priority,mxRdata.mxname.c_str());
					mxname+=mxRdata.mxname+string(",");
				}
			}//for(...
			RW_LOG_DEBUG("[smtpclnt] Success to Query MX,results=%d\r\n%s\r\n",
				pdnsh->answers,mxname.c_str());
			if(mxname!="") break; else continue; //continue trying
		}else if(sr==SOCKSERR_TIMEOUT)  continue; //timeout, try again
		//otherwise error, domain name parsing may not be possible
		RW_LOG_DEBUG("[smtpclnt] Failed to Query MX,error=%d\r\n",sr);
		break;
	}//?while
	if(mxname=="") return NULL;
	MXINFO mxinfo,&rmxinfo=(it!=map_mxhost.end())?(*it).second:mxinfo;
	rmxinfo.mxhosts.clear(); rmxinfo.setmx(mxname.c_str());
	rmxinfo.tStart=time(NULL); rmxinfo.tExpire=3600*12; //valid time, 12 hours
	if(it==map_mxhost.end()) map_mxhost[dm]=rmxinfo;
	return &map_mxhost[dm];
}

smtpClient :: smtpClient(const char *ehlo)
{
	m_lTimeout=SMTP_MAX_RESPTIMEOUT;
	if(ehlo) m_ehloName.assign(ehlo);
	//read static mxinfo
	FILE *fp=::fopen("mx_hosts","rb");
	if(fp)
	{
		char *pline,sline[256];
		while( ::fgets(sline,256,fp) ){
			int len=strlen(sline);
			pline=sline+len-1;//remove trailing null spaces and carriage return/newline
			while(pline>=sline && (*pline=='\r' || *pline=='\n' || *pline==' '))
			{ *(char *)pline=0; pline--; }
			pline=sline; while(*pline==' ') pline++; //remove leading spaces
			if(pline[0]==0 || pline[0]=='!') continue; //empty line or comment line
			
			const char *ptrmx=strchr(pline,' '); //split domain name and MX info
			if(ptrmx==NULL) if( (ptrmx=strchr(pline,'\t'))==NULL) continue;
			*(char *)ptrmx='\0'; if(pline[0]==0) continue; //invalid mail domain name
			while(*ptrmx==' ' || *ptrmx=='\t') ptrmx++; //skip delimiter
			MXINFO mxinfo; mxinfo.tStart=mxinfo.tExpire=-1;
			mxinfo.setmx(ptrmx); 
			std::string dm; dm.assign(::strlwr(pline));//convert to lowercase
			if(mxinfo.mxhosts.size()>0) map_mxhost[dm]=mxinfo;
		}//?while( ::fgets(sline,256,fp)
		::fclose(fp);
	}//?if(fp)
}
smtpClient :: ~smtpClient(){}

//set SMTP service authentication type and account
void smtpClient :: setSMTPAuth(SMTPAUTH_TYPE authType,
							   const char *strAccount,const char *strPwd)
{
	m_authType=authType;//if SMTPAUTH_NONE is set, account is irrelevant
	if(strAccount) m_strAccount.assign(strAccount);
	if(strPwd) m_strPwd.assign(strPwd);
	return;
}

//****************************************
// function: direct mail delivery. Returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT smtpClient :: sendMail_MX(mailMessage &mms,const char *dnssvr,int dnsport)
{
	std::vector<std::pair<std::string,std::string> > *pvec=NULL;
	const char *emailDomain_ptr=NULL,*emailDomain_pre=NULL;
	std::string strDomainMX;//MX mailbox domain name
	m_authType=SMTPAUTH_NONE;//direct mail delivery; connecting to MX server requires no authentication
	int i,okCount=0,errCount=0; m_errors.clear();
	SOCKSRESULT sr=SOCKSERR_SMTP_RECIPIENT;
	while(true)
	{
		if(pvec==NULL) //loop to send to each destination mailbox
			pvec=&mms.vecTo(mailMessage::TO);
		else if(pvec==&mms.vecTo(mailMessage::TO) )
			pvec=&mms.vecTo(mailMessage::CC);
		else if(pvec==&mms.vecTo(mailMessage::CC))
			pvec=&mms.vecTo(mailMessage::BCC);
		else break;
		std::vector<std::pair<std::string,std::string> >::iterator it=pvec->begin();
		for(;it!=pvec->end();it++) 
		{//get the domain name of the destination mailbox
			errCount++; MXINFO *pmxinfo=NULL;
			emailDomain_ptr=strrchr((*it).first.c_str(),'@');
			if(emailDomain_ptr) //perform DNS parse to get MX mailbox server domain name
				pmxinfo=dnsMX(dnssvr,dnsport,emailDomain_ptr+1);
			if(pmxinfo==NULL)
			{
				string s=(*it).first; s.append(" failed to parse Domain name.\r\n");
				RW_LOG_PRINT(LOGLEVEL_DEBUG,"[smtpclnt] %s",s.c_str());
				m_errors.push_back(s); continue;
			}//?if(pmxinfo==NULL)
			for(i=0;i<(int)pmxinfo->mxhosts.size();i++)
			{
				sr=ConnectSvr(pmxinfo->mxhosts[i].c_str(),SMTP_SERVER_PORT);
				if(sr!=SOCKSERR_OK){ Close(); continue; }
				RW_LOG_DEBUG("[smtpclnt] begin to send email to %s into %s\r\n",
					(*it).first.c_str(),pmxinfo->mxhosts[i].c_str());
				sr=_sendMail(mms,(*it).first.c_str());
				RW_LOG_DEBUG("[smtpclnt] end to send email to %s into %s\r\n",
					(*it).first.c_str(),pmxinfo->mxhosts[i].c_str());
				this->Close(); //disconnected
				if( sr==SOCKSERR_OK) break; //successsend email
				//if one MX mailbox response errors, stop looping and exit with failure
				if(sr==SOCKSERR_SMTP_RESP){ i=pmxinfo->mxhosts.size(); break; }
			}//?for(int i=0;i<pmxinfo->
			if(i<(int)pmxinfo->mxhosts.size()){ 
				okCount++; errCount--;
				RW_LOG_DEBUG("[smtpclnt] Successs to send email to %s\r\n",(*it).first.c_str());
			}else RW_LOG_DEBUG("[smtpclnt] Failed to send email to %s\r\n",(*it).first.c_str());
		}//?for(;it!=pvec->end();it++) 
	}//?while(true)
//	if(errCount>0) sr=SOCKSERR_SMTP_FAILED; //emails not all sent successfully
	return (okCount>0)?SOCKSERR_OK:SOCKSERR_SMTP_FAILED;
}

//****************************************
// function: direct mail delivery. Returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT smtpClient :: sendMail_MX(const char *emlfile,const char *dnssvr,int dnsport)
{
	mailMessage mms;
	if(mms.initFromemlfile(emlfile)!=SOCKSERR_OK)
		return SOCKSERR_SMTP_EMLFILE;
	return sendMail_MX(mms,dnssvr,dnsport);
}
//****************************************
// function: connect to SMTP server and send the specified email. Returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT smtpClient :: sendMail(const char *emlfile,const char *smtpsvr,int smtpport,const char *from)
{
	mailMessage mms; if(from) mms.setFrom(from,NULL);
	if(mms.initFromemlfile(emlfile)!=SOCKSERR_OK)
		return SOCKSERR_SMTP_EMLFILE;
	return sendMail(mms,smtpsvr,smtpport);
}
SOCKSRESULT smtpClient :: sendMail(mailMessage &mms,const char *smtpsvr,int smtpport)
{
	//connect to the specified mail server
	SOCKSRESULT sr=ConnectSvr(smtpsvr,smtpport);
	if(sr!=SOCKSERR_OK) return sr;
	sr=_sendMail(mms,NULL);
	this->Close(); return sr;
}

//****************************************
// function: send the specified email. Returns SOCKSERR_OK on success
//toemail -- if toemail is not null, send to the mailbox specified by toemail; otherwise send to the email recipient specified by mms
//****************************************
SOCKSRESULT smtpClient :: _sendMail(mailMessage &mms,const char *toemail)
{
//	if(this->status()!=SOCKS_CONNECTED) return SOCKSERR_CLOSED;
	char buf[SMTP_MAX_PACKAGE_SIZE];
	//send"MAIL FROM:"
	int buflen=sprintf(buf,"MAIL FROM: <%s>\r\n",mms.from());
	if(!sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)){
		m_errors.push_back(string(buf)); //geterrorresponse
		return SOCKSERR_SMTP_RESP;
	}

	int rcpt_count=0;//send"RCPT TO:"
	if(toemail)
	{
		buflen=sprintf(buf,"RCPT TO: <%s>\r\n",toemail);
		if(sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)) rcpt_count++;
	}else{
		std::vector<std::pair<std::string,std::string> > *pvec=NULL;
		while(true)
		{
			if(pvec==NULL) 
				pvec=&mms.vecTo(mailMessage::TO);
			else if(pvec==&mms.vecTo(mailMessage::TO) )
				pvec=&mms.vecTo(mailMessage::CC);
			else if(pvec==&mms.vecTo(mailMessage::CC))
				pvec=&mms.vecTo(mailMessage::BCC);
			else break;
			std::vector<std::pair<std::string,std::string> >::iterator it=pvec->begin();
			for(;it!=pvec->end();it++)
			{
				buflen=sprintf(buf,"RCPT TO: <%s>\r\n",(*it).first.c_str());
				if(sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)) rcpt_count++;
			}
		}//?while(true)
	}//?if(toemail)...else
	if(rcpt_count<=0) return SOCKSERR_SMTP_RECIPIENT;
	
	//generate email data, prepare to send
	RW_LOG_DEBUG(0,"Be encoding mail data,please waiting...\r\n");
	//generate a temporary file for saving the encoded email body
	const char *maildatafile_ptr=mms.createMailFile(NULL,true);
	RW_LOG_DEBUG("Ready for sending mail data, %s.\r\n",maildatafile_ptr);
	if(maildatafile_ptr==NULL) return SOCKSERR_SMTP_FAILED; 

	//send"DATA\r\n"
	buflen=sprintf(buf,"DATA\r\n");
	if(!sendCommand(354,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
	{
		m_errors.push_back(string(buf)); //geterrorresponse
		return SOCKSERR_SMTP_RESP;
	}else{
		FILE *fp=::fopen(maildatafile_ptr,"rb");
		long pos=mms.MailFileStartPos();
		if(pos>0) ::fseek(fp,pos,SEEK_SET);
		char readbuf[2048];
		while(true)
		{
			buflen=::fread(readbuf,sizeof(char),2048,fp);
			if(buflen<=0) break;
			if( this->Send(buflen,readbuf,-1)< 0) break;
			if(m_parent && m_parent->status()<=SOCKS_CLOSED) break;
		}//?while
		RW_LOG_DEBUG("Success to send mail data, size=%d.\r\n",::ftell(fp));
		::fclose(fp);
	}
	//flag indicating email data transfer complete
	strcpy(buf,"\r\n.\r\n"); buflen=5;
	if(!sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)){
		m_errors.push_back(string(buf)); //geterrorresponse
		return SOCKSERR_SMTP_RESP;
	}
	buflen=sprintf(buf,"QUIT\r\n");
	sendCommand(221,buf,buflen,SMTP_MAX_PACKAGE_SIZE);
	return SOCKSERR_OK;
}



//****************************************
// function: connect to smtp server. Returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT smtpClient :: ConnectSvr(const char *smtpsvr,int smtpport)
{
	SOCKSRESULT sr=this->Connect(smtpsvr,smtpport);
	if(sr<0){
		RW_LOG_DEBUG("[smtpclnt] Failed to connect SMTP (%s:%d),err=%d\r\n",smtpsvr,smtpport,sr);
		return SOCKSERR_SMTP_CONN;
	}
	
	//waiting to receive server response
	char buf[SMTP_MAX_PACKAGE_SIZE];
	if(!sendCommand(220,buf,0,SMTP_MAX_PACKAGE_SIZE)) return SOCKSERR_SMTP_CONN;
	//sendEHLOcommand
	int buflen=sprintf(buf,"EHLO %s\r\n",(m_ehloName!="")?m_ehloName.c_str():smtpsvr);
	if(!sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_CONN;

	if(m_authType==SMTPAUTH_NONE) return SOCKSERR_OK;
	//SMTP server requires LOGIN authentication
	if(m_authType!=SMTPAUTH_LOGIN) return SOCKSERR_SMTP_SUPPORT;
	return (Auth_LOGIN()==SOCKSERR_OK)?SOCKSERR_OK:SOCKSERR_SMTP_AUTH;
}

//****************************************
// function: server authentication. Returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT smtpClient::Auth_LOGIN()
{	
	char buf[SMTP_MAX_PACKAGE_SIZE]; 
	int buflen=sprintf(buf,"AUTH LOGIN\r\n");
	if(!sendCommand(334,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_RESP;
	//send Base64-encoded user account:
	buflen=cCoder::base64_encode((char *)m_strAccount.c_str(),
			m_strAccount.length(),buf);
	buf[buflen++]='\r'; buf[buflen++]='\n'; buf[buflen]=0;
	if(!sendCommand(334,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_RESP;
	//send Base64-encoded user password:
	buflen=cCoder::base64_encode((char *)m_strPwd.c_str(),
			m_strPwd.length(),buf);
	buf[buflen++]='\r'; buf[buflen++]='\n'; buf[buflen]=0;
	if(!sendCommand(235,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_AUTH; //authenticationfailure
	
	return SOCKSERR_OK;
}

//send command and get server response
//[in] response_expected --- expected service response code
//[in] buf: send buffer, also used as receive response data buffer
//[in] buflen: size of data to send; maxbuflen specifies the size of the buf buffer
bool smtpClient :: sendCommand(int response_expected,char *buf,int buflen
									  ,int maxbuflen)
{
	if(buflen>0)
	{
		RW_LOG_DEBUG("[smtpclnt] c--->s:\r\n\t%s",buf);
		if( this->Send(buflen,buf,-1)<=0 ){ buf[0]=0;  return false; }
	}
	//send success, waiting to receive server response
	SOCKSRESULT sr=this->Receive(buf,maxbuflen-1,m_lTimeout);
	if(sr<=0) {
		RW_LOG_DEBUG(0,"[smtpclnt] failed to receive responsed message.\r\n");
		buf[0]=0; return false; 
	} else buf[sr]=0;
	RW_LOG_DEBUG("[smtpclnt] s--->c:\r\n\t%s",buf);
	int responseCode=atoi(buf);

	//may have multiple response lines; if not the last line, the response header is "DDD- ...\r\n"
	//the last line of a multi-line response is "DDD ...\r\n"
	while(true)
	{
		bool bReceivedAll=(buf[sr-1]=='\n'); //possibly fully received
		if(bReceivedAll)
		{
			buf[sr-1]=0; //get the last line
			const char *ptr=strrchr(buf,'\n');
			buf[sr-1]='\n';
			if(ptr==NULL) ptr=buf; else ptr++;
			if(atoi(ptr)!=responseCode || ptr[3]!='-') break;
		}
		sr=this->Receive(buf,maxbuflen-1,m_lTimeout);
		if(sr<=0) break; else buf[sr]=0;
		RW_LOG_DEBUG("%s",buf);
	}//?while
	return (response_expected==responseCode);
}

//-----------------------------------------------------------------------------
//---------------------------------mailMessage---------------------------------

mailMessage :: ~mailMessage()
{
	if(m_strMailFile!="" && m_bDeleteFile)
		::DeleteFile(m_strMailFile.c_str());
}

//parse email address to get name and email. E.g. myname<my@163.com> or my@163.com
void parseEmail(std::string &strEmail,std::string &strName)
{
	const char *ptrS,*ptrE,*ptremail=strEmail.c_str();
	ptrS=strchr(ptremail,'<');
	if(ptrS==NULL) return;
	ptrE=strchr(ptrS+1,'>');
	if(ptrE==NULL) return;
	if(ptrS==ptremail) strName.assign(ptrE+1);
	else strName.assign(ptremail,ptrS-ptremail);
	strEmail.assign(ptrS+1,ptrE-ptrS-1);
}
//****************************************
//initialize mailMessage object from email file
//returns SOCKSERR_OK on success
//emlfile: email format file, two types of format files
//if first line is 'Email body is base64 encoded', it represents an email received by smtpsvr to be forwarded
//otherwise it is an email composed by user to send, format:
//FROM: <sender>\r\n
//TO: <recipient>,<recipient>,...\r\n
//Attachs: <attachment>,<attachment>,...\r\n
//Subject: <subject>\r\n
//Bodytype: <TEXT|HTML>\r\n
//\r\n
//...
//****************************************
#define READ_BUFFER_SIZE 2048
SOCKSRESULT mailMessage :: initFromemlfile(const char *emlfile)
{
	FILE *fp=(emlfile==NULL)?NULL:(::fopen(emlfile,"rb"));
	if(fp==NULL) return SOCKSERR_SMTP_EMLFILE;
	char *pline,sline[READ_BUFFER_SIZE];
	mailMessage::EMAILBODY_TYPE bt=mailMessage::TEXT_BODY;
	int len,emltype=-1;//email body encoding: 0=unencoded, 1=Base64 encoded
	while( ::fgets(sline,READ_BUFFER_SIZE,fp) )
	{
		len=strlen(sline); pline=sline+len-1;
		while(pline>=sline && (*pline=='\r' || *pline=='\n' || *pline==' '))
		{ *(char *)pline=0; pline--; }//remove trailing null spaces and carriage return/newline
		pline=sline; while(*pline==' ') pline++; //remove leading spaces
		if(emltype==-1){//based on the first line read, check whether the email is base64 encoded
			emltype=(strcasecmp(pline,"Email body is base64 encoded")==0)?1:0;
			if(emltype==1) continue; //base64 encoded, read next line of data
		}//?if(emltype==-1)
		if(pline[0]==0) break; //empty line, indicates what follows is the email body; stop parsing
		if(pline[0]=='!') continue; //comment line, skip
		if(strncasecmp(pline,"FROM: ",6)==0)
		{//sender
			pline+=6; while(*pline==' ') pline++;//delete leading spaces
			m_strFrom.assign(pline); m_strName="";
			parseEmail(m_strFrom,m_strName); //split into name and email address
		}else if(strncasecmp(pline,"TO: ",4)==0)
		{//recipient; multiple recipients separated by commas
			pline+=4; while(*pline==' ') pline++;//delete leading spaces
			while(true){
				char *ptr=strchr(pline,',');
				if(ptr) *ptr='\0';
				std::string strName,strEmail(pline);
				parseEmail(strEmail,strName); //split into name and email address
				this->AddRecipient(strEmail.c_str(),m_strName.c_str(),mailMessage::TO);
				if(ptr==NULL) break; else *ptr=',';
				pline=ptr+1;while(*pline==' ') pline++;
			}//?while(true)
		}else if(emltype==1) continue; //Base64 encoded email, no longer parse the remaining parts
		////non-Base64-encoded eml email, parse remaining parts
		else if(strncasecmp(pline,"Attachs: ",9)==0) 	
		{//email attachment; multiple attachments are separated by commas
			pline+=9; while(*pline==' ') pline++;//delete leading spaces
			while(true){
				char *ptr=strchr(pline,',');
				if(ptr) *ptr='\0'; //format: <contentID>aaa.jpg or aaa.jpg
				std::string strContentID,strFilename(pline); 
				parseEmail(strFilename,strContentID); //split; the part inside <> is the contentID
				if(strContentID!="")//parseEmail parses in reverse order; this function is just reused
					 this->AddAtach(strContentID.c_str(),emlfile,strFilename.c_str());
				else this->AddAtach(strFilename.c_str(),emlfile,NULL);
				if(ptr==NULL) break; else *ptr=',';
				pline=ptr+1;while(*pline==' ') pline++;
			}//?while(true)
		}else if(strncasecmp(pline,"Bodytype: ",10)==0)	
		{//email bodytype
			pline+=10; while(*pline==' ') pline++;//delete leading spaces
			bt=(strcasecmp(pline,"HTML")==0)?mailMessage::HTML_BODY:mailMessage::TEXT_BODY;
		}else if(strncasecmp(pline,"Charsets: ",10)==0)	
		{//character set encoding used by the email body type
			pline+=10; while(*pline==' ') pline++;//delete leading spaces
			if(pline[0]!=0) this->m_strBodyCharset.assign(pline);
		}else if(strncasecmp(pline,"Subject: ",9)==0)	
		{//email subject
			pline+=9; while(*pline==' ') pline++;//delete leading spaces
			m_strSubject.assign(pline);
		}	
	}//?while
	this->m_contentType=bt; //setemail bodytype
	if( emltype==0 ){ //non-Base64-encoded email
		m_strMailFile="";
		while(true){
			size_t l=::fread((void *)sline,sizeof(char),READ_BUFFER_SIZE-1,fp);
			sline[l]=0; this->body().append(sline);
			if(l<(READ_BUFFER_SIZE-1)) break; //reading complete
		} 
	}else this->setBody(emlfile,::ftell(fp));
	::fclose(fp); return SOCKSERR_OK;
}

//add attachment; filepath specifies the relative path of the attachment
//contentID - specifies contentID; if specified, it is an image file embedded in the HTML body //yyc add 2006-07-20
#include <io.h>
bool mailMessage::AddAtach(const char *filename,const char *filepath,const char *contentID)
{
	if(filename==NULL || filename[0]==0) return false;
	std::string strfile;
	if(filepath!=NULL && filename[1]!=':') //if the filename does not point to an absolute path
	{
		const char *ptr=strrchr(filepath,'\\');
		if(ptr) strfile.assign(filepath,ptr-filepath+1);
		strfile.append(filename);
	}
	else strfile.assign(filename);
	if(_access(strfile.c_str(),0)==-1) return false;
	if(contentID && contentID[0]!=0)
	{
		char tmpbuf[128]; sprintf(tmpbuf,"<%s>",contentID);
		strfile.insert(0,tmpbuf);
	}
	m_attachs.push_back(strfile);
	return true;
}

//addrecipient
bool mailMessage::AddRecipient(const char *email,const char *nick,RECIPIENT_TYPE rt)
{
	if(email==NULL || email[0]==0) return false;
	if(nick==NULL || nick[0]==0) nick=email;
	std::pair<std::string,std::string> p(email,nick);
	std::vector<std::pair<std::string,std::string> > *pvec=&m_vecTo;
	if(rt==CC) pvec=&m_vecCc; else if(rt==BCC) pvec=&m_vecBc;
	pvec->push_back(p);
	return true;
}

const char MAILBOUNDARY_STRING[]="#yycnet.yeah.net#";
//base64-encode the specified file and write to the specified stream
//contentID - specifies contentID; if specified, it is an image file embedded in the HTML body //yyc add 2006-07-20
bool base64File(const char *filename,ofstream &out,const char *contentID)
{
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) return false;
	long len=strlen(filename);
	const char *fname=filename;//get filename
	for(int i=len-1;i>=0;i--)
	{
		if(*(filename+i)=='\\' || *(filename+i)=='/'){ fname=filename+i+1; break; }
	}//?for
	
	char srcbuf[1024],dstbuf[1500];
	static char *ct[]={"application/octet-stream","image/jpeg","image/gif","image/png","image/bmp","image/tif"};
	int idx_ct=0; const char *ptr=strrchr(fname,'.');
	if(ptr && (strcasecmp(ptr,".jpg")==0 || strcasecmp(ptr,".jpeg")==0) )
		idx_ct=1;
	else if(ptr && strcasecmp(ptr,".gif")==0)
		idx_ct=2;
	else if(ptr && strcasecmp(ptr,".png")==0)
		idx_ct=3;
	else if(ptr && strcasecmp(ptr,".bmp")==0)
		idx_ct=4;
	else if(ptr && strcasecmp(ptr,".tif")==0)
		idx_ct=5;
	
	if(contentID==NULL || contentID[0]==0)
	{
		len=sprintf(srcbuf,"--%s_000\r\n"
						   "Content-Type:%s;\r\n"
						   "\tName=\"%s\"\r\n"
					       "Content-Disposition:attachment;\r\n"
						   "\tFileName=\"%s\"\r\n"
					       "Content-Transfer-Encoding: base64\r\n\r\n",
							MAILBOUNDARY_STRING,ct[idx_ct],fname,fname);
	}else{
		const char *p1,*ptr_ct=ct[idx_ct];
		if(contentID[0]=='(' && (p1=strchr(contentID+1,')')) ){
			*(char *)p1='\0'; 
			ptr_ct=contentID+1; contentID=p1+1;
		}
		len=sprintf(srcbuf,"--%s_000\r\n"
						   "Content-Type: %s;\r\n"
						   "\tname=\"%s\"\r\n"
					       "Content-Transfer-Encoding: base64\r\n"
						   "Content-ID: <%s>\r\n\r\n",
							MAILBOUNDARY_STRING,ptr_ct,fname,contentID);
	}
	
	out.write(srcbuf,len);
	//start base64 encoding the specified file
	while(true)
	{ //read bytes in multiples of 3 each time for base64 encoding
		len=::fread(srcbuf,sizeof(char),1020,fp);
		if(len<=0) break;
		len=cCoder::base64_encode(srcbuf,len,dstbuf);
		out.write(dstbuf,len); out.write("\r\n",2);
	}//?while
	out.write("\r\n",2);//add an empty line to indicate end of attachment base64 encoding
	::fclose(fp); return true;
}
//generate the Base64-encoded email body file
//bDelete -- specifies whether to delete the generated file when the mailMessage object is released
//on success return the generated filename
const char * mailMessage::createMailFile(const char *file,bool bDelete)
{
	if(file==NULL || file[0]==0)
	{
		if(m_strMailFile!="" && _access(m_strMailFile.c_str(),0)!=-1)
			return m_strMailFile.c_str();
		//generate a temporary file for saving the encoded email body
		char buf[64]; time_t tNow=time(NULL);
		srand( (unsigned)clock() );
		struct tm * ltime=localtime(&tNow);
		sprintf(buf,"tmp%04d%02d%02d%02d%02d%02d_%d.eml",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
			ltime->tm_hour, ltime->tm_min, ltime->tm_sec,rand());
		m_strMailFile.assign(buf);
	}
	else m_strMailFile.assign(file);

	ofstream out(m_strMailFile.c_str(),ios::binary);
	if(out.is_open()==0){m_strMailFile=""; return NULL;}
	m_bDeleteFile=bDelete;

	//From:
	out << "From: " << ((m_strName!="")?m_strName:m_strFrom);
	out << " <" << m_strFrom << ">\r\n";
	//To:
	if(m_vecTo[0].second=="")
		out << "To: <" << m_vecTo[0].first << ">\r\n";
	else
		out << "To: \"" << m_vecTo[0].second << "\" <" << m_vecTo[0].first << ">\r\n";
	for(int i=1;i<(int)m_vecTo.size();i++)
	{
		if(m_vecTo[i].second=="")
			out << "\t <" << m_vecTo[i].first << ">\r\n";
		else
			out << "\t \"" << m_vecTo[i].second << "\" <" << m_vecTo[i].first << ">\r\n";
	}
	//CC:
	if(m_vecCc.size()>0)
	{
		if(m_vecCc[0].second=="")
			out << "CC: <" << m_vecCc[0].first << ">\r\n";
		else
			out << "CC: \"" << m_vecCc[0].second << "\" <" << m_vecCc[0].first << ">\r\n";
		for(int i=1;i<(int)m_vecCc.size();i++)
		{
			if(m_vecCc[i].second=="")
				out << "\t <" << m_vecCc[i].first << ">\r\n";
			else
				out << "\t \"" << m_vecCc[i].second << "\" <" << m_vecCc[i].first << ">\r\n";
		}
	}//?m_vecCc.size()
	//Subject:
	out << "Subject: " << m_strSubject << "\r\n";
	//Date:
	char datebuf[64];
	int len=sprintf(datebuf,"Date: "); datebuf[len]=0;
	cTime t=cTime::GetCurrentTime();
	int tmlen=t.Format(datebuf+len,48,"%a, %d %b %Y %H:%M:%S %Z");
	if(tmlen<=0) tmlen=0; len+=tmlen; 
	datebuf[len++]='\r'; datebuf[len++]='\n'; datebuf[len]=0;
	out << datebuf;

	out << "X-Mailer: YMailer v2.1\r\nMIME-Version: 1.0\r\n";
	out << "Content-Type: multipart/related;\r\n\ttype=\"multipart/alternative\";\r\n";
	out << "\tboundary=\"" << MAILBOUNDARY_STRING << "_000\"\r\n" << "\r\n";
	out << "This is a multi-part message in MIME format\r\n" << "\r\n";
	
	out << "--" << MAILBOUNDARY_STRING << "_000\r\n";
	out << "Content-Type: multipart/alternative;\r\n";
	out << "\tboundary=\"" << MAILBOUNDARY_STRING << "_001\"\r\n";
	out << "\r\n\r\n";
	
	char *ptr_encodeBody=NULL;
	if(strcasecmp(m_strBodyCharset.c_str(),"utf-8")==0)
	{
		//perform utf8 and Base64 encoding on the email body
		long encode_bodylen=cCoder::Utf8EncodeSize(m_strBody.length());
		ptr_encodeBody=new char[encode_bodylen+1];
		if(ptr_encodeBody){
			encode_bodylen=cCoder::utf8_encode(m_strBody.c_str(),m_strBody.length(),ptr_encodeBody);
			char *ptr_utf8=ptr_encodeBody;
			//then perform base64 encoding on the utf8
			if( (ptr_encodeBody=new char[cCoder::Base64EncodeSize(encode_bodylen)+1]) )
				cCoder::base64_encode(ptr_utf8,encode_bodylen,ptr_encodeBody);
			delete[] ptr_utf8;
		}//?if(ptr_encodeBody)
	}
	//optional body of the first part of a multi-part email
	//text body-----------------
	out << "--" << MAILBOUNDARY_STRING << "_001\r\n";
	out << "Content-type: text/plain;\r\n";
	if(m_contentType!=HTML_BODY)
	{
		if(ptr_encodeBody)
		{	
			out << "\tCharset=\"utf-8\"\r\nContent-Transfer-Encoding: base64\r\n\r\n";
			out<< ptr_encodeBody;
		}else{
			out << "\tCharset=\"" << m_strBodyCharset << "\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
			out << m_strBody;
		}
	}else{
		out << "\tCharset=\"gb2312\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
		out << "The email is HTML format!\r\nThis is an HTML format email!";
	}
	out <<"\r\n\r\n";
	//HTML body-----------------
	if(m_contentType==HTML_BODY){
		out << "--" << MAILBOUNDARY_STRING << "_001\r\n";
		out << "Content-type: text/html;\r\n";
		if(ptr_encodeBody)
		{	
			out << "\tCharset=\"utf-8\"\r\nContent-Transfer-Encoding: base64\r\n\r\n";
			out<< ptr_encodeBody;
		}else{
			out << "\tCharset=\"" << m_strBodyCharset << "\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
			out << m_strBody;
		}
		out <<"\r\n\r\n";
	}
	if(ptr_encodeBody) delete[] ptr_encodeBody;
	out <<"--" << MAILBOUNDARY_STRING <<"_001--\r\n\r\n";
	//end of optional body handling for the first part of a multi-part email
	
	//start handling email attachments
	int fileAttachs=0;//valid attachment count
	int i;
	for(i=0; i<(int)m_attachs.size();i++)
	{//if the attachment filename starts with <>, the content inside <> represents the contentID
		const char *ptr=m_attachs[i].c_str();
		const char *contentID=NULL;
		if(*ptr=='<'){
			contentID=ptr+1;
			if( (ptr=strchr(contentID,'>')) )
			{ *(char *)ptr='\0'; ptr++; }
			else{ptr=contentID; contentID=NULL; } 
		}
		if(base64File(ptr,out,contentID)) fileAttachs++;
	}//?for
	out <<"--" << MAILBOUNDARY_STRING <<((fileAttachs>0)?"_000--":"");
	return m_strMailFile.c_str();;
}

/*
//get MX mailbox name
//return 0 failure; return 1 got MX mailbox domain name from MX cache; return 2 got MX mailbox domain name via DNS resolution
int dnsMX(const char *dnssvr,int dnsport,const char *domainMX,std::string &mxname)
{
	mxname="";//first try to get MX mailbox name via mx_hosts
	FILE *fp=::fopen("mx_hosts","rb");
	if(fp)
	{
		char *pline,sline[256]; int n=strlen(domainMX);
		while( ::fgets(sline,256,fp) ){
			int len=strlen(sline);
			pline=sline+len-1;//remove trailing null spaces and carriage return/newline
			while(pline>=sline && (*pline=='\r' || *pline=='\n' || *pline==' '))
			{ *(char *)pline=0; pline--; }
			pline=sline; while(*pline==' ') pline++; //remove leading spaces
			if(pline[0]==0 || pline[0]=='!') continue; //empty line or comment line	
			if(strncasecmp(pline,domainMX,n)) continue; //not the domain being searched
			else pline+=n;
			if(*pline!=' ' && *pline!='\t') continue; //invalid line
			while(*pline==' ' || *pline=='\t') pline++; //skip delimiter
			mxname.assign(pline); break;
		}//?while( ::fgets(sline,256,fp)
		::fclose(fp);
	}//?if(fp)
	if(mxname!="") return 1;
	dnsClient dnsc; DNS_RDATA_MX mxRdata; 
	if(dnsc.Open(0)<=0) return 0;
	SOCKSRESULT sr=dnsc.Query_MX(domainMX,dnssvr,dnsport);
	if(sr==DNS_RCODE_ERR_OK) //parse MX mailbox domain name success
	{
		PDNS_HEADER pdnsh=dnsc.resp_dnsh();
		RW_LOG_DEBUG("[smtpclnt] Success to Query MX,results=%d.\r\n",pdnsh->answers);
		for(int i=0;i<pdnsh->answers;i++)
		{
			PDNS_RESPONSE pdnsr=dnsc.resp_dnsr(i);
			if(pdnsr->type==T_MX)
			{
				dnsc.parse_rdata_MX(&mxRdata);
				RW_LOG_DEBUG("\t %d [%d] %s\r\n",i+1,mxRdata.priority,mxRdata.mxname.c_str());
				mxname+=mxRdata.mxname+string(",");
			}
		}//for(...
		return 2;
	}else RW_LOG_DEBUG("[smtpclnt] Failed to Query MX,error=%d\r\n",sr);
	return 0;
}
SOCKSRESULT smtpClient :: sendMail_MX(mailMessage &mms,const char *dnssvr,int dnsport)
{
	std::vector<std::pair<std::string,std::string> > *pvec=NULL;
	const char *emailDomain_ptr=NULL,*emailDomain_pre=NULL;
	std::string strDomainMX;//MX mailbox domain name
	m_authType=SMTPAUTH_NONE;//direct mail delivery; connecting to MX server requires no authentication
	int errCount=0; m_errors.clear();
	SOCKSRESULT sr=SOCKSERR_SMTP_RECIPIENT;
	while(true)
	{
		if(pvec==NULL) //loop to send to each destination mailbox
			pvec=&mms.vecTo(mailMessage::TO);
		else if(pvec==&mms.vecTo(mailMessage::TO) )
			pvec=&mms.vecTo(mailMessage::CC);
		else if(pvec==&mms.vecTo(mailMessage::CC))
			pvec=&mms.vecTo(mailMessage::BCC);
		else break;
		std::vector<std::pair<std::string,std::string> >::iterator it=pvec->begin();
		for(;it!=pvec->end();it++) 
		{//get the domain name of the destination mailbox
			errCount++;
			emailDomain_ptr=strrchr((*it).first.c_str(),'@');
			if(emailDomain_ptr)
			{	
				emailDomain_ptr++; //perform DNS parse to get MX mailbox server domain name
				if(emailDomain_pre==NULL || strDomainMX=="" || 
					strcasecmp(emailDomain_ptr,emailDomain_pre)!=0)
					dnsMX(dnssvr,dnsport,emailDomain_ptr,strDomainMX);
				
				emailDomain_pre=emailDomain_ptr;
				RW_LOG_DEBUG("[smtpclnt] begin to send email to %s into %s.\r\n",
					(*it).first.c_str(),strDomainMX.c_str());

				if(strDomainMX!="")//MX domain name parsecomplete
				{//prepare to send email
					//strDomainMX is a comma-separated multiple domain name string; loop to get each MX domain name
					const char *ptrDomainMX=strDomainMX.c_str();
					while(ptrDomainMX[0]!=0)
					{
						const char *delmPos=strchr(ptrDomainMX,',');
						if(delmPos) *(char *)delmPos='\0';
						
						sr=ConnectSvr(ptrDomainMX,SMTP_SERVER_PORT);
						if( sr==SOCKSERR_OK)
						{
							if( (sr=_sendMail(mms,(*it).first.c_str()))!=SOCKSERR_OK)
							{
								string s=(*it).first; s.append(" failed to send.\r\n");
								m_errors.push_back(s);
								RW_LOG_DEBUG("[smtpclnt] Failed to send, error=%d\r\n",sr);
								if(delmPos) { *(char *)delmPos=','; delmPos=NULL; } //exit loop
							}//?if( (sr=_sendMail(mms,(*it).first.c_str()))!=SOCKSERR_OK)
							else
							{ 
								errCount--;
								if(delmPos) { *(char *)delmPos=','; delmPos=NULL; } //exit loop
							}
						}//?if( sr==SOCKSERR_OK)
						else
						{
							string s=(*it).first; s.append(" failed to connect ");
							s+=string(ptrDomainMX); s.append(".\r\n"); m_errors.push_back(s);
							RW_LOG_DEBUG("[smtpclnt] Failed to connect %s,return %d.\r\n",ptrDomainMX,sr);
						}//?if( sr==SOCKSERR_OK)...else
						this->Close(); //disconnected

						if(delmPos){
							*(char *)delmPos=',';
							ptrDomainMX=delmPos+1;
						}else break;
					}//?while(...)
				}//?if(strDomainMX!="")
				else
				{
					sr=SOCKSERR_SMTP_DNSMX;
					string s=(*it).first; s.append(" failed to parse Domain name.\r\n");
					m_errors.push_back(s);
				}
				RW_LOG_DEBUG("[smtpclnt] end to send email to %s into %s.\r\n",
					(*it).first.c_str(),strDomainMX.c_str());
			}//?if(emailDomain_ptr)
			else
			{
				sr=SOCKSERR_SMTP_EMAIL;
				string s=(*it).first; s.append(" is not a valid email.\r\n");
				m_errors.push_back(s);
				RW_LOG_DEBUG(0,"[smtpclnt] "); RW_LOG_DEBUG(s.length(),s.c_str());
			}
		}//?for(;it!=pvec->end();it++)
	}//?while
	if(errCount>0) sr=SOCKSERR_SMTP_FAILED; //emails not all sent successfully
	return sr;
}
*/