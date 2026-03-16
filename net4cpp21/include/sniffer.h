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

		//if bindip==NULL or =="", bind to the local machine's first IP by default
		//otherwise bind to the specified IP and create a raw socket to start sniffing
		//returns SOCKSERR_OK on success，(startsniff必须绑定IP，以便specifiedsetpromiscuous mode的网卡,otherwise报10022error)
		SOCKSRESULT sniff(const char *bindip);
	protected:
		//data has arrived
		virtual void onData(char *dataptr);
	private:
		std::string m_logfile; //IP packet log file
		iprules m_rules;//过滤规则
		cThread m_thread;
		static void sniffThread(sniffer *psniffer);
		static void sniffThread_fromfile(sniffer *psniffer);
	}; 

}//?namespace net4cpp21

#endif

