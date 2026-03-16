/*******************************************************************
   *	proxysvr.h
   *    DESCRIPTION:proxy server declaration
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-08-24
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#ifndef __YY_PROXY_SERVER_H__
#define __YY_PROXY_SERVER_H__

#include "proxydef.h"
#include "socketSvr.h"

namespace net4cpp21
{
	class cProxysvr
	{
	public:
		cProxysvr();
		virtual ~cProxysvr(){}
		//set proxy service supported types
		void setProxyType(int itype) { m_proxytype=itype; }
		void setProxyAuth(bool bAuth) { m_bProxyAuthentication=bAuth; }
		bool setCascade(const char *casHost,int casPort,int type,const char *user,const char *pswd);
		bool getIfLogdata() const { return m_bLogdatafile; }
		void setIfLogdata(bool b){ m_bLogdatafile=b; }
	protected:
		//get account info, return the specified account object
		PROXYACCOUNT *getAccount(const char *struser);
		//add new account info
		PROXYACCOUNT *newAccount(const char *struser);
		SOCKSRESULT delAccount(const char *struser);
		void onConnect(socketTCP *psock);//a user has connected
		//create forward-to task thread
		virtual bool onTransferTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return false;
		}
		//received forwarded data, used for data analysis handling
		virtual void onData(char *buf,long len,socketTCP *from,socketTCP *to)
		{ return; }
	private:
		std::pair<std::string,int> * GetCassvr(){ //get secondary proxy settings
			std::pair<std::string,int> *p=NULL;
			int n=m_vecCassvr.size();
			if(n==1) p=&m_vecCassvr[0];
			else if(n>1){
				srand(clock());
				p=&m_vecCassvr[rand()%n]; //随机get一个application service得info
			}
			return p; 
		}

		PROXYACCOUNT * ifAccess(socketTCP *psock,const char *user,const char *pwd,int * perrCode=NULL);
		void doSock4req(socketTCP *psock);
		void doSock5req(socketTCP *psock);
		void doHttpsreq(socketTCP *psock);
		void transData(socketTCP *psock,socketTCP *peer,const char *sending_buf,long sending_size);
		static void transThread(void *pthreadParam);
	private:
		int m_proxytype;//本proxy service支持的代理type
		bool m_bProxyAuthentication;//本servicewhether需要authentication
		//此proxy service的accountinfo
		std::map<std::string,PROXYACCOUNT> m_accounts;
		//secondary proxy related parameters
		bool m_bCascade; //whether支持secondary proxy,支持multiplesecondary proxy service器，随机选择
		std::vector<std::pair<std::string,int> > m_vecCassvr;
//		std::string m_casProxysvr; //secondary proxy service port
//		int m_casProxyport; 
		int m_casProxytype; //secondary proxy supported types
		bool m_casProxyAuthentication; //whether secondary proxy requires authentication
		std::pair<std::string,std::string> m_casAccessAuth;
		bool m_bLogdatafile; //whether记录proxy serviceforward的data到logfile
	};

	class proxyServer : public socketSvr,public cProxysvr
	{
	public:
		proxyServer();
		virtual ~proxyServer();
	private:
		//triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock)
		{
			cProxysvr::onConnect(psock);
			return;
		}
		//create forward-to task thread
		virtual bool onTransferTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return (m_threadpool.addTask(pfunc,pargs,THREADLIVETIME)!=0);
		}
	};
}//?namespace net4cpp21

#endif

