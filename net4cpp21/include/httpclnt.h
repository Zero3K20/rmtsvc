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
		//send HTTP request lstartRange,lendRange告诉web service请求文件的范围
		//success返回SOCKSERR_OK  //不等待响应返回
		SOCKSRESULT send_httpreq_headX(const char *strurl,long lTimeOut,long lstartRange,long lendRange);
		SOCKSRESULT send_httpreq_head(const char *strurl,long lstartRange=0,long lendRange=-1)
		{//send HTTP response头，不等待返回
			return send_httpreq_headX(strurl,-1,lstartRange,lendRange);
		}
		//success返回HTTP response码, <0发生error 0:unknown的响应码
		SOCKSRESULT send_httpreq(const char *strurl,long lstartRange=0,long lendRange=-1);

		httpResponse & Response() { return m_httprsp; }
		long rspContentLen() { return m_httprsp.lContentLength(); }
		//保存HTTP response为specified的文件(不包含HTTP response header)
		//返回保存文件的size，==0发生error
		unsigned long save_httpresp(const char *filename)
		{
			return m_httprsp.save_resp(this,filename);
		}
		
	private:
		httpRequest m_httpreq; //HTTP requestparse对象
		httpResponse m_httprsp; //HTTP response handling对象
	};
}//?namespace net4cpp21

#endif

