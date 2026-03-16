/*******************************************************************
   *	msnproxy.cpp
   *    DESCRIPTION:use MSN service as proxy to forward data
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2006-08-28
   *	
   *******************************************************************/

#include "../../include/sysconfig.h"
#include "../../include/cCoder.h"
#include "../../include/cLogger.h"
#include "../../include/Handle.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;

#ifdef __SUPPORT_MSNPROXY__

#define MSN_PROXY_DATA		0
#define MSN_PROXY_CONNECT	1
#define MSN_PROXY_CONNECTED	2
#define MSN_PROXY_CLOSED	3

unsigned long g_proxyID=0;
typedef struct _msnproxy_transInfo
{
	unsigned long remote_id;
	unsigned long local_id;
	bool bDelete;
	socketTCP *psock;
	HCHATSESSION hchat;
	msnMessager *pmsnmeaager;
}msnproxy_transInfo;
typedef THandle<msnproxy_transInfo> HTRANSINFO;
cMutex m_proxyMutex;
std::map<unsigned long,HTRANSINFO> m_mapProxyTransInfo;

unsigned long msnMessager::proxy_senddata(HCHATSESSION hchat,const char *proxyCmd,const char *content,int len)
{
	cContactor *pcon=(cContactor *)hchat;
	if(pcon==NULL || pcon->m_chatSock.status()!=SOCKS_CONNECTED) return 0;
	unsigned long trID=msgID();
	char msgHeader[256];//first generate MSG header //reserve 56 bytes of space for writing the MSG command header
	int headerlen=sprintf(msgHeader+56,"MIME-Version: 1.0\r\n"
						   "Content-Type: text/plain; charset=UTF-8\r\n"
						   "X-MMS-IM-Format: FN=MS Sans Serif; EF=; CO=0; CS=0; PF=0\r\n"
						   "MSN-Proxy: %s\r\n\r\n",proxyCmd);
	int iret=sprintf(msgHeader,"MSG %d A %d\r\n",trID,headerlen+len);
	memmove(msgHeader+(56-iret),msgHeader,iret);
	pcon->m_chatSock.Send(headerlen+iret,msgHeader+(56-iret),-1);
	pcon->m_chatSock.Send(len,content,-1);
	return trID;
}

//host - address and port of the application service to connect to: ip:port
void * msnMessager::proxy_createConnection(HCHATSESSION hchat,socketTCP *psock,const char *host)
{
	//send proxy connect request
	HTRANSINFO htransinfo(new msnproxy_transInfo);
	if(htransinfo.get_rep()==NULL) return NULL;
	htransinfo->local_id=++g_proxyID;
	htransinfo->remote_id=0;
	htransinfo->bDelete=false;
	htransinfo->psock=psock;
	htransinfo->hchat=hchat;
	htransinfo->pmsnmeaager=this;
	char proxyCmd[64]; 
	sprintf(proxyCmd,"%d %d-%d",MSN_PROXY_CONNECT,htransinfo->local_id,htransinfo->remote_id);
	if(!proxy_senddata(hchat,proxyCmd,host,strlen(host)) ) return NULL;
	m_proxyMutex.lock();
	m_mapProxyTransInfo[htransinfo->local_id]=htransinfo;
	m_proxyMutex.unlock();
	int iTimeout=30; //wait for command response, max 15 seconds
	while(htransinfo->remote_id==0 && --iTimeout>0)
	{
		if(htransinfo->psock->checkSocket(0,SOCKS_OP_READ)<SOCKSERR_OK) break;
		Sleep(500);
	}//?while
	if(htransinfo->remote_id!=0) return htransinfo.get_rep();
	m_proxyMutex.lock();
	m_mapProxyTransInfo.erase(htransinfo->local_id);
	m_proxyMutex.unlock();
	RW_LOG_PRINT(LOGLEVEL_WARN,"[MSN-Proxy] connect to remote service via MSN proxy bot %s failed!\r\n",host);
	return NULL;
}

//received MSN PROXY data
//proxyCmd - 代理command。format: <command> <remote-id>-<local-id>
void msnMessager::onProxyChat(HCHATSESSION hchat,const char *proxyCmd, char *undecode_szMsg,int szMsglen)
{
//	RW_LOG_PRINT(LOGLEVEL_WARN,"[MSN PROXY] %s msglen=%d\r\n",proxyCmd,szMsglen);

	long local_id=0,remote_id=0;
	int proxycmd=atoi(proxyCmd);
	const char *ptr=strchr(proxyCmd,' ');
	if(ptr==NULL) return; else ptr++;
	remote_id=atol(ptr); 
	if( (ptr=strchr(ptr,'-'))==NULL ) return;
	else local_id=atol(ptr+1);
	
	if(proxycmd==MSN_PROXY_CONNECTED) //connect对端success
	{
		m_proxyMutex.lock();
		std::map<unsigned long,HTRANSINFO>::iterator it=m_mapProxyTransInfo.find(local_id);
		if(it!=m_mapProxyTransInfo.end())
			(*it).second->remote_id=remote_id;
		m_proxyMutex.unlock();
	}
	else if(proxycmd==MSN_PROXY_CLOSED) //对端已close
	{
		m_proxyMutex.lock();
		std::map<unsigned long,HTRANSINFO>::iterator it=m_mapProxyTransInfo.find(local_id);
		if(it!=m_mapProxyTransInfo.end()) (*it).second->psock->Close();
		m_proxyMutex.unlock();
	}
	else if(proxycmd==MSN_PROXY_DATA) //代理forward data
	{
		socketTCP *psock=NULL;
		m_proxyMutex.lock();
		std::map<unsigned long,HTRANSINFO>::iterator it=m_mapProxyTransInfo.find(local_id);
		if(it!=m_mapProxyTransInfo.end()) psock=(*it).second->psock;
		m_proxyMutex.unlock();
		if(psock && psock->status()==SOCKS_CONNECTED)
		{
			//进行Base64 decoding
			szMsglen=cCoder::base64_decode(undecode_szMsg,szMsglen,undecode_szMsg);
			psock->Send(szMsglen,undecode_szMsg,-1); //forward data
		}
		else{
			char buf[64]; sprintf(buf,"%d %d-%d",MSN_PROXY_CLOSED,local_id,remote_id);
			proxy_senddata(hchat,buf,"closed\r\n",8);
		}
	}//?else if(proxycmd==MSN_PROXY_DATA)
	else if(proxycmd==MSN_PROXY_CONNECT) //connectspecified的application service器
	{
		HTRANSINFO htransinfo(new msnproxy_transInfo);
		if(htransinfo.get_rep()==NULL) return;
		htransinfo->local_id=++g_proxyID;
		htransinfo->remote_id=remote_id;
		htransinfo->hchat=hchat;
		htransinfo->pmsnmeaager=this;
		int port=0; const char *host=undecode_szMsg;
		const char *ptr=strchr(host,':');
		if(ptr) { port=atoi(ptr+1); *(char *)ptr=0; }
		htransinfo->bDelete=true;
		htransinfo->psock=new socketTCP;
		if(htransinfo->psock){
			htransinfo->psock->setParent(&m_curAccount.m_chatSock);
			htransinfo->psock->setRemoteInfo(host,port);
		}
		if( m_threadpool.addTask((THREAD_CALLBACK *)proxy_transThread,(void *)htransinfo.get_rep(),THREADLIVETIME)==0)
		{
			delete htransinfo->psock;
			char buf[64]; sprintf(buf,"%d %d-%d",MSN_PROXY_CLOSED,local_id,remote_id);
			proxy_senddata(hchat,buf,"closed\r\n",8);
		}else{
			m_proxyMutex.lock();
			m_mapProxyTransInfo[htransinfo->local_id]=htransinfo;
			m_proxyMutex.unlock();
		}
	}
	return;
}

//dataforward
void msnMessager::proxy_transferData(void *param,const char *otherdata,long datalen)
{
	msnproxy_transInfo *pinfo=(msnproxy_transInfo *)param;
	if(pinfo==NULL || pinfo->pmsnmeaager==NULL) return;
	msnMessager *pmsnmeaager=pinfo->pmsnmeaager;
	socketTCP *psock=pinfo->psock;
	unsigned long local_id=pinfo->local_id;
	unsigned long remote_id=pinfo->remote_id;
	char proxyCmd[24];

	if(psock)
	{
		if(pinfo->bDelete) //connectspecified的application service
		{
			if(psock->Connect(NULL,0,8)>0)
			{
				sprintf(proxyCmd,"%d %d-%d",MSN_PROXY_CONNECTED,local_id,remote_id);
				pmsnmeaager->proxy_senddata(pinfo->hchat,proxyCmd,"success\r\n",9);
			}
		}//?if(psock->parent()==NULL)
		
		//----------------startdataforward-------------------
		sprintf(proxyCmd,"%d %d-%d",MSN_PROXY_DATA,local_id,remote_id);
		unsigned long trID=0; cCond cond; cond.setArgs(0);
		int iret; char *buf=new char[1024+SSENDBUFFERSIZE];
		while(buf && psock->status()==SOCKS_CONNECTED)
		{
			if(datalen>0)
			{
				iret=(datalen>1024)?1024:datalen;
				::memcpy(buf,otherdata,iret);
				datalen-=iret; otherdata+=iret;
			}else{
				iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
				if(iret<0) break; 
				if(iret==0) continue;
				//read data sent by client
				iret=psock->Receive(buf,1024,-1);
				if(iret<0) break; //==0 means received data exceeded the limit
				if(iret==0){ Sleep(200); continue; }
			}
			//进行Base64 encoding
			iret=cCoder::base64_encode(buf,iret,buf+1024);
			trID=pmsnmeaager->proxy_senddata(pinfo->hchat,proxyCmd,buf+1024,iret);
			if(trID==0) break;
			if(trID%4==0) //为了避免send太快导致MSNserviceclose connection，waitingresponse应答
			{
				pmsnmeaager->m_conds[trID]=&cond; //延时waiting3秒
				cond.wait(3); pmsnmeaager->eraseCond(trID);
			}//?if(trID%4==0)
		}//?while
		//----------------dataforwardend-------------------

		if(pinfo->bDelete) delete psock;
	}//?if(psock
	
	sprintf(proxyCmd,"%d %d-%d",MSN_PROXY_CLOSED,local_id,remote_id);
	pmsnmeaager->proxy_senddata(pinfo->hchat,proxyCmd,"closed\r\n",8);

	m_proxyMutex.lock();
	m_mapProxyTransInfo.erase(local_id);
	m_proxyMutex.unlock();
	return;
}

#endif
