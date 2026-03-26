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
	{//a client connection login
		//create a new vidccSession and add it to the m_sessions queue
		vidccSession *pvidcc=docmd_helo(psock,buf+5);
		if(pvidcc==NULL) return; //login failure, close connection
		char buf[VIDC_MAX_COMMAND_SIZE]; int buflen=0;//enter command handling loop
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
			tLastReceived=time(NULL); //time of last received data
			buflen+=iret; buf[buflen]=0; //parsevIDCcommand
			const char *ptrCmd,*ptrBegin=buf;
			while( (ptrCmd=strchr(ptrBegin,'\r')) )
			{
				*(char *)ptrCmd=0;//startparsecommand
				if(ptrBegin[0]==0) goto NextCMD; //do not handle empty line data
				if(strncmp(ptrBegin,"SSLC ",5)==0) //set client authentication certificate info for a certain map service
				{//format: SSLC name=<XXX> certlen=<certificate bytes> keylen=<private key bytes> pwdlen=<password length>\r\n followed by data bytes\r\n
					const char *ptrData=ptrCmd+1;
					if(*ptrData=='\r' || *ptrData=='\n') ptrData++; //skip \r\n
					long dataLen=buflen-(ptrData-buf); //get unprocessed data remaining in the receive buffer
					//docmd_sslc returns the number of bytes consumed from the receive buffer
					dataLen=pvidcc->docmd_sslc(ptrBegin+5,ptrData,dataLen);
					ptrBegin=ptrData+dataLen; continue; //continue with the following handling
				}else pvidcc->parseCommand(ptrBegin);

NextCMD:		//advance ptrBegin to the start of the next command data
				ptrBegin=ptrCmd+1; 
				while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //skip \r\n
			}//?while( (ptrCmd=
			//if there is an incomplete command, move it
			if((iret=(ptrBegin-buf))>0 && (buflen-iret)>0)
			{//if ptrBegin-buf==0, this is an error command data packet
				buflen-=iret;
				memmove((void *)buf,ptrBegin,buflen);
			} else buflen=0;
		}//?while...
		//delete vidccSession from the m_sessions queue
		m_mutex.lock();
		std::map<long,vidccSession *>::iterator it=m_sessions.find((long)pvidcc);
		if(it!=m_sessions.end()) m_sessions.erase(it);
		m_mutex.unlock(); 
		pvidcc->Destroy(); delete pvidcc; //Destroy will wait for all pipes to finish
	}//?if(strncmp(buf,"HELO ",5)==0)
	else if(strncmp(buf,"PIPE ",5)==0)
	{//this connection is a pipe connection from a vIDCc
		//create a pipe so that releasing psock after onConnect exits will not affect the forward thread
		socketTCP *pipe=new socketTCP(*psock);
		//set parent object to null so we can wait for pipe binding below
		if(pipe==NULL) return; else pipe->setParent(NULL);
		long vidccID=atol(buf+5);
		vidccSession *pvidcc=AddPipeFromVidcSession(pipe,vidccID);
		if(pvidcc==NULL){ delete pipe; return; }
		
		time_t t=time(NULL);
		while(psocketsvr->status()==SOCKS_LISTEN && 
		//	  pvidcc->isConnected() && //pvidcc may have already been released and deleted
			  pipe->status()==SOCKS_CONNECTED )
		{//if pipe binding succeeds, pipe's parent pointer points to the bound peer
			if(pipe->parent()!=NULL) return; //forward socket has already been bound
			//if not bound within the specified time, end
			else if((time(NULL)-t)>VIDC_PIPE_ALIVETIME) break; 
			cUtils::usleep(200000); //delay 200ms
		}//?while
		if(DelPipeFromVidcSession(pipe,vidccID)) delete pipe;
		//otherwise this pipe has been taken and occupied, do not delete
//		else{ pipe->Close(); pipe->setParent(NULL); }
	}//?else if(strncmp(ptrBegin,"PIPE ",5)==0)
	//yyc add 2007-08-21 support MakeHole command, used for TCP hole punching
	else if(strcmp(buf,"HOLE\r\n")==0)
	{
		//return the connect IP address and port
		iret=sprintf(buf,"200 %s:%d\r\n",psock->getRemoteIP(),psock->getRemotePort());
		iret=psock->Send(iret,buf,-1);
		//loop waiting for the other party to release the connection
		while(psocketsvr->status()==SOCKS_LISTEN && 
			  psock->status()==SOCKS_CONNECTED )
		{
			iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break;
			if(iret==0) continue;
			//if the other party gracefully closes the connection, checkSocket returns 1, but reading data via recv will return 0
			iret=psock->Receive(buf,VIDC_MAX_COMMAND_SIZE-1,-1);
			if(iret<0) break;
		}//?while
	}//?else if(strcmp(buf,"HOLE\r\n")==0)
	//a new connection comes in; initially only HELO and PIPE commands can be sent, other requests will close the connection
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
//a vIDC client has newly established a pipe
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


