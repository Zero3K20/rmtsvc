/*******************************************************************
   *	httpreq.h
   *    DESCRIPTION:HTTP request parsing object
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-02-08
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#ifndef __YY_HTTP_REQUEST_H__
#define __YY_HTTP_REQUEST_H__

#include "httpdef.h"
#include "Buffer.h"
#include "socketTcp.h"

#include <map>

namespace net4cpp21
{
	class httpRequest 
	{
	public:
		httpRequest();
		~httpRequest(){}
		HTTPREQ_TYPE get_reqType() const { return m_httpreq_iType; }
		DWORD get_httpVer() const { return m_httpreq_dwVer; }
		std::string& url() { return m_httpreq_strUrl; }
		bool ifReceivedAll() const { return m_httpreq_bReceiveALL; }
		bool bKeepAlive();
		time_t IfModifiedSince();
		void set_requestRange(long lstartpos,long lendpos);
		//there may be multiple ranges, returns the count
		int get_requestRange(long *lpstartpos,long *lpendpos,int idx=0);
		void set_Authorization(const char *user,const char *pswd);
		HTTPAUTH_TYPE get_Authorization(std::string &user,std::string &pswd);
		//set POST data type
		void set_contentType(HTTPREQ_CONTENT_TYPE itype,const char *lpBoundary);
		HTTPREQ_CONTENT_TYPE get_contentType(std::string *strBoundary);
		const char * get_contentCharset();
		long get_contentLen() const { return m_httpreq_lContentlen; }
		//return submitted data pointer, only meaningful for non-form data
		cBuffer &get_contentData() { return m_httpreq_postdata; }
		void ifParseParams(bool b) { m_bParseParams=b; } //set whether to parse submitted parameters, used for HTTP proxy service

		//get HTTP request data
		const char *Request(const char *reqname){
			if(reqname==NULL) return NULL;

			std::map<std::string,std::string>::iterator it=
				m_httpreq_params_GET.find(reqname);
			if(it!=m_httpreq_params_GET.end())
				return (*it).second.c_str();
			it=m_httpreq_params_POST.find(reqname);
			return (it!=m_httpreq_params_POST.end())?
				((*it).second.c_str()):NULL;
		}
		const char *Cookies(const char *cookiename)
		{
			std::map<std::string,std::string>::iterator it=
				(cookiename)?m_httpreq_COOKIE.find(cookiename):m_httpreq_COOKIE.end();
			return (it!=m_httpreq_COOKIE.end())?((*it).second.c_str()):NULL;
		}
		const char *Header(const char *pheader)
		{//get the specified HTTP request header
			std::map<std::string,std::string>::iterator it=
				(pheader)?m_httpreq_HEADER.find(pheader):m_httpreq_HEADER.end();
			return (it!=m_httpreq_HEADER.end())?((*it).second.c_str()):NULL;
		}

		//---------------------------------------------------------------
		void init_httpreq(bool ifKeepHeader=false);//initialize HTTP request to prepare for sending a new HTTP request
		std::map<std::string,std::string> &QueryString() { return m_httpreq_params_GET;}
		std::map<std::string,std::string> &Form() { return m_httpreq_params_POST;}
		std::map<std::string,std::string> &Cookies() { return m_httpreq_COOKIE; }
		std::map<std::string,std::string> &Header() { return m_httpreq_HEADER; }
	
		//encode HTTP request and send; returns SOCKSERR_OK on success
		SOCKSRESULT send_req(socketTCP *psock,const char *lpszurl);
		void SetPostData(const char *buf,long buflen) //set the POST data to send
		{
			m_httpreq_params_POST.clear(); //at this point POST Param is invalid
			m_httpreq_postdata.len()=0; //clear existing data
			m_httpreq_postdata.Resize(buflen+1);
			if(m_httpreq_postdata.str()==NULL) return; 
			::memcpy(m_httpreq_postdata.str(),buf,buflen);
			m_httpreq_postdata.len()=buflen;
		}
		const char * encodeReqestH(unsigned long &lret)
		{
			SOCKSRESULT sr=send_req(NULL,NULL);
			if(sr!=SOCKSERR_OK) return NULL;
			lret=m_httpreq_postdata.len();
			return m_httpreq_postdata.str();
		}
		//--------------------------------------------------------------

		//receive and decode/handle HTTP request
		SOCKSRESULT recv_reqH(socketTCP *psock,time_t timeout=HTTP_MAX_RESPTIMEOUT);
		bool recv_remainder(socketTCP *psock,long receiveBytes=-1); //receive the remaining unreceived data
		//parse Basic authentication account and password string
		static bool ParseAuthorizationBasic(const char *str,
										  std::string &username,std::string &password);
	private:
		HTTPREQ_TYPE ParseRequest(const char *httpreqH);
		HTTPREQ_TYPE ParseFirstRequestLine(const char *lpszLine);
		void EncodeFirstRequestLine(cBuffer &buf);
		void parseURL(const char *lpszurl);
		void encodeURL(cBuffer &buf);
		void parseParam(char *strParam,char delm,
							 std::map<std::string,std::string> &maps,const char *ptrCharset);
		void encodeParam(cBuffer &buf,char delm,
							 std::map<std::string,std::string> &maps);

	private:
		DWORD m_httpreq_dwVer; //httpprotocolversion
		HTTPREQ_TYPE m_httpreq_iType; //HTTP requesttype
		long m_httpreq_lContentlen;
		std::string m_httpreq_strUrl;
		
		//saves parameters submitted from the URL
		std::map<std::string,std::string> m_httpreq_params_GET;
		//saves parameters submitted from a POST form
		std::map<std::string,std::string> m_httpreq_params_POST;
		std::map<std::string,std::string> m_httpreq_HEADER;
		std::map<std::string,std::string> m_httpreq_COOKIE;
		bool m_httpreq_bReceiveALL;//whether the complete HTTP request has been received
		cBuffer m_httpreq_postdata; //saves the received partial POST data

		bool m_bParseParams; //whether to parse parameters; default is true
							//mainly used for proxy services; the proxy service does not parse parameters when handling HTTPS proxy to improve speed
	};

}//?namespace net4cpp21

#endif
