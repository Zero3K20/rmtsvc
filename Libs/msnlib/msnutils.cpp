/*******************************************************************
   *	msnlib.cpp
   *    DESCRIPTION:MSN protocol handler class implementation
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2005-06-28
   *	
   *******************************************************************/

#include "../../include/sysconfig.h"
#include "../../include/httpclnt.h"
#include "../../include/cCoder.h"
#include "../../include/cLogger.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;

//HTTPS authentication, get passport
SOCKSRESULT msnMessager :: passport_auth(std::string &strKey,const char *strAccount,const char *pwd)
{
	httpClient httpsock; 
	std::string httpsURL; int iret;
	//by default, first get the real HTTPS URL from nexus.passport.com
	//this URL should be https://loginnet.passport.com/login2.srf
	//generally this doesn't change, but it's better to fetch from Microsoft's site to be safe
/*	iret=httpsock.send_httpreq("https://nexus.passport.com:443/rdr/pprdr.asp");
	if(iret==200){//return success
		httpResponse & resp=httpsock.Response();
		const char *ptr=resp.Header("PassportURLs");
		if(ptr && (ptr=strstr(ptr,"DALogin=")) )
		{
			ptr+=strlen("DALogin=");
			const char *ptr1=strchr(ptr,',');
			if(ptr1){
				httpsURL.assign("https://");
				httpsURL.append(ptr,ptr1-ptr);
				RW_LOG_DEBUG("https URL:%s\r\n",httpsURL.c_str());
			}//?if(ptr1)
		}
	}//?if(iret==200)
	else
		RW_LOG_DEBUG("Failed to access https://nexus.passport.com:443/rdr/pprdr.asp,err=%d\r\n",iret);
*/ //yyc remove 2006-02-28: often gets stuck during SSL negotiation after connecting to nexus.passport.com, therefore skip this step

	//ifgetfailure，尝试使用default的https URL
	if(httpsURL=="") httpsURL.assign("https://loginnet.passport.com/login2.srf");
	char buf[512]; std::string strMimeAccount;
	iret=cCoder::mime_encode(strAccount,strlen(strAccount),buf); buf[iret]=0;
	strMimeAccount.assign(buf);
TRANS302:
	iret=sprintf(buf,"Passport1.4 OrgVerb=GET,OrgURL=http%%3A%%2F%%2Fmessenger%%2Emsn%%2Ecom,sign-in=%s,pwd=%s,%s",
		strMimeAccount.c_str(),pwd,strKey.c_str());
	buf[iret]=0; httpsock.cls_httpreq();
	httpsock.add_reqHeader("Authorization",buf);
	iret=httpsock.send_httpreq(httpsURL.c_str());
	if(iret==302) //转向
	{
		httpResponse & resp=httpsock.Response();
		const char *ptr=resp.Header("Location");
		RW_LOG_PRINT(LOGLEVEL_DEBUG,"Transfer to %s.\r\n",ptr);
		if(ptr==NULL) return SOCKSERR_MSN_AUTH;
		httpsURL.assign(ptr); goto TRANS302;
	}
	else if(iret==200) //responsesuccess
	{
		httpResponse & resp=httpsock.Response();
		const char *ptr=resp.Header("Authentication-Info");
		if(ptr==NULL || (ptr=strstr(ptr,"from-PP=\'"))==NULL) 
			return SOCKSERR_MSN_AUTH;
		ptr+=strlen("from-PP=\'");
		for(int i=0;true;i++)
			if(ptr[i]==0 || ptr[i]==39) break; //chr(')=39
		
		strKey.assign(ptr,i); return MSN_ERR_OK;
	}//?else if(iret==200)
	else if(iret==401) //HTTP/1.1 401 Unauthorized
		return SOCKSERR_MSN_EMAIL; //invalidaccountorpassword
	return SOCKSERR_MSN_AUTH;
}

bool msnMessager :: MSNP11Challenge(std::string &strChallenge,const char *szClientID,const char *szClientCode) 
{   
	MD5_CTX context;
	unsigned char pMD5Hash[16],pMD5Hash_1[16]; 
	MD5_Init(&context);
	MD5_Update(&context, (const void *)strChallenge.c_str(), strChallenge.length());
	MD5_Update(&context, (const void *)szClientCode, strlen(szClientCode));
	MD5_Final(pMD5Hash, &context);
//	printf("MD5Hash=0x");for(int mm=0;mm<16;mm++) printf("%02x",pMD5Hash[mm]); printf("\r\n");
	memcpy(pMD5Hash_1,pMD5Hash,16);
	int i,*pMD5Parts=(int *)pMD5Hash_1; 
	for (i=0; i<4; i++) pMD5Parts[i]&=0x7FFFFFFF;
	
	int nchlLen=strChallenge.length()+strlen(szClientID);
	if (nchlLen%8!=0) nchlLen+=8-(nchlLen%8);
	char chlString[256]; chlString[nchlLen]=0;
	memset(chlString,'0',nchlLen);
	memcpy(chlString,strChallenge.c_str(),strChallenge.length());
	memcpy(chlString+strChallenge.length(),szClientID,strlen(szClientID));
	int *pchlStringParts=(int *)chlString;

	LONG64 nHigh=0;
	LONG64 nLow=0;

	for (i=0; i<(nchlLen/4)-1; i+=2) {
		LONG64 temp=pchlStringParts[i];
		temp=(pMD5Parts[0] * (((0x0E79A9C1 * (LONG64)pchlStringParts[i]) % 0x7FFFFFFF)+nHigh) + pMD5Parts[1])%0x7FFFFFFF;
		nHigh=(pMD5Parts[2] * (((LONG64)pchlStringParts[i+1]+temp) % 0x7FFFFFFF) + pMD5Parts[3]) % 0x7FFFFFFF;
		nLow=nLow + nHigh + temp;
	}
	nHigh=(nHigh+pMD5Parts[1]) % 0x7FFFFFFF;
	nLow=(nLow+pMD5Parts[3]) % 0x7FFFFFFF;

	unsigned long *pNewHash=(unsigned long *)pMD5Hash;
	
	pNewHash[0]^=nHigh;
	pNewHash[1]^=nLow;
	pNewHash[2]^=nHigh;
	pNewHash[3]^=nLow;

	char szHexChars[]="0123456789abcdef";
	for (i=0; i<16; i++) {
		chlString[i*2]=szHexChars[(pMD5Hash[i]>>4)&0xF];
		chlString[(i*2)+1]=szHexChars[pMD5Hash[i]&0xF];
	}
	chlString[32]=0; strChallenge.assign(chlString);
//	printf("result1=%s.\r\n",strChallenge.c_str());
	return true;
}

//对file进行sha-1算法的运算，获得一个20byte流
//再用base64进行加码获得的character串
bool msnMessager :: SHA1File(FILE *fp,string &strRet)
{
	if(fp==NULL) return false;
	SHA_CTX ctx; unsigned char digest[20];
	char readBuf[2048]; long readLen;

	SHA1_Init(&ctx);
	while( (readLen=fread(readBuf,sizeof(char),2048,fp))>0 )
	{
		SHA1_Update(&ctx, (const void *)readBuf, readLen);
		if(readLen<2048) break;
	}//?while
	SHA1_Final(digest, &ctx);		
	//进行base64 encoding
	readLen=cCoder::base64_encode((char *)digest,20,readBuf);
	readBuf[readLen]=0; strRet.assign(readBuf);
	return true;
}

bool msnMessager :: SHA1Buf(const char *buf,long len,string &strRet)
{
	if(buf==NULL || len<=0) return false;
	SHA_CTX ctx; unsigned char digest[20];
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, (const void *)buf, len);
	SHA1_Final(digest, &ctx);
	//进行base64 encoding
	char readBuf[256]; readBuf[0]=0;
	long readLen=cCoder::base64_encode((char *)digest,20,readBuf);
	readBuf[readLen]=0; strRet.assign(readBuf);
	return true;
}

bool msnMessager :: MD5Buf(const char *buf,long len,string &strRet)
{
	if(buf==NULL || len<=0) return false;
	MD5_CTX context;
	unsigned char digest[16]; char tmpBuf[32];
	MD5_Init(&context);
	MD5_Update(&context, (const void *)buf, len);
	MD5_Final(digest, &context);
	for (int i = 0; i < 16; i++) sprintf (tmpBuf+i*2,"%02x", digest[i]);
	strRet.assign(tmpBuf,32);
	return true;
}

//------------------------------------------------------------------------------------------
//----------------------------private function----------------------------------------------
//add一个新联系人
cContactor * msnMessager :: _newContact(const char *email,const wchar_t *nickW)
{
	if(email==NULL || email[0]==0) return NULL;
	::_strlwr((char *)email);
	std::map<std::string, cContactor *>::iterator it=m_contacts.find(email);
	cContactor *pcon=NULL;
	if(it!=m_contacts.end() && (pcon=(*it).second) )
	{
		if(nickW){
			pcon->m_nick.assign(nickW);
			std::string tmpS; tmpS.resize( pcon->m_nick.length()*2+1);
			wchar2chars(nickW,(char *)tmpS.c_str(),tmpS.length());
			pcon->m_nick_char.assign(tmpS.c_str());
		}
	}
	else if( (pcon=new cContactor()) ){
		pcon->m_email.assign(email);
		//复制代理info
		pcon->m_chatSock.setProxy(m_curAccount.m_chatSock);
		//set其他chatSock的父为andNSconnect的socket，一旦commandconnect通道disconnect则异常
		pcon->m_chatSock.setParent((socketBase *)&m_curAccount.m_chatSock);
		m_contacts[pcon->m_email]=pcon;

		if(nickW)
		{
			pcon->m_nick.assign(nickW);
			std::string tmpS; tmpS.resize( pcon->m_nick.length()*2+1);
			wchar2chars(nickW,(char *)tmpS.c_str(),tmpS.length());
			pcon->m_nick_char.assign(tmpS.c_str());
		}
		else //default昵称andemail相同
		{
			pcon->m_nick_char=pcon->m_email; wchar_t *nickw=NULL;
			if( (nickw=new wchar_t[pcon->m_email.length()+1]) )
			{
				int len=MultiByteToWideChar(CP_ACP,0,pcon->m_nick_char.c_str(),pcon->m_nick_char.length(),
					nickw,pcon->m_nick_char.length()+1); nickw[len]=0;
				pcon->m_nick.assign(nickw); delete[] nickw;
			}
		}
	}
	return pcon;
}

bool msnMessager :: connectSvr(socketProxy &sock,const char *strhost,int iport)
{
	if(iport<=0 && strhost)
	{
		char *ptr=(char *)strchr(strhost,':');
		if(ptr) {*ptr=0; iport=atoi(ptr+1); }
	}
	SOCKSRESULT sr=sock.Connect(strhost,iport,MSN_MAX_RESPTIMEOUT);
	return (sr>0);
}

int splitstring(const char *str,char delm,std::vector<std::string> &vec,int maxSplit)
{
	if(str==NULL) return 0;
	while(*str==' ') str++;//delete leading spaces
	const char *ptr=strchr(str,delm);
	while(true)
	{
		if(maxSplit>0 && vec.size()>=maxSplit)
		{
			vec.push_back(str); break;
		}
		if(ptr) *(char *)ptr=0;
		vec.push_back(str);
		if(ptr==NULL) break;
		*(char *)ptr=delm; str=ptr+1;
		while(*str==' ') str++;//delete leading spaces
		ptr=strchr(str,delm);
	}//?while
	return vec.size();
}
