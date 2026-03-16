/*******************************************************************
   *	httpclnt.cpp
   *    DESCRIPTION:HTTP protocol client implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    LAST MODIFY DATE:2006-02-08
   *
   *	net4cpp 2.1
   *	HTTP/1.1
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/httpclnt.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

//send HTTP request; lstartRange and lendRange specify the requested file range
//strurlformat http[s]://[username:password@]host[:port]/...
//example: http://127.0.0.1
//	   https://aa:bb@17.0.0.1:81/aa.htm
//returns SOCKSERR_OK on success
SOCKSRESULT httpClient::send_httpreq_headX(const char *strurl,long lTimeOut,long lstartRange,long lendRange)
{//send HTTP response header, do not wait for return
	if(strurl==NULL) return SOCKSERR_PARAM;
	while(*strurl==' ') strurl++;//delete leading spaces
	if(strncasecmp(strurl,"http://",7) && strncasecmp(strurl,"https://",8) ) return SOCKSERR_PARAM;

	std::string webhost,struser,strpswd;
	int webport=(strurl[4]==':')?HTTP_SERVER_PORT:HTTPS_SERVER_PORT;
	int iOffset=(strurl[4]==':')?7:8;
	const char *ptrURL=strchr(strurl+iOffset,'/');
	if(ptrURL) *(char *)ptrURL=0;
	//先尝试detach访问accountandpassword
	const char *ptr1,*ptr=strchr(strurl+iOffset,'@');
	if(ptr){//set了访问accountandpassword
		*(char *)ptr=0;
		if( (ptr1=strchr(strurl+iOffset,':')) )
		{
			*(char *)ptr1=0;
			struser.assign(strurl+iOffset);
			strpswd.assign(ptr1+1);
			*(char *)ptr1=':';
		}else struser.assign(strurl+iOffset);
		*(char *)ptr='@';
		iOffset=ptr-strurl+1;
	}//?if(ptr)
	//detach出httpservice的address:portand实际的URL
	if( (ptr=strchr(strurl+iOffset,':')) )
	{ 
		webport=atoi(ptr+1);
		webhost.assign(strurl+iOffset,ptr-strurl-iOffset);
	}else webhost.assign(strurl+iOffset);
	if(ptrURL) *(char *)ptrURL='/';
	
	m_httpreq.set_requestRange(lstartRange,lendRange);
	if(struser!="") m_httpreq.set_Authorization(struser.c_str(),strpswd.c_str());
	std::map<std::string,std::string> &header=m_httpreq.Header();
	header["Host"]=webhost; //setHostinfo
	//要进行socketconnect,connectspecified的web service
	SOCKSRESULT sr=this->Connect(webhost.c_str(),webport,lTimeOut);
	if(sr<=0)
	{
		RW_LOG_DEBUG("[httpclnt] Failed to connect HTTP server(%s:%d),error=%d\r\n",webhost.c_str(),webport,sr);
		return SOCKSERR_CONN;
	}
#ifdef _SURPPORT_OPENSSL_
	else if(strurl[4]!=':')
	{ //访问的yeshttp SSLservice
		if(!this->ifSSL()) this->initSSL(false,NULL); //initializationSSLclient
		if(!this->SSL_Associate()){ this->Close(); return SOCKSERR_SSLASSCIATE; }
	}
#endif
	//sendrequest
	return (ptrURL==NULL)?m_httpreq.send_req(this,"/"):m_httpreq.send_req(this,ptrURL);
}

//returnHTTP response码，if发生errorreturn<0
SOCKSRESULT httpClient::send_httpreq(const char *strurl,long lstartRange,long lendRange)
{
	SOCKSRESULT sr=send_httpreq_head(strurl,lstartRange,lendRange);
	if(sr!=SOCKSERR_OK) return SOCKSERR_HTTP_SENDREQ;
	return m_httprsp.recv_rspH(this);
}

//add other related data to HTTP request
void httpClient::add_reqHeader(const char *szname,const char *szvalue)
{
	if(szname==NULL) return;
	std::map<std::string,std::string> &header=m_httpreq.Header();
	std::string val; if(szvalue) val.assign(szvalue);
	header[szname]=val; return;
}
void httpClient::add_reqCookie(const char *szname,const char *szvalue)
{
	if(szname==NULL) return;
	std::map<std::string,std::string> &cookies=m_httpreq.Cookies();
	std::string val; if(szvalue) val.assign(szvalue);
	cookies[szname]=val; return;
}
void httpClient::add_reqPostdata(const char *szname,const char *szvalue)
{
	if(szname==NULL) return;
	std::map<std::string,std::string> &form=m_httpreq.Form();
	std::string val; if(szvalue) val.assign(szvalue);
	form[szname]=val; return;
}

void httpClient::set_reqPostdata(const char *buf,long buflen)
{
	m_httpreq.SetPostData(buf,buflen);
}

/*
//returnHTTP response码，if发生errorreturn<0
SOCKSRESULT httpClient::send_httpreq(const char *strurl,long lstartRange,long lendRange)
{
	if(strurl==NULL) return SOCKSERR_PARAM;
	while(*strurl==' ') strurl++;//delete leading spaces
	if(strncasecmp(strurl,"http://",7) && strncasecmp(strurl,"https://",8) ) return SOCKSERR_PARAM;

	std::string webhost,struser,strpswd;
	int webport=(strurl[4]==':')?HTTP_SERVER_PORT:HTTPS_SERVER_PORT;
	int iOffset=(strurl[4]==':')?7:8;
	const char *ptrURL=strchr(strurl+iOffset,'/');
	if(ptrURL) *(char *)ptrURL=0;
	//先尝试detach访问accountandpassword
	const char *ptr1,*ptr=strchr(strurl+iOffset,'@');
	if(ptr){//set了访问accountandpassword
		*(char *)ptr=0;
		if( (ptr1=strchr(strurl+iOffset,':')) )
		{
			*(char *)ptr1=0;
			struser.assign(strurl+iOffset);
			strpswd.assign(ptr1+1);
			*(char *)ptr1=':';
		}else struser.assign(strurl+iOffset);
		*(char *)ptr='@';
		iOffset=ptr-strurl+1;
	}//?if(ptr)
	//detach出httpservice的address:portand实际的URL
	if( (ptr=strchr(strurl+iOffset,':')) )
	{ 
		webport=atoi(ptr+1);
		webhost.assign(strurl+iOffset,ptr-strurl-iOffset);
	}else webhost.assign(strurl+iOffset);
	if(ptrURL) *(char *)ptrURL='/';
	
	m_httpreq.set_requestRange(lstartRange,lendRange);
	if(struser!="") m_httpreq.set_Authorization(struser.c_str(),strpswd.c_str());
	std::map<std::string,std::string> &header=m_httpreq.Header();
	header["Host"]=webhost; //setHostinfo
	//要进行socketconnect,connectspecified的web service
	SOCKSRESULT sr=this->Connect(webhost.c_str(),webport);
	if(sr<=0)
	{
		RW_LOG_DEBUG("[httpclnt] Failed to connect HTTP server(%s:%d),error=%d\r\n",webhost.c_str(),webport,sr);
		return SOCKSERR_CONN;
	}
#ifdef _SURPPORT_OPENSSL_
	else if(strurl[4]!=':')
	{ //访问的yeshttp SSLservice
		if(!this->ifSSL()) this->initSSL(false,NULL); //initializationSSLclient
		if(!this->SSL_Associate()){ this->Close(); return SOCKSERR_SSLASSCIATE; }
	}
#endif
	//sendrequest
	sr=(ptrURL==NULL)?m_httpreq.send_req(this,"/"):m_httpreq.send_req(this,ptrURL);
	if(sr!=SOCKSERR_OK) return SOCKSERR_HTTP_SENDREQ;
	return m_httprsp.recv_rspH(this);
}
*/

