/*******************************************************************
   *	mportsvr.h
   *    DESCRIPTION:local port mapping service
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#ifndef __YY_MAPPORTSVR_H__
#define __YY_MAPPORTSVR_H__

#include "../../net4cpp21/include/socketSvr.h"
#include "../../net4cpp21/include/buffer.h"
#include "mportdef.h"
#include "deelx.h"
typedef CRegexpT<char> regexp;

namespace net4cpp21
{
	typedef struct _RegCond
	{
		std::string strHeader; //HTTP header to modify
		std::string strPattern;//Regexp match pattern
		std::string strReplto; //string to replace
		_RegCond(){}
		~_RegCond(){}
	}RegCond;
	//HTTP request/response header parser object
	//local TCP port mapping service class
	class mportTCP : public socketSvr
	{
	public:
		mportTCP();
		virtual ~mportTCP();
		long &Tag() { return m_lUserTag; }
		SSLTYPE getSSLType() const { return m_ssltype; }
		bool getIfLogdata() const { return m_bLogdatafile; }
		void setIfLogdata(bool b){ m_bLogdatafile=b; }
		SOCKSRESULT Start(const char *strMyCert,const char *strMyKey,const char *strKeypwd,
					   const char *strCaCert,const char *strCaCRL); //start mapping service
		SOCKSRESULT StartX();
		void Stop(); //stopmapservice
		
		//set the application service to be mapped
		void setAppsvr(const char *appsvr,int apport,const char *appdesc,MPORTTYPE apptype=MPORTTYPE_UNKNOW);
		void setMapping(int mportStart,int mportEnd,const char *bindip=NULL);
		void setSSLType(SSLTYPE ssltype,bool bSSLVerify);
		void setMaxRatio(unsigned long maxratio) { m_maxratio=maxratio; }

		void xml_info_mtcp(cBuffer &buffer);
		int  str_info_mtcp(const char *mapname,char *buf);

		bool addRegCond(int rspcode,const char *header,const char *pattern,const char *replto);
		bool addRegCond(const char *url,const char *header,const char *pattern,const char *replto);
		bool str_info_regcond(const char *mapname,std::string &strini);
	protected:
		virtual socketTCP * connectAppsvr(char *strHost,socketTCP *psock);
		std::pair<std::string,int> * GetAppsvr(){
			std::pair<std::string,int> *p=NULL;
			int n=m_appSvr.size();
			if(n==1) p=&m_appSvr[0];
			else if(n>1){
				srand(clock());
				p=&m_appSvr[rand()%n]; //随机get一个application service得info
			}
			return p; 
		}

	private:
		virtual void onAccept(socketTCP *psock); //triggered when a new client connects to this service
		bool AnalysePASV(mportTCP* &pftpDatasvr,char *buf,int len,socketTCP *ppeer);
		static void transDataThread(std::pair<socketTCP *,FILE *> *p);
	private:
		//mapped application service
		std::vector<std::pair<std::string,int> > m_appSvr;
		MPORTTYPE m_apptype;//mapapplication servicetype
		int m_mportBegin;  //要求map port范围
		int m_mportEnd;
		char m_bindLocalIP[16]; //要求绑定的localIP
		SSLTYPE m_ssltype; //SSLconverttype
		bool m_bSSLVerify; //SSLservicewhether需要authenticationclientcertificate
		long m_lUserTag; //用于自define的扩充flag,对于本class无意义
		unsigned long m_maxratio; //limit maximum bandwidth kb/s
		bool m_bLogdatafile; //whether记录proxy serviceforward的data到logfile
		//modifyHTTPresponse头info int - HTTP response代码
		std::map<int,std::vector<RegCond> > m_modRspHeader;
		//modifyHTTPrequest头info string - 匹配HTTP requestURL
		std::map<std::string,std::vector<RegCond> > m_modReqHeader;
		//yyc add 2010-02-13 增加对url重写支持
		//first - 要匹配的url(支持Regexp) ，second - 替换为(Regexp)
		std::map<std::string,std::string> m_modURLRewriter;
	};
}//?namespace net4cpp21

#endif

