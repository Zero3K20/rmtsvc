/*******************************************************************
   *	sniffer.h
   *    DESCRIPTION:sniffer class declaration (promiscuous mode)
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SNIFFER_H__
#define __YY_SNIFFER_H__

#include "socketRaw.h"
#include "IPRules.h"
#include "cThread.h"

namespace net4cpp21
{
	class sniffer : public socketRaw
	{
	public:
		sniffer(){}
		virtual ~sniffer();
		virtual void Close();
		iprules &rules() { return m_rules;}
		void setLogfile(const char *filename){
			if(filename!=NULL && filename[0]!=0)
				m_logfile.assign(filename);
			else m_logfile="";
			return;
		}
		//whether to log IP packets to file
		bool ifLog()  const { return m_logfile!=""; }
		//open IP log file and simulate sniff
		bool openLogfile(const char *logfile);

		//bindip==NULL或==""default绑定local machine第一个ip
		//否则绑定specified的ip,createRaw socket启动sniff
		//success返回SOCKSERR_OK，(启动sniff必须绑定IP，以便specified设置promiscuous mode的网卡,否则报10022error)
		SOCKSRESULT sniff(const char *bindip);
	protected:
		//有data到达
		virtual void onData(char *dataptr);
	private:
		std::string m_logfile; //ip包记录文件
		iprules m_rules;//过滤规则
		cThread m_thread;
		static void sniffThread(sniffer *psniffer);
		static void sniffThread_fromfile(sniffer *psniffer);
	}; 

}//?namespace net4cpp21

#endif

