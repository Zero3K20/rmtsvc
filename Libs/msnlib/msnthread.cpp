/*******************************************************************
   *	msnlib.cpp
   *    DESCRIPTION:MSN protocol handler class implementation
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2005-06-28
   *	
   *******************************************************************/

#include "../../include/sysconfig.h"
#include "../../include/cCoder.h"
#include "../../utils/utils.h"
#include "../../include/cLogger.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;

//handle command interaction between this client and the NS server
void msnMessager :: receiveThread(msnMessager *pmsnmessager) 
{
	if(pmsnmessager==NULL) return;
	RW_LOG_PRINT(LOGLEVEL_WARN,0,"main-thread of msnMessager has been started.\r\n");
	socketTCP *psock=(socketTCP *)&pmsnmessager->m_curAccount.m_chatSock;

	time_t tStart=time(NULL);
	char buffer[MSN_MAX_NS_COMMAND_SIZE]; //receive client commands
	int buflen=0; //clear command receive buffer
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0){
			if( (time(NULL)-tStart)>=20/*MSN_MAX_RESPTIMEOUT*/ ){ 
				psock->Send(5,"PNG\r\n",-1); 
				tStart=time(NULL); 
			}
			continue; //no data
		}//?if(iret==0)
		//read data sent by client
		iret=psock->Receive(buffer+buflen,MSN_MAX_NS_COMMAND_SIZE-buflen-1,-1);
		if(iret<0) break; //==0 means received data exceeded the limit
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buflen+=iret; buffer[buflen]=0;
		//parsemsncommand
		unsigned long trID=0; //NSreturnresponseID
		char *tmpptr,*ptrCmd,*ptrBegin=buffer;
		while( (ptrCmd=strchr(ptrBegin,'\r')) )
		{
			*(char *)ptrCmd=0;//startparsecommand
			if(ptrBegin[0]==0) goto NextCMD; //nothandlenull行data

//handleMSN NSservercommand ---- begin-------------------------------------------
			//注意:收到的commandyes先进行utf8编码then performmime编码的
//			RW_LOG_DEBUG("[msnlib] recevied Command from NS: %s.\r\n",ptrBegin);
			if( strncmp(ptrBegin,"MSG ",4)==0 || 
				strncmp(ptrBegin,"GCF ",4)==0 ||
				strncmp(ptrBegin,"UBX ",4)==0 )
			{//handle多行message,format范例
				//MSG Hotmail Hotmail 514  其中514指本message的后续待receive的byte数
				//GCF 9 Shields.xml 159 其中159指本message的后续待receive的byte数(为Shields.xmlfile内容)
				//UBX idazhi@hotmail.com 60 其中60指本message的后续待receive的byte数
				if( (tmpptr=strrchr(ptrBegin,' ')) )
				{//getmessagelength len
					int len=atoi(tmpptr+1); 
					//判断command的end符号yes\r还yes\r\n
					int nSkip=(*(ptrCmd+1)=='\n')?2:1;
					if( (buflen-(ptrCmd-buffer+nSkip))>=len) 
					{ //message已经全部receive完毕
						char c=*(ptrCmd+len+nSkip); 
						*(ptrCmd+len+nSkip)=0;//保护要handle的character串有end符号

						//handlemessage //+nSkipskip \r\n
						if(ptrBegin[0]=='M')
							pmsnmessager->nscmd_msg(psock,ptrCmd+nSkip);
						else if(ptrBegin[0]=='U')
						{
							*(char *)tmpptr=0; //getemailinfo
							pmsnmessager->nscmd_ubx(psock,ptrBegin+4,ptrCmd+nSkip);
						}
//						else if(ptrBegin[0]=='G')
//							pmsnmessager->nscmd_gcf(psock,ptrCmd+nSkip); //nothandleGCFmessage

						*(ptrCmd+len+nSkip)=c;
						//skip后续message,注意while循环会ptrBegin=ptrCmd+1;therefore要-1
						ptrCmd+=(len+nSkip-1);
					}
					else{ *ptrCmd='\r'; break; }//continue receiving remaining messages
				}//?if( (tmpptr=strrchr(ptrBegin,' ')) )
			}//?strncmp(ptrBegin,"MSG ",4)
			else if( atol(ptrBegin)!=0 ) 
			{//某个command的errorreturn format: error码 trID 说明\r\n
				if( (tmpptr=strchr(ptrBegin,' ')) ) 
					trID=(unsigned long)atol(tmpptr+1);
			}//?if( atol(ptrBegin)!=0 )
			else if(strncmp(ptrBegin,"SBS ",4)==0)
				pmsnmessager->nscmd_sbs(psock,ptrBegin);
			else if(strncmp(ptrBegin,"XFR ",4)==0)
				trID=(unsigned long)atol(ptrBegin+4);
			else if(strncmp(ptrBegin,"SYN ",4)==0)
				pmsnmessager->nscmd_syn(psock,ptrBegin);
			else if(strncmp(ptrBegin,"LSG ",4)==0)
				pmsnmessager->nscmd_lsg(psock,ptrBegin);
			else if(strncmp(ptrBegin,"LST ",4)==0)
				pmsnmessager->nscmd_lst(psock,ptrBegin);
			else if(strncmp(ptrBegin,"CHL ",4)==0)
				pmsnmessager->nscmd_chl(psock,ptrBegin);
			else if(strncmp(ptrBegin,"ILN ",4)==0)
				pmsnmessager->nscmd_iln(psock,ptrBegin);
			else if(strncmp(ptrBegin,"FLN ",4)==0)
				pmsnmessager->nscmd_fln(psock,ptrBegin);
			else if(strncmp(ptrBegin,"NLN ",4)==0)
				pmsnmessager->nscmd_nln(psock,ptrBegin);
			else if(strncmp(ptrBegin,"REM ",4)==0)
				trID=pmsnmessager->nscmd_rem(psock,ptrBegin);
			else if(strncmp(ptrBegin,"ADC ",4)==0)
				trID=pmsnmessager->nscmd_adc(psock,ptrBegin);
			else if(strncmp(ptrBegin,"RNG ",4)==0)
				trID=pmsnmessager->nscmd_rng(psock,ptrBegin);
			else if(strncmp(ptrBegin,"PRP ",4)==0)
				trID=pmsnmessager->nscmd_prp(psock,ptrBegin);
			else
				RW_LOG_DEBUG("[msnlib] recevied Command from NS: %s.\r\n",ptrBegin);

			if(trID!=0){
				pmsnmessager->m_mutex.lock();
				std::map<unsigned long,cCond *>::iterator it=pmsnmessager->m_conds.find(trID);
				if(it!=pmsnmessager->m_conds.end()){ 
					char *respbuf=(char *)((*it).second->getArgs());
					if(respbuf) strcpy(respbuf,ptrBegin);
					(*it).second->active(); 
				}
				pmsnmessager->m_mutex.unlock(); trID=0; 
			}//?if(trID!=0)
//handleMSN NSservercommand ----  end -------------------------------------------
			

NextCMD:	//移动ptrBegin到nextcommanddata起始
			ptrBegin=ptrCmd+1; 
			while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //skip \r\n
		}//?while
		//if有未receive完的command则移动
		if((iret=(ptrBegin-buffer))>0 && (buflen-iret)>0)
		{//ifptrBegin-buf==0说明这yes一个errorcommanddatapacket
			buflen-=iret;
			memmove((void *)buffer,ptrBegin,buflen);
		} else buflen=0;
	}//?while
	
	psock->Close(); pmsnmessager->onSIGNOUT();
	RW_LOG_PRINT(LOGLEVEL_WARN,0,"main-thread of msnMessager has been ended\r\n");
	return;

}

//handle本clientandSSserver的command交互
//每个sessionThreadyes一个chat session
void msnMessager :: sessionThread(void *param) 
{
	std::pair<msnMessager *,cContactor *> *pp=(std::pair<msnMessager *,cContactor *> *)param;
	msnMessager *pmsnmessager=pp->first;
	cContactor *pcon=pp->second; delete pp;
	if(pmsnmessager==NULL || pcon==NULL) return;
	srand(clock()); //保证此thread的的rand的随机，otherwise每个thread都会以default的种子产生随机数，导致每次
	//进入此thread产生的随机数相同 2005-07-18 yyc comment
	socketProxy *pchatsock=&pcon->m_chatSock;
	RW_LOG_PRINT(LOGLEVEL_WARN,"session-thread(%s) of msnMessager has been started.\r\n",
		pcon->m_email.c_str());
	pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_CREATE,
		pcon->m_email.c_str(),0);
	
	pcon->m_chat_contacts=0;//参与此次交谈的会话的联系人个数
	time_t tStart=time(NULL);//会话starttime
	time_t tTimeout=60;//10*MSN_MAX_RESPTIMEOUT;
	char buffer[MSN_MAX_SS_COMMAND_SIZE]; //receive client commands
	int buflen=0;//buffer中command的length
	while(pchatsock->status()==SOCKS_CONNECTED)
	{
		int iret=pchatsock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; //此socket发生error
		if(iret==0){
			if(pcon->m_chat_contacts>0 && pcon->m_vecMessage.size()>0) 
			{//yyc add 2006-03-13
				int iSize=pcon->m_vecMessage.size();
				for(int i=0;i<iSize;i++)
				{
					std::pair<char *,long> &p=pcon->m_vecMessage[i];
					pchatsock->Send(p.second,p.first,-1);
					delete[] p.first;
				}
				pcon->m_vecMessage.clear();
			}//yyc add 2006-03-13 end
			continue; //no data
		}
		//data has arrived,receivedata
		iret=pchatsock->Receive(buffer+buflen,MSN_MAX_SS_COMMAND_SIZE-buflen-1,-1);
		if(iret<0)  break;  //==0 means received data exceeded the limit
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buflen+=iret; buffer[buflen]=0;
		//handleMSN SSservercommand
		unsigned long trID=0; //SSreturnresponseID
		char *tmpptr,*ptrCmd,*ptrBegin=buffer;
//		RW_LOG_DEBUG("[msnlibXXXX] recevied Command from SS: %s.\r\n",ptrBegin);
		while( (ptrCmd=strchr(ptrBegin,'\r')) )
		{
			*(char *)ptrCmd=0;//startparsecommand
			if(ptrBegin[0]==0) goto NextCMD; //nothandlenull行data

//handleMSN SSservercommand ---- begin-------------------------------------------
//			RW_LOG_DEBUG("[msnlib] recevied Command from SS: %s.\r\n",ptrBegin);
			if( strncmp(ptrBegin,"MSG ",4)==0 )
			{//handle多行message,format范例
				//MSG yycnet@hotmail.com yyc:) 91  其中91指本message的后续待receive的byte数
				if( (tmpptr=strrchr(ptrBegin,' ')) )
				{//getmessagelength len
					int len=atoi(tmpptr+1); 
					//判断command的end符号yes\r还yes\r\n
					int nSkip=(*(ptrCmd+1)=='\n')?2:1;
					if( (buflen-(ptrCmd-buffer+nSkip))>=len) 
					{ //message已经全部receive完毕
						//让ptrBegin+4pointer toMSGmessage的send人Email，去掉后面的其他data
						if( (tmpptr=strchr(ptrBegin+4,' ')) ) *tmpptr=0;

						char c=*(ptrCmd+len+nSkip); 
						*(ptrCmd+len+nSkip)=0;//保护要handle的character串有end符号
						//handlemessage //+nSkipskip \r\n
						if(ptrBegin[0]=='M')
							pmsnmessager->sscmd_msg(pcon,ptrBegin+4,ptrCmd+nSkip,len); 

						*(ptrCmd+len+nSkip)=c;
						//skip后续message,注意while循环会ptrBegin=ptrCmd+1;therefore要-1
						ptrCmd+=(len+nSkip-1);
					}
					else{ *ptrCmd='\r'; break; }//continue receiving remaining messages
				}//?if( (tmpptr=strrchr(ptrBegin,' ')) )
			}//?strncmp(ptrBegin,"MSG ",4)
			else if( atol(ptrBegin)!=0 ) 
			{//某个command的errorreturn format: error码 trID 说明\r\n
				if( (tmpptr=strchr(ptrBegin,' ')) ) 
					trID=(unsigned long)atol(tmpptr+1);
			}//?if( atol(ptrBegin)!=0 )
			else if(strncmp(ptrBegin,"IRO ",4)==0)
			{//ifnotyes我发起的聊天，则其他人进入yes收到IRO
				//IRO <trID> 1 1 yycnet@hotmail.com yyc:)\r\n
				//parse出email
				char *ptrStart=NULL,*ptr=ptrBegin+4;
				int icount=0;
				while(*ptr){
					if(*ptr==' '){
						icount++;
						if(icount==3) 
							ptrStart=ptr+1;
						else if(icount==4){ *ptr=0; break; }
					}//?if(*ptr==' ')
					ptr++;
				}//?while
				if(ptrStart!=NULL)
					pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_JOIN,
							ptrStart,pcon->m_chat_contacts);
				pcon->m_chat_contacts++;
			}//?else if(strncmp(pcmdbuf,"IRO ",4)==0)
			else if(strncmp(ptrBegin,"JOI ",4)==0)
			{//ifyes我发起的聊天，则其他人进入yes收到JOI
				//JOI yycnet@hotmail.com yyc:)\r\n
				if( (tmpptr=strchr(ptrBegin+4,' ')) ) *tmpptr=0;
				pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_JOIN,
					ptrBegin+4,pcon->m_chat_contacts);
				pcon->m_chat_contacts++;
			}//?else if(strncmp(pcmdbuf,"JOI ",4)==0)
			else if(strncmp(ptrBegin,"BYE ",4)==0)
			{//某个userexit了聊天
				pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_BYE,
					ptrBegin+4,pcon->m_chat_contacts);
				if( --pcon->m_chat_contacts<=0 ) goto EXIT1;
			}//?else if(strncmp(pcmdbuf,"BYE ",4)==0)
			else if(strncmp(ptrBegin,"ACK ",4)==0)
			{ //ACK trID\r\n
				trID=(unsigned long)atol(ptrBegin+4);
			}
			else
				RW_LOG_DEBUG("[msnlib] recevied Command from SS: %s.\r\n",ptrBegin);

			if(trID!=0){
				pmsnmessager->m_mutex.lock();
				std::map<unsigned long,cCond *>::iterator it=pmsnmessager->m_conds.find(trID);
				if(it!=pmsnmessager->m_conds.end()){ 
					char *respbuf=(char *)((*it).second->getArgs());
					if(respbuf) strcpy(respbuf,ptrBegin);
					(*it).second->active(); 
				}
				pmsnmessager->m_mutex.unlock(); trID=0;
			}//?if(trID!=0)
//handleMSN SSservercommand ----  end -------------------------------------------
			

NextCMD:	//移动ptrBegin到nextcommanddata起始
			ptrBegin=ptrCmd+1; 
			while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //skip \r\n
		}//?while
		
		//check whether有未handle完的data
		if( (iret=(ptrBegin-buffer))> 0)
		{
			if((buflen-iret)>0) //有data未handle完
			{
				buflen-=iret;
				memmove((void *)buffer,ptrBegin,buflen);
			}
			else buflen=0; //data已全部handle完
		}
		//ifptrBegin-buf==0说明这yes一个未receive完的packet
/***********yyc remove 2006-09-01*********************************
	//if有未receive完的command则移动
		if((iret=(ptrBegin-buffer))>0 && (buflen-iret)>0)
		{//ifptrBegin-buf==0说明这yes一个errorcommanddatapacket 此句error见MSGhandle
			buflen-=iret;
			memmove((void *)buffer,ptrBegin,buflen);
		} else buflen=0;
*****************************************************************/
	}//?while
EXIT1:
	pchatsock->Send(5,"OUT\r\n",-1);
	pchatsock->Close(); pcon->m_shell.destroy();
	pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_DESTROY,NULL,0);
	if(pcon->m_vecMessage.size()>0) //yyc add 2006-03-13
	{
		std::vector<std::pair<char *,long> >::iterator it=pcon->m_vecMessage.begin();
		for(;it!=pcon->m_vecMessage.end();it++) delete[] (*it).first;
		pcon->m_vecMessage.clear();
	}
	pcon->m_chat_contacts=0;
	std::map<std::string,cMsncx *>::iterator it=pcon->m_msncxMaps.begin();
	for(;it!=pcon->m_msncxMaps.end();it++) delete (*it).second; 
	pcon->m_msncxMaps.clear(); pcon->m_msnc1Maps.clear();

	RW_LOG_PRINT(LOGLEVEL_WARN,"session-thread(%s) of msnMessager has been ended\r\n",
		pcon->m_email.c_str());
	return;
}
//andcmd shell交互的thread
//readcmd shell的输出并send导chatclient
void msnMessager :: shellThread(void *param)
{
	std::pair<msnMessager *,cContactor *> *pp=(std::pair<msnMessager *,cContactor *> *)param;
	msnMessager *pmsnmessager=pp->first;
	cContactor *pcon=pp->second; delete pp;
	if(pmsnmessager==NULL || pcon==NULL) return;
	RW_LOG_PRINT(LOGLEVEL_DEBUG,0,"shell-thread of msnMessager has been started.\r\n");
	

	char msgHeader[512]; //保留56byte的null间用于writeMSGcommand头
	int headerlen=pmsnmessager->encodeChatMsgHead(msgHeader+56,512-57,NULL,NULL);
	socketProxy *pchatsock=&pcon->m_chatSock;
	char Buf[2048]; int bufLen=0; 
	bool bSend=false;
	time_t tSend=time(NULL);
	while(pchatsock->status()==SOCKS_CONNECTED)
	{
		cUtils::usleep(SCHECKTIMEOUT); //休眠200ms
		int iret=pcon->m_shell.Read(Buf+bufLen,2048-bufLen-1);
		if(iret<0) break; 
		bufLen+=iret; Buf[bufLen]=0;
		if(iret==0){
			if((time(NULL)-tSend)>=MSN_MAX_RESPTIMEOUT)
				bSend=true; //防止chatSession没有交互被close
			else if(bufLen>0) bSend=true;//立即senddata
		}//?if(iret==0)
		else if(bufLen>=1500) bSend=true; //立即senddata
		if(bSend){
			if(bufLen<=0)
				pmsnmessager->sendcmd_SS_Typing(pcon,NULL);
			else if( !pmsnmessager->sendcmd_SS_chatMsg(pcon,msgHeader,headerlen,Buf,bufLen) ) 
				break;
			bufLen=0; bSend=false;
			tSend=time(NULL);
		}
	}//?while

	RW_LOG_PRINT(LOGLEVEL_DEBUG,0,"shell-thread of msnMessager has been ended\r\n");
	return;
}
