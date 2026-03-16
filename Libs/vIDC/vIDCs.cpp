/*******************************************************************
   *	vIDCs.cpp
   *    DESCRIPTION:vIDC service class implementation
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/

#include "../../net4cpp21/include/sysconfig.h"
#include "../../net4cpp21/utils/utils.h"
#include "../../net4cpp21/include/cLogger.h"
#include "vidcs.h"

using namespace std;
using namespace net4cpp21;

vidcsvr :: vidcsvr()
{
	m_bAuthentication=false;
}

vidcsvr :: ~vidcsvr() { Destroy(); }

void vidcsvr :: Destroy()
{
	std::map<long,vidccSession *>::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();it++)
	{
		vidccSession *pvidcc=(*it).second;
		if(pvidcc!=NULL) pvidcc->Destroy();
		delete pvidcc;
	}
	m_sessions.clear();
}

bool vidcsvr :: DisConnect(long vidccID) //forcibly disconnect a vidcc connection
{
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it==m_sessions.end()) return false;
	vidccSession *pvidcc=(*it).second;
	//closing the socket connection will automatically delete this vidccSession in onConnect
	if(pvidcc) pvidcc->Close(); else m_sessions.erase(it); 
	return true;
}
//enable logging for the service mapped by a vidcc
void vidcsvr :: setLogdatafile(long vidccID,bool b)
{
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it==m_sessions.end()) return;
	vidccSession *pvidcc=(*it).second;
	pvidcc->setIfLogdata(b); 
	return;
}

void vidcsvr :: xml_list_vidcc(cBuffer &buffer)
{
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str()==NULL) return;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidccs>");
	
	std::map<long,vidccSession *>::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();it++)
	{
		if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
		if(buffer.str()==NULL) return;
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcc>");
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vID>%d</vID>",(*it).first);
		vidccSession *pvidcc=(*it).second;
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vname>%s</vname>",pvidcc->vidccName());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vip>%s</vip>",pvidcc->vidccIP());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcc>");
	}
		
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidccs>");
	return;
}

void vidcsvr :: xml_info_vidcc(cBuffer &buffer,long vidccID)
{
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it==m_sessions.end())
	{
		if(buffer.Space()<48) buffer.Resize(buffer.size()+48);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>inValid vidccID</retmsg>");
		return;
	}
	vidccSession *pvidcc=(*it).second;
	if(pvidcc==NULL) return;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcc_status>");
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vID>%d</vID>",vidccID);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vname>%s</vname>",pvidcc->vidccName());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vip>%s</vip>",pvidcc->vidccIP());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vdesc>%s</vdesc>",pvidcc->vidccDesc());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ver>%.1f</ver>",pvidcc->vidccVer()/10.0);
	time_t t=pvidcc->ConnectTime(); struct tm *ltime=localtime(&t);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<starttime>%04d-%02d-%02d %02d:%02d:%02d</starttime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	pvidcc->xml_list_mapped(buffer);
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcc_status>");
	return;
}

//a new connection has arrived
void vidcsvr::onConnect(socketTCP *psock)
{
	socketBase *psocketsvr=psock->parent(); 
	if(psocketsvr==NULL) return; //save the socket object of the vidcs service
	char buf[VIDC_MAX_COMMAND_SIZE];
	int iret=psock->Receive(buf,VIDC_MAX_COMMAND_SIZE-1,VIDC_MAX_RESPTIMEOUT);
	//if error occurs or no data received within specified time, return and disconnect
	if(iret<=0) return; else buf[iret]=0;
	
	RW_LOG_DEBUG("[vidcSvr] c--->s:\r\n\t%s\r\n",buf);
	if(strncmp(buf,"HELO ",5)==0)
	{//a certainclientconnect登陆
		//create新的vidccSession并add到m_sessionsqueue中
		vidccSession *pvidcc=docmd_helo(psock,buf+5);
		if(pvidcc==NULL) return; //登陆failureclose connection
		char buf[VIDC_MAX_COMMAND_SIZE]; int buflen=0;//进入commandhandle循环
		time_t tLastReceived=time(NULL);
		while(psocketsvr->status()==SOCKS_LISTEN && 
			  psock->status()==SOCKS_CONNECTED )
		{
			iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break;
			if(iret==0)
				if((time(NULL)-tLastReceived)>VIDC_MAX_CLIENT_TIMEOUT) break; else continue;
			//read data sent by client
			iret=psock->Receive(buf+buflen,VIDC_MAX_COMMAND_SIZE-buflen-1,-1);
			if(iret<0) break; //==0 means received data exceeded the limit
			if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
			tLastReceived=time(NULL); //last一次receive到datatime
			buflen+=iret; buf[buflen]=0; //parsevIDCcommand
			const char *ptrCmd,*ptrBegin=buf;
			while( (ptrCmd=strchr(ptrBegin,'\r')) )
			{
				*(char *)ptrCmd=0;//startparsecommand
				if(ptrBegin[0]==0) goto NextCMD; //nothandlenull行data
				if(strncmp(ptrBegin,"SSLC ",5)==0) //seta certainmapservice的clientauthenticationcertificateinfo
				{//format: SSLC name=<XXX> certlen=<certificatebyte> keylen=<私钥byte> pwdlen=<passwordlength>\r\n后续byte\r\n
					const char *ptrData=ptrCmd+1;
					if(*ptrData=='\r' || *ptrData=='\n') ptrData++; //skip \r\n
					long dataLen=buflen-(ptrData-buf); //getreceivebuffer中还未分析handle的data
					//docmd_sslcreturn从receivebuffer中取出handle的data个数
					dataLen=pvidcc->docmd_sslc(ptrBegin+5,ptrData,dataLen);
					ptrBegin=ptrData+dataLen; continue; //continue下面的handle
				}else pvidcc->parseCommand(ptrBegin);

NextCMD:		//移动ptrBegin到nextcommanddata起始
				ptrBegin=ptrCmd+1; 
				while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //skip \r\n
			}//?while( (ptrCmd=
			//if有未receive完的command则移动
			if((iret=(ptrBegin-buf))>0 && (buflen-iret)>0)
			{//ifptrBegin-buf==0说明这is aerrorcommanddatapacket
				buflen-=iret;
				memmove((void *)buf,ptrBegin,buflen);
			} else buflen=0;
		}//?while...
		//从m_sessionsqueue中deletevidccSession
		m_mutex.lock();
		std::map<long,vidccSession *>::iterator it=m_sessions.find((long)pvidcc);
		if(it!=m_sessions.end()) m_sessions.erase(it);
		m_mutex.unlock(); 
		pvidcc->Destroy(); delete pvidcc; //Destroy会waitingall的管道end
	}//?if(strncmp(buf,"HELO ",5)==0)
	else if(strncmp(buf,"PIPE ",5)==0)
	{//此connectyesa certainvIDCc的管道connect
		//create一个管道，这样atonConnectexit后releasepsockwill not影响到forwardthread
		socketTCP *pipe=new socketTCP(*psock);
		//setnull的父object，以便下面waiting管道绑定
		if(pipe==NULL) return; else pipe->setParent(NULL);
		long vidccID=atol(buf+5);
		vidccSession *pvidcc=AddPipeFromVidcSession(pipe,vidccID);
		if(pvidcc==NULL){ delete pipe; return; }
		
		time_t t=time(NULL);
		while(psocketsvr->status()==SOCKS_LISTEN && 
		//	  pvidcc->isConnected() && //也许pvidcc已经被releasedelete了
			  pipe->status()==SOCKS_CONNECTED )
		{//if管道绑定success，则pipe的parentpointer to绑定的对端
			if(pipe->parent()!=NULL) return; //已经绑定了forwardsocket
			//specified的time内还没有被绑定则end
			else if((time(NULL)-t)>VIDC_PIPE_ALIVETIME) break; 
			cUtils::usleep(200000); //延迟200ms
		}//?while
		if(DelPipeFromVidcSession(pipe,vidccID)) delete pipe;
		//otherwise此管道已经被取走占用,notdelete
//		else{ pipe->Close(); pipe->setParent(NULL); }
	}//?else if(strncmp(ptrBegin,"PIPE ",5)==0)
	//yyc add 2007-08-21 surpport MakeHolecommand，用于TCP穿洞
	else if(strcmp(buf,"HOLE\r\n")==0)
	{
		//returnconnect的IPaddressandport
		iret=sprintf(buf,"200 %s:%d\r\n",psock->getRemoteIP(),psock->getRemotePort());
		iret=psock->Send(iret,buf,-1);
		//循环waiting对方releaseconnect
		while(psocketsvr->status()==SOCKS_LISTEN && 
			  psock->status()==SOCKS_CONNECTED )
		{
			iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break;
			if(iret==0) continue;
			//if对方温andclose了connect则checkSocket检测==1,但读data::recv会return0
			iret=psock->Receive(buf,VIDC_MAX_COMMAND_SIZE-1,-1);
			if(iret<0) break;
		}//?while
	}//?else if(strcmp(buf,"HOLE\r\n")==0)
	//一个新的connect过来，起始只有可能sendHELOandPIPEcommand,对于其他request则close connection
}
//clientlogin authentication
//	format:HELO <SP> <NAME:pswd> <SP> VER <SP> DESC
vidccSession * vidcsvr :: docmd_helo(socketTCP *psock,const char *param)
{
	int len,clntVer=0xff; char buf[64];
	string strUser,strPswd,strDesc;
	const char *ptr1,*ptr=strchr(param,' ');
	if(ptr){ //getNameandpassword
		*(char *)ptr=0;
		if( (ptr1=strchr(param,':')) )
		{
			strPswd.assign(ptr1+1);
			strUser.assign(param,ptr1-param);
		}else strUser.assign(param);
		*(char *)ptr=' '; param=ptr+1;
		//getversion number
		if( (ptr=strchr(param,' ')) )
		{
			strDesc.assign(ptr+1);
			clntVer=atoi(param);
		}else clntVer=atoi(param);
	}//?if(ptr)
	if(clntVer<VIDCC_MIN_VERSION){
		len=sprintf(buf,msg_err_405,VIDCS_VERSION);
		psock->Send(len,buf,-1); return NULL;
	}
	if(m_bAuthentication && m_strPswd!=strPswd){
		len=sprintf(buf,msg_err_221,VIDCS_VERSION);
		psock->Send(len,buf,-1); return NULL;
	}
	vidccSession *pvidcc=new vidccSession(psock,clntVer,strUser.c_str(),strDesc.c_str());
	if(pvidcc==NULL) return NULL;
	m_mutex.lock();
	m_sessions[(long)pvidcc]=pvidcc;
	m_mutex.unlock();
	len=sprintf(buf,"200 %d %d command success.\r\n",(long)pvidcc,VIDCS_VERSION);
	psock->Send(len,buf,-1); return pvidcc;
}
//vIDCclient新建立了一个管道
//	format:PIPE <SP> CLNTID <CRLF>
vidccSession * vidcsvr :: AddPipeFromVidcSession(socketTCP *pipe,long vidccID)
{
	vidccSession *pvidcc=NULL;
	m_mutex.lock();
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it!=m_sessions.end())
	{
		pvidcc=(*it).second;
		if(!pvidcc->AddPipe(pipe)) pvidcc=NULL;
//		RW_LOG_DEBUG("[vidcsvr] new pipe from vidccID %d\r\n",vidccID);
	}
	m_mutex.unlock(); return pvidcc;
}
bool vidcsvr :: DelPipeFromVidcSession(socketTCP *pipe,long vidccID)
{
	bool bret=false; m_mutex.lock();
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it!=m_sessions.end())
	{
		vidccSession *pvidcc=(*it).second;
		if(pvidcc) bret=pvidcc->DelPipe(pipe);
	}
	m_mutex.unlock(); return bret;
}
//------------------------------------------------------------
vidcServer :: vidcServer()
{
	m_strSvrname="vIDC Server";
}
void vidcServer :: Stop()
{
	Close();
	vidcsvr::Destroy();
	m_threadpool.join();
}


