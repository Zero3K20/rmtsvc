/*******************************************************************
   *	httpclnt.h
   *    DESCRIPTION:HTTP protocol client declaration
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

#ifndef __YY_HTTP_CLIENT_H__
#define __YY_HTTP_CLIENT_H__

#include "httpdef.h"
#include "httpreq.h"
#include "httprsp.h"
#include "proxyclnt.h"

namespace net4cpp21
{
	class httpClient : public socketProxy
	{
	public:
		httpClient(){}
		virtual ~httpClient(){}
		//add other related data to HTTP request
		void add_reqHeader(const char *szname,const char *szvalue);
		void add_reqCookie(const char *szname,const char *szvalue);
		void add_reqPostdata(const char *szname,const char *szvalue);
		void set_reqPostdata(const char *buf,long buflen);
		//clear HTTP-related data from httpreq object
		void cls_httpreq(bool ifKeepHeader=false) { m_httpreq.init_httpreq(ifKeepHeader); return; }
		//send HTTP request; lstartRange and lendRange specify the requested file range
		//returns SOCKSERR_OK on success // does not wait for response
		SOCKSRESULT send_httpreq_headX(const char *strurl,long lTimeOut,long lstartRange,long lendRange);
		SOCKSRESULT send_httpreq_head(const char *strurl,long lstartRange=0,long lendRange=-1)
		{//send HTTP response header, do not wait for return
			return send_httpreq_headX(strurl,-1,lstartRange,lendRange);
		}
		//returns HTTP response code on success, <0 means error, 0 means unknown response code
		SOCKSRESULT send_httpreq(const char *strurl,long lstartRange=0,long lendRange=-1);

		httpResponse & Response() { return m_httprsp; }
		long rspContentLen() { return m_httprsp.lContentLength(); }
		//save HTTP response to specified file (not including HTTP response header)
		//returns saved file size; ==0 means error occurred
		unsigned long save_httpresp(const char *filename)
		{
			return m_httprsp.save_resp(this,filename);
		}
		
	private:
		httpRequest m_httpreq; //HTTP requestparseobject
		httpResponse m_httprsp; //HTTP response handlingobject
	};
}//?namespace net4cpp21

#endif

