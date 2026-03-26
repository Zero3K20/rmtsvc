/*******************************************************************
   *	proxysvr.cp
   *    DESCRIPTION:proxy server implementation
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

#include "../include/sysconfig.h"
#include "../include/proxysvr.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

cProxysvr :: cProxysvr()
{
	m_proxytype=PROXY_HTTPS|PROXY_SOCKS4|PROXY_SOCKS5;
	m_bProxyAuthentication=false;

	//secondary proxy related parameters
	m_bCascade=false; //defaultnot supportedsecondary proxy
//	m_casProxysvr="";
//	m_casProxyport=0; 
	m_casProxytype=PROXY_HTTPS|PROXY_SOCKS4|PROXY_SOCKS5; //secondary proxy supported types
	m_casProxyAuthentication=false; //whether secondary proxy requires authentication
	m_bLogdatafile=false;
}


SOCKSRESULT cProxysvr :: delAccount(const char *struser)
{
	if((long)struser==-1){ m_accounts.clear(); return SOCKSERR_OK;}
	if(struser==NULL || struser[0]==0) return SOCKSERR_PROXY_USER;
	::strlwr((char *)struser);
	std::map<std::string,PROXYACCOUNT>::iterator it=m_accounts.find(struser);
	if(it==m_accounts.end()) return SOCKSERR_PROXY_USER;
	if((*it).second.m_loginusers>0) return SOCKSERR_PROXY_DENY;
	m_accounts.erase(it);
	return SOCKSERR_OK;
}

//get account info, return the specified account object
PROXYACCOUNT * cProxysvr :: getAccount(const char *struser)
{
	if(struser==NULL || struser[0]==0) return NULL;
	::strlwr((char *)struser);
	std::map<std::string,PROXYACCOUNT>::iterator it=m_accounts.find(struser);
	if(it!=m_accounts.end()) return &(*it).second;
	return NULL;
}

//add new account info
PROXYACCOUNT * cProxysvr :: newAccount(const char *struser)
{
	if(struser==NULL || struser[0]==0) return NULL;
	PROXYACCOUNT proa;
	proa.m_username.assign(struser);
	proa.m_maxratio=0;
	proa.m_loginusers=0;
	proa.m_maxLoginusers=0;
	proa.m_limitedTime=0;

	::strlwr((char *)struser);
	std::map<std::string,PROXYACCOUNT>::iterator it=m_accounts.find(struser);
	if(it!=m_accounts.end()){
		if((*it).second.m_loginusers>0) return NULL;
		m_accounts.erase(it);
	}
	m_accounts[struser]=proa;
	it=m_accounts.find(struser);
	if(it==m_accounts.end()) return NULL;
	return &(*it).second;
}

//set secondary proxy related parameters m_vecCassvr
bool cProxysvr::setCascade(const char *casHost,int casPort,int type,const char *user,const char *pswd)
{
	m_vecCassvr.clear(); //clearsecondary proxy
	if(casHost && casHost[0]!=0)
	{
		const char *ptrEnd,*ptrBegin=casHost;
		while(true)
		{
			while(*ptrBegin==' ') ptrBegin++;
			ptrEnd=strchr(ptrBegin,',');
			if(ptrEnd) *(char *)ptrEnd=0;

			const char *ptr=strchr(ptrBegin,':');
			int hostport=casPort;
			if(ptr){ *(char *)ptr=0; hostport=atoi(ptr+1);}
			if(hostport>0 && ptrBegin[0]!=0)
			{ //secondary proxy service address and port valid
				std::pair<std::string,int> p(ptrBegin,hostport);
				m_vecCassvr.push_back(p);
			}
			if(ptr) *(char *)ptr=':';

			if(ptrEnd) *(char *)ptrEnd=',';
			else break;
			ptrBegin=ptrEnd+1;
		}//?while
	}//?if(casHost && casHost[0]!=0) 
	m_bCascade=(m_vecCassvr.size()>0)?true:false; //if set, supports cascaded secondary proxy
/*	if(casHost==NULL || casHost[0]==0 || casPort<=0)
	{//not supportedsecondary proxy
		m_bCascade=false;
	}else{
		m_bCascade=true;
		m_casProxysvr.assign(casHost);
		m_casProxyport=casPort;
	} */

	m_casProxytype=type;
	if(user && user[0]!=0)
		m_casProxyAuthentication=true;
	else
		m_casProxyAuthentication=false;
	if(user) m_casAccessAuth.first.assign(user);
	if(pswd) m_casAccessAuth.second.assign(pswd);
	return m_bCascade;
}


PROXYACCOUNT * cProxysvr::ifAccess(socketTCP *psock,const char *user,const char *pwd,int * perrCode)
{
	if(perrCode) *perrCode=SOCKSERR_PROXY_USER;
	if(user==NULL || psock==NULL) return NULL;
	while(*user==' ') user++;//delete leading spaces
	::_strlwr((char *)user);//convert account name to lowercase
	std::map<std::string,PROXYACCOUNT>::iterator it=m_accounts.find(user);
	if(it==m_accounts.end()) return NULL;
	
	PROXYACCOUNT &proa=(*it).second;
	std::string strPwd;
	if(pwd) strPwd.assign(pwd);
	if(proa.m_userpwd!="" && proa.m_userpwd!=strPwd)
		return NULL; //password error //if password is empty, no password verification required
	
	if(proa.m_limitedTime>0 && time(NULL)>proa.m_limitedTime ){
		if(perrCode) *perrCode=SOCKSERR_PROXY_EXPIRED;
		return NULL; //account expired
	}
	if(!proa.m_ipRules.check(psock->getRemoteip(),psock->getRemotePort(),RULETYPE_TCP) ){
		if(perrCode) *perrCode=SOCKSERR_PROXY_DENY;
		return NULL;
	}
	if(proa.m_maxLoginusers>0 && proa.m_loginusers>proa.m_maxLoginusers){
		if(perrCode) *perrCode=SOCKSERR_PROXY_CONNECTIONS;
		return NULL;
	}

	if(perrCode) *perrCode=SOCKSERR_OK;
	proa.m_loginusers++; //set data channel bandwidth limit
	psock->setSpeedRatio(proa.m_maxratio*1024,proa.m_maxratio*1024);
	return &proa;
}


void cProxysvr::onConnect(socketTCP *psock)
{
	char ctype; int iret;
	if((m_proxytype & 0x7)==0) return;
	iret=psock->Peek(&ctype,1,PROXY_MAX_RESPTIMEOUT);
	if(iret<=0) return;
	
	//determine the proxy service type
	if(ctype==0x04) //may have received a socks4 proxy request
	{
		if((m_proxytype & PROXY_SOCKS4)==0) return; //this proxy does not support SOCKS4 proxy protocol
		doSock4req(psock);
	}
	else if(ctype==0x05) //may have received a socks5 proxy request
	{
		if((m_proxytype & PROXY_SOCKS5)==0) return; //this proxy does not support socks5 proxy protocol
		doSock5req(psock);
	}
	else if((m_proxytype & PROXY_HTTPS)!=0) //this proxy supports HTTPS proxy protocol
		doHttpsreq(psock);
	return;
}

typedef struct _TransThread_Param
{
	socketTCP * psock;
	FILE *fp;
	cProxysvr *psvr;
}TransThread_Param;

//dataforward
void cProxysvr :: transData(socketTCP *psock,socketTCP *peer,const char *sending_buf,long sending_size)
{
//	ASSERT(psock);
//	ASSERT(peer);
	char buf[SSENDBUFFERSIZE];
	FILE *fp=NULL;
	if(m_bLogdatafile){ //whether to log proxy data
		char logfilename[256]; //log filename
		int logfilenameLen=sprintf(logfilename,"%s(%d)-",psock->getRemoteIP(),psock->getRemotePort());
		logfilenameLen+=sprintf(logfilename+logfilenameLen,"%s(%d).log",peer->getRemoteIP(),peer->getRemotePort());
		logfilename[logfilenameLen]=0;
		if( (fp=::fopen(logfilename,"ab")) )
		{
			logfilenameLen=sprintf(logfilename,"C %s:%d <----> S ",psock->getRemoteIP(),psock->getRemotePort());
			::fwrite(logfilename,1,logfilenameLen,fp);
			logfilenameLen=sprintf(logfilename,"%s:%d\r\n",peer->getRemoteIP(),peer->getRemotePort());
			::fwrite(logfilename,1,logfilenameLen,fp);
		}
	}//?if(m_bLogdatafile)

	onData((char *)1,0,psock,peer); //used to analyze/handle data, start a connection
	if(sending_buf && sending_size>0)//data pending send
	{
		onData((char*)sending_buf,sending_size,psock,peer);
		peer->Send(sending_size,sending_buf,-1);
///#ifdef PROXYDATA_LOG //whether to log proxy data
		if(fp){
			::fwrite("\r\nC ---> S\r\n",1,12,fp);
			::fwrite(sending_buf,1,sending_size,fp);
		}
///#endif
	}//?if(sending_buf && sending_size>0)

	peer->setSpeedRatio(psock->getMaxSendRatio(),psock->getMaxRecvRatio());
	TransThread_Param ttParam; ttParam.psvr =this;
	ttParam.psock =peer; ttParam.fp =fp;
//	std::pair<socketTCP *,FILE *> p(peer,fp);
	if(onTransferTask((THREAD_CALLBACK *)&transThread,(void *)&ttParam))
	{
		while( psock->status()==SOCKS_CONNECTED )
		{
			int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break; 
			if(iret==0) continue;
			//read data sent by client
			iret=psock->Receive(buf,SSENDBUFFERSIZE,-1);
			if(iret<0) break; //==0 means received data exceeded the limit
			if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
///#ifdef PROXYDATA_LOG //whether to log proxy data
			if(fp){
				::fwrite("\r\nC ---> S\r\n",1,12,fp);
				::fwrite(buf,1,iret,fp);
			}
///#endif
			onData(buf,iret,psock,peer);
			iret=peer->Send(iret,buf,-1);
			if(iret<0) break;
		}//?while
		peer->Close(); //waitingforwardthread end
		while(peer->parent()!=NULL) cUtils::usleep(SCHECKTIMEOUT);
		onData(NULL,0,peer,psock); //notify protocol analyzer that connection has closed, release related resources
	}else{//?if(onTransferTask
		peer->Close(); peer->setParent(NULL);
		RW_LOG_DEBUG(0,"Failed to create transfer-Thread\r\n");
	}

///#ifdef PROXYDATA_LOG //whether to log proxy data
	if(fp){
		::fwrite("\r\n***Proxy End*** \r\n",1,20,fp);
		::fclose(fp);
	}
///#endif
	return;
}

//forwardthread
void cProxysvr :: transThread(void *pthreadParam)
{	
//	std::pair<socketTCP *,FILE *> &p=*(std::pair<socketTCP *,FILE *> *)pthreadParam;
//	FILE *fp=p.second; cProxysvr *psvr=NULL;
//	socketTCP *psock=p.first;
	
	TransThread_Param *ptParam=(TransThread_Param *)pthreadParam;
	FILE *fp=ptParam->fp; socketTCP *psock=ptParam->psock;
	cProxysvr *psvr=ptParam->psvr;

	if(psock==NULL) return;
	socketTCP *ppeer=(socketTCP *)psock->parent();
	if(ppeer==NULL) return;

	char buf[SSENDBUFFERSIZE]; //startforward
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//read data sent by client
		iret=psock->Receive(buf,SSENDBUFFERSIZE,-1);
		if(iret<0) break; //==0 means received data exceeded the limit
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
///#ifdef PROXYDATA_LOG //whether to log proxy data
			if(fp){
				::fwrite("\r\nS ---> C\r\n",1,12,fp);
				::fwrite(buf,1,iret,fp);
			}
///#endif
		if(psvr) psvr->onData(buf,iret,psock,ppeer);
		iret=ppeer->Send(iret,buf,-1);
		if(iret<0) break;
	}//?while
	
	ppeer->Close(); 
	psock->setParent(NULL); //used by onAccept thread to check whether forward thread has ended
	return;
}

//------------------------------------------------------------
proxyServer :: proxyServer()
{
	m_strSvrname="Proxy Server";
}
proxyServer :: ~proxyServer()
{
	 Close();
	 m_threadpool.join();
}
