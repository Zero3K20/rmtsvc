/*******************************************************************
   *	mapport.cpp
   *    DESCRIPTION:port mapping service
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

#include "../include/sysconfig.h"
#include "../include/mapport.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

#define MAX_TRANSFER_BUFFER 2048
#define MAX_CONNECT_TIMEOUT 15

mapServer :: mapServer()
{
	m_strSvrname="map Server";
}

mapServer :: ~mapServer()
{
	Close();
	//service析构前要保证thread都end，因为thread中访问了mapServerclass的object
	m_threadpool.join(); 
}

//set the application service to map
bool mapServer :: mapped(const char *appsvr,int appport,int apptype)
{
	if(appsvr==NULL) return false;
	std::pair<std::string,int> p(appsvr,appport);
	const char *ptr=strchr(appsvr,':');
	if(ptr){
		*(char *)ptr=0; p.first.assign(appsvr); *(char *)ptr++=':';
		if(appport<=0){ while(*ptr==' ') ptr++; appport=atoi(ptr); }
	}
	if(appport<=0) return false; else p.second=appport;
	m_mappedApp.push_back(p);
	return true;
}

socketTCP * mapServer :: connect_mapped(std::pair<std::string,int>* &p)
{
	int n=m_mappedApp.size();
	if(n==1) 
		p=&m_mappedApp[0];
	else if(n>0) //随机get一个application service得info
		p=&m_mappedApp[rand()%n];
	else return NULL;
	socketTCP *psock=new socketTCP;
	if(psock==NULL) return NULL;
	psock->setParent(this);
	if( psock->Connect(p->first.c_str(),p->second,MAX_CONNECT_TIMEOUT)> 0)
		return psock;
	delete psock; return NULL;
}

//triggered when a new client connects to this service
void mapServer :: onAccept(socketTCP *psock)
{
	//connect被map的service
	std::pair<std::string,int> *p=NULL;
	socketTCP *ppeer=connect_mapped(p);
	if(ppeer==NULL){
		RW_LOG_DEBUG("Can not connect Mapped server(%s:%d)\r\n",
			((p)?p->first.c_str():""),((p)?p->second:0) );	
		return;
	}else ppeer->setParent(psock);
	
	onData((char *)1,0,psock,ppeer); //用于分析handledata，start一个connect
	if( m_threadpool.addTask((THREAD_CALLBACK *)&transThread,(void *)ppeer,THREADLIVETIME)==0 )
	{
		RW_LOG_DEBUG(0,"Failed to create transfer-Thread\r\n");
		return;
	}
	char buf[MAX_TRANSFER_BUFFER]; //startforward
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//read data sent by client
		iret=psock->Receive(buf,MAX_TRANSFER_BUFFER-1,-1);
		if(iret<0) break; //==0 means received data exceeded the limit
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buf[iret]=0; onData(buf,iret,psock,ppeer);
		iret=ppeer->Send(iret,buf,-1);
		if(iret<0) break;
	}//?while
	ppeer->Close(); onData(NULL,0,psock,ppeer);
	while(ppeer->parent()!=NULL) cUtils::usleep(SCHECKTIMEOUT);
	onData(NULL,0,ppeer,psock); //用于notificationprotocol分析打印程序connect已经close，可release相关资源
	delete ppeer; return;
}

//forwardthread
void mapServer :: transThread(socketTCP *psock)
{
	if(psock==NULL) return;
	socketTCP *ppeer=(socketTCP *)psock->parent();
	if(ppeer==NULL) return;
	mapServer *psvr=(mapServer *)ppeer->parent();

	char buf[MAX_TRANSFER_BUFFER]; //startforward
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//read data sent by client
		iret=psock->Receive(buf,MAX_TRANSFER_BUFFER-1,-1);
		if(iret<0) break; //==0 means received data exceeded the limit
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buf[iret]=0; psvr->onData(buf,iret,psock,ppeer);
		iret=ppeer->Send(iret,buf,-1);
		if(iret<0) break;
	}//?while
	ppeer->Close(); 
	psock->setParent(NULL); //用于onAcceptthread判断forwardthreadyesnoend
	return;
}
