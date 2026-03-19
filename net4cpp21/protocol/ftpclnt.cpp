/*******************************************************************
   *	ftpclnt.cpp
   *    DESCRIPTION:FTP protocol client implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-01-23
   *
   *	net4cpp 2.1
   *	File Transfer Protocol
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/ftpclnt.h"
#include "../include/cLogger.h"

#ifdef _SUPPORT_TLSCLIENT_
#include "../utils/OTP.h"
#endif

using namespace std;
using namespace net4cpp21;

#define RES_MAX_TIMEOUT 5 //5 seconds

void ftpClient::setTimeout(time_t s)
{
	m_lTimeout=s;
	if(m_lTimeout<1) m_lTimeout=RES_MAX_TIMEOUT;
	return;
}

//set the FTP service account
void ftpClient :: setFTPAuth(const char *strAccount,const char *strPwd)
{
	if(strAccount) m_strAccount.assign(strAccount);
	if(strPwd) m_strPwd.assign(strPwd);
	return;
}
//get data connection socket
SOCKSRESULT ftpClient::GetDatasock(const char *ftppath,socketTCP &datasock,bool bRetr)
{
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"TYPE I\r\n");
	sendCommand(200,buf,buflen,FTP_MAX_COMMAND_SIZE);
	int dataport=sendPASV(buf,FTP_MAX_COMMAND_SIZE);
	
	datasock.setParent(this->parent());
//	RW_LOG_DEBUG("[ftpclnt] Connect to %s:%d for data\r\n",buf,dataport,sr);
	if( datasock.Connect(buf,dataport)<=0 ) return SOCKSERR_FTP_DATACONN;
	buflen=sprintf(buf,"%s %s\r\n",((bRetr)?"RETR":"STOR"), ftppath);
	if(!sendCommand(150,buf,buflen,FTP_MAX_COMMAND_SIZE))
		return (bRetr)?SOCKSERR_FTP_RETR:SOCKSERR_FTP_STOR;
	return SOCKSERR_OK;
}
//****************************************
// function功能：download the specified file returns SOCKSERR_OK on success
// startPoint: start downloading from the specified position
// lens: number of bytes to download, ==-1 means download all
//****************************************
SOCKSRESULT ftpClient :: RetrFile(const char *ftppath,const char *savefile,long startPoint,long lsize)
{
	char buf[FTP_MAX_COMMAND_SIZE]; int buflen;
	if(this->status()!=SOCKS_CONNECTED) return SOCKSERR_CLOSED;
	if(startPoint>0 )
	{
		buflen=sprintf(buf,"REST %d\r\n",startPoint);
		if(!sendCommand(350,buf,buflen,FTP_MAX_COMMAND_SIZE))
			return SOCKSERR_FTP_REST; //RW_LOG_DEBUG(0,"this site does not support resume transfer\r\n");
	}//?if(startPoint>0 )
	//判断要download的filewhetherexists
	SOCKSRESULT sr=FileSize(ftppath);
	if(sr<=0) return SOCKSERR_FTP_NOEXIST;
	
	FILE *fp=(startPoint>0 || lsize>0)?fopen(savefile,"ab"):fopen(savefile,"wb");
	if(fp==NULL) return SOCKSERR_FTP_FILE;
	if(startPoint>0)
		::fseek(fp,startPoint,SEEK_SET);
	else ::fseek(fp,0,SEEK_SET);
	socketProxy datasock; datasock.setProxy(*this); //set代理
	sr=GetDatasock(ftppath,datasock,true);
	if( sr!=SOCKSERR_OK ){ ::fclose(fp); return sr; }

	char recvbuf[4096]; unsigned long received=0;
	if(lsize<=0) lsize=-1;
	while( received<(unsigned long)lsize ) 
	{
		int iret=datasock.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//read data sent by client
		iret=datasock.Receive(recvbuf,4096,-1);
		if(iret<0) break; else received+=iret;
		::fwrite(recvbuf,sizeof(char),iret,fp);
	}//?while
	::fclose(fp); datasock.Close();
	if(!sendCommand(226,buf,0,FTP_MAX_COMMAND_SIZE)) SOCKSERR_FTP_RETR;
	return SOCKSERR_OK;
}
//****************************************
// function功能：上载specified的file returns SOCKSERR_OK on success
// ftppath -- specifies the upload destination; if ==NULL, upload to current FTP position with the same filename
// startPoint: start uploading from the specified position
//****************************************
SOCKSRESULT ftpClient :: StorFile(const char *ftppath,const char *filename,long startPoint)
{
	char buf[FTP_MAX_COMMAND_SIZE]; int buflen;
	if(this->status()!=SOCKS_CONNECTED) return SOCKSERR_CLOSED;
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) SOCKSERR_FTP_FILE;
	if(startPoint>0 )
	{
		buflen=sprintf(buf,"REST %d\r\n",startPoint);
		if(!sendCommand(350,buf,buflen,FTP_MAX_COMMAND_SIZE))
			return SOCKSERR_FTP_REST; //RW_LOG_DEBUG(0,"this site does not support resume transfer\r\n");
		else ::fseek(fp,startPoint,SEEK_SET);
	}//?if(startPoint>0 )
	if(ftppath==NULL)
	{
		const char *ptr=strrchr(filename,'\\');
		if(ptr) ftppath=ptr+1; else ftppath=filename;
	}
	socketProxy datasock; datasock.setProxy(*this); //set代理
	SOCKSRESULT sr=GetDatasock(ftppath,datasock,false);
	if( sr!=SOCKSERR_OK ){ ::fclose(fp); return sr; }
	
	char readbuf[4096]; unsigned long received=0;
	while(this->status()==SOCKS_CONNECTED  )
	{
		int iret=::fread(readbuf,sizeof(char),4096,fp);
		if( datasock.Send(iret,readbuf,-1)<0 ) break;
		if(iret<4096) break; //file已读完
		if(datasock.checkSocket(0,SOCKS_OP_READ)<0) break;
	}//?while
	::fclose(fp); datasock.Close();
	if(!sendCommand(226,buf,0,FTP_MAX_COMMAND_SIZE)) SOCKSERR_FTP_STOR;
	return SOCKSERR_OK;
}

//****************************************
// function: get the specified file size, returns >=0 on success
//****************************************
SOCKSRESULT ftpClient :: FileSize(const char *ftppath)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"SIZE %s\r\n",ftppath);
	if(!sendCommand(213,buf,buflen,FTP_MAX_COMMAND_SIZE))
	{
		if(buf[0]==0)
			return SOCKSERR_FTP_RESP;
		else if(atoi(buf)==521) 
			return SOCKSERR_FTP_DENY;
		return SOCKSERR_FTP_NOEXIST;
	}
	return atol(buf+4);
}

//****************************************
// function功能：转到specified的directory returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient :: CWD(const char *ftppath)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"CWD %s\r\n",ftppath);
	if(!sendCommand(200,buf,buflen,FTP_MAX_COMMAND_SIZE))
	{
		if(buf[0]==0)
			return SOCKSERR_FTP_RESP;
		return SOCKSERR_FTP_NOEXIST;
	}
	return SOCKSERR_OK;
}

//****************************************
// function功能：createspecified的directory returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient :: MKD(const char *ftppath)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"MKD %s\r\n",ftppath);
	if(!sendCommand(257,buf,buflen,FTP_MAX_COMMAND_SIZE))
	{
		if(buf[0]==0)
			return SOCKSERR_FTP_RESP;
		else if(atoi(buf)==521)
			return SOCKSERR_FTP_EXIST;
		return SOCKSERR_FTP_FAILED;
	}
	return SOCKSERR_OK;
}

//****************************************
// function功能：deletespecified的directory returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient :: RMD(const char *ftppath)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"RMD %s\r\n",ftppath);
	if(!sendCommand(224,buf,buflen,FTP_MAX_COMMAND_SIZE))
	{
		if(buf[0]==0)
			return SOCKSERR_FTP_RESP;
		else if(atoi(buf)==521)
			return SOCKSERR_FTP_DENY;
		else if(atoi(buf)==550)
			return SOCKSERR_FTP_NOEXIST;
		return SOCKSERR_FTP_FAILED;
	}
	return SOCKSERR_OK;
}

//****************************************
// function功能：deletespecified的file returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient :: Delete(const char *ftppath)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"RMD %s\r\n",ftppath);
	if(!sendCommand(250,buf,buflen,FTP_MAX_COMMAND_SIZE))
	{
		if(buf[0]==0)
			return SOCKSERR_FTP_RESP;
		else if(atoi(buf)==521)
			return SOCKSERR_FTP_DENY;
		else if(atoi(buf)==550)
			return SOCKSERR_FTP_NOEXIST;
		return SOCKSERR_FTP_FAILED;
	}
	return SOCKSERR_OK;
}
//****************************************
// function功能：get file list returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient :: LIST(std::string &listbuf)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	return sendLIST(NULL,listbuf);
}

//****************************************
// function功能：connectftpserver returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient :: ConnectSvr(const char *ftpsvr,int ftpport)
{
	SOCKSRESULT sr=this->Connect(ftpsvr,ftpport);
	if(sr<0) return sr;
//		RW_LOG_DEBUG("[ftpclnt] Failed to connect FTP server(%s:%d),error=%d\r\n",
//			ftpsvr,ftpport,sr);

	//waiting to receive server response
	char buf[FTP_MAX_COMMAND_SIZE];
	if(!sendCommand(220,buf,0,FTP_MAX_COMMAND_SIZE)) return SOCKSERR_FTP_RESP;
	
	//FTPserver进行account authentication
	sr=Auth_LOGIN();
	if(sr!=SOCKSERR_OK) Close();
	return sr;
}

//****************************************
// function功能：connectftpserver returns SOCKSERR_OK on success
// ftpurl -- ftp://[account:password@]host[:port/....
//****************************************
SOCKSRESULT ftpClient :: ConnectSvr(const char *ftpurl)
{
	if(strncasecmp(ftpurl,"ftp://",6)==0)
		ftpurl+=6;
	//从ftpurl中parsedaccount passwordandhost
	const char *ptrUrlBegin=strchr(ftpurl,'/');
	if(ptrUrlBegin) *(char *)ptrUrlBegin=0;
	const char *ptemp,*ptr=strchr(ftpurl,'@');
	if(ptr)
	{
		*(char *)ptr=0; m_strPwd="";
		if( (ptemp=strchr(ftpurl,':')) )
		{
			m_strAccount.assign(ftpurl,ptemp-ftpurl);
			m_strPwd.assign(ptemp+1);
		} else m_strAccount.assign(ftpurl);
		*(char *)ptr='@'; ftpurl=ptr+1;
	}
	else
	{
		m_strAccount.assign("anonymous");
		m_strPwd.assign("ftpclnt");
	}
	std::string host; int iport=21;
	if( (ptr=strchr(ftpurl,':')) ){
		host.assign(ftpurl,ptr-ftpurl);
		iport=atoi(ptr+1);
	} else host.assign(ftpurl);
	if(ptrUrlBegin) *(char *)ptrUrlBegin='/';
	return ConnectSvr(host.c_str(),iport);
}

//****************************************
// function功能：serverauthentication returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient::Auth_LOGIN()
{
//	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
//		return SOCKSERR_CLOSED;
	
	char buf[FTP_MAX_COMMAND_SIZE]; 
	//send经useraccount
	int buflen=sprintf(buf,"USER %s\r\n",m_strAccount.c_str());
	if(!sendCommand(331,buf,buflen,FTP_MAX_COMMAND_SIZE))
		return SOCKSERR_FTP_RESP;
	//根据server的return判断yes普通passwordtransfer还yesMD4/MD5passwordencryptiontransfer
	const char *ptr=strstr(buf,"otp-");
	if(ptr==NULL)
	{//send经useraccount
		buflen=sprintf(buf,"PASS %s\r\n",m_strPwd.c_str());
	}
#ifdef _SUPPORT_TLSCLIENT_
	else if(strncmp(ptr+4,"md4 ",4)==0 || strncmp(ptr+4,"md5 ",4)==0)
	{
		OTP otps; bool bMD5=(strncmp(ptr+4,"md5 ",4)==0);
		int count=atoi(ptr+8);
		const char *seed=NULL;
		if( (seed=strchr(ptr+8,' ')) ) //find the starting position of the seed
		{
			seed++; //种子的起始position.
			//定bit种子endposition
			if( (ptr=strchr(seed,' ')) ) *(char *)ptr=0;
		}
		
		ptr=(bMD5)?otps.md5(seed,m_strPwd.c_str(),count):
				   otps.md4(seed,m_strPwd.c_str(),count);
		buflen=sprintf(buf,"PASS %s\r\n",ptr);
	}
	else return SOCKSERR_FTP_SUPPORT;//unsupported encryption transfer method
#else
	else return SOCKSERR_FTP_SUPPORT;//unsupported encryption transfer method
#endif
	
	if(!sendCommand(230,buf,buflen,FTP_MAX_COMMAND_SIZE))
	{
		if(buf[0]==0) return SOCKSERR_FTP_RESP;
		return SOCKSERR_FTP_AUTH; //authenticationfailure
	}
	return SOCKSERR_OK;
}

//****************************************
// function: send PASV to get data transfer IP and port, returns data port (>0) on success
//****************************************
SOCKSRESULT ftpClient::sendPASV(char *buf,int MAXBUFSIZE)
{
	int buflen=sprintf(buf,"PASV\r\n");
	if(!sendCommand(227,buf,buflen,MAXBUFSIZE))
		return SOCKSERR_FTP_FAILED;
	//get要connectdatatransfer主机andport
	int i=0,dataport=0; char *ptrIP;
	char *ptr=(char *)strchr(buf,'(');
	if(ptr){ 
		ptr++; ptrIP=ptr;
		while(*ptr)
		{
			if(*ptr==','){
				i++;
				if(i==4)
				{
					*ptr++=0;
					dataport=atoi(ptr);
				}
				else if(i>4)
				{
					dataport=dataport*256+atoi(++ptr);
					break;
				}
				else *ptr++='.';
			}//?if(*ptr==',')
			else ptr++;
		}//?while
	}//?if(ptr)
	if(ptrIP) ::memmove(buf,ptrIP,strlen(ptrIP)+1);
	return dataport;
}
//****************************************
// function功能：sendLISTget file list returns SOCKSERR_OK on success
//****************************************
SOCKSRESULT ftpClient::sendLIST(const char *listcmd,std::string &listbuf)
{
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"TYPE A\r\n");
	sendCommand(200,buf,buflen,FTP_MAX_COMMAND_SIZE);
	int dataport=sendPASV(buf,FTP_MAX_COMMAND_SIZE);
	listbuf.assign(buf); //temporarysavedatatransferIP
	socketProxy datasock; 
	datasock.setProxy(*this); //set代理
	
	SOCKSRESULT sr=datasock.Connect(listbuf.c_str(),dataport);
	if(sr<0) return SOCKSERR_FTP_DATACONN;
//		RW_LOG_DEBUG("[ftpclnt] Failed to connect(%s:%d) for data,error=%d\r\n",
//			listbuf.c_str(),dataport,sr);

	buflen=(listcmd)?sprintf(buf,"LIST %s\r\n",listcmd):sprintf(buf,"LIST\r\n");
					 
	if(!sendCommand(150,buf,buflen,FTP_MAX_COMMAND_SIZE))
		return SOCKSERR_FTP_LIST;
	
	datasock.setParent(this->parent());
	listbuf.assign("");//startgetdata
	while(datasock.status()==SOCKS_CONNECTED && 
		  this->status()==SOCKS_CONNECTED  )
	{
		int iret=datasock.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//read data sent by client
		iret=datasock.Receive(buf,FTP_MAX_COMMAND_SIZE-1,-1);
		if(iret<0) break; else buf[iret]=0;
		listbuf.append(buf);
	}//?while
	datasock.Close();
	if(!sendCommand(226,buf,0,FTP_MAX_COMMAND_SIZE)) SOCKSERR_FTP_LIST;
	return SOCKSERR_OK;
}


//send command and get server response
//[in] response_expected --- expected service response code
//[in] buf: send buffer, also used as receive response data buffer
//[in] buflen: size of data to send; maxbuflen specifies the size of the buf buffer
inline bool ftpClient :: sendCommand(int response_expected,char *buf,int buflen
									  ,int maxbuflen)
{
	if(buflen>0)
	{
//		RW_LOG_DEBUG("[ftpclnt] c--->s:\r\n\t%s",buf);
		if( this->Send(buflen,buf,-1)<=0 ){ buf[0]=0;  return false; }
	}
	//send success, waiting to receive server response
	SOCKSRESULT sr=this->Receive(buf,maxbuflen-1,m_lTimeout);
	if(sr<=0) {
//		RW_LOG_DEBUG(0,"[ftpclnt] failed to receive responsed message.\r\n");
		buf[0]=0; return false; 
	} else buf[sr]=0;
//	RW_LOG_DEBUG("[ftpclnt] s--->c:\r\n\t%s",buf);
	int responseCode=atoi(buf);

	//may have multiple response lines; if not the last line, the response header is "DDD- ...\r\n"
	//the last line of a multi-line response is "DDD ...\r\n"
	while(true)
	{
		bool bReceivedAll=(buf[sr-1]=='\n'); //possibly fully received
		if(bReceivedAll)
		{
			buf[sr-1]=0;
			const char *ptr=strrchr(buf,'\n');
			buf[sr-1]='\n';
			if(ptr==NULL) ptr=buf; else ptr++;
			if(atoi(ptr)==responseCode && ptr[3]!='-') break;
		}
		sr=this->Receive(buf,maxbuflen-1,m_lTimeout);
		if(sr<=0) break; else buf[sr]=0;
//		RW_LOG_DEBUG("%s",buf);
	}//?while
	return (response_expected==responseCode);
}

/*
//****************************************
// function功能：上载specified的file returns SOCKSERR_OK on success
// ftppath -- specifies the upload destination; if ==NULL, upload to current FTP position with the same filename
// startPoint: start uploading from the specified position
//****************************************
SOCKSRESULT ftpClient :: StorFile(const char *ftppath,const char *filename,long startPoint)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) SOCKSERR_FTP_FILE;
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=0;
	if(startPoint>0 )
	{
		buflen=sprintf(buf,"REST 0\r\n");
		if(!sendCommand(350,buf,buflen,FTP_MAX_COMMAND_SIZE))
		{
			RW_LOG_DEBUG(0,"this site does not support resume transfer\r\n");
			return SOCKSERR_FTP_REST;
		}
		::fseek(fp,startPoint,SEEK_SET);
	}//?if(startPoint>0 )
	if(ftppath==NULL)
	{
		const char *ptr=strrchr(filename,'\\');
		if(ptr) ftppath=ptr+1; else ftppath=filename;
	}
	
	SOCKSRESULT sr=sendSTOR(ftppath,fp);
	::fclose(fp); return sr;
}

//****************************************
// function功能：download the specified file returns SOCKSERR_OK on success
// startPoint: start downloading from the specified position
// lens: number of bytes to download, ==-1 means download all
//****************************************
SOCKSRESULT ftpClient :: RetrFile(const char *ftppath,const char *savefile,
								  long startPoint,long lens)
{
	if(this->status()!=SOCKS_CONNECTED) //必须firstcreateconnect
		return SOCKSERR_CLOSED;
	char buf[FTP_MAX_COMMAND_SIZE];
	int buflen=0;
	if(startPoint>0 )
	{
		buflen=sprintf(buf,"REST 0\r\n");
		if(!sendCommand(350,buf,buflen,FTP_MAX_COMMAND_SIZE))
		{
			RW_LOG_DEBUG(0,"this site does not support resume transfer\r\n");
			return SOCKSERR_FTP_REST;
		}
	}//?if(startPoint>0 )
	//判断要download的filewhetherexists
	std::string listbuf;
	SOCKSRESULT sr=sendLIST(ftppath,listbuf);
	if(sr!=SOCKSERR_OK) return sr;
	FILE *fp=NULL;
	if(startPoint>0)
	{
		fp=::fopen(savefile,"ab");
		if(fp) ::fseek(fp,startPoint,SEEK_SET);
	}
	else if(lens>0)
	{
		fp=::fopen(savefile,"ab");
		if(fp) ::fseek(fp,0,SEEK_SET);
	}
	else fp=::fopen(savefile,"wb");
	sr=(fp)?sendRETR(ftppath,fp,lens): SOCKSERR_FTP_FILE;
	if(fp) ::fclose(fp);
	return sr;
}
//****************************************
// function功能：download the specified file，并save。
//****************************************
SOCKSRESULT ftpClient::sendRETR(const char *retr,FILE *fp,long receiveBytes)
{
	char buf[4096];
	int buflen=sprintf(buf,"TYPE I\r\n");
	sendCommand(200,buf,buflen,FTP_MAX_COMMAND_SIZE);
	int dataport=sendPASV(buf,FTP_MAX_COMMAND_SIZE);
	std::string datahost(buf);
	socketProxy datasock; 
	datasock.setProxy(*this); //set代理

	SOCKSRESULT sr=datasock.Connect(datahost.c_str(),dataport);
	if(sr<0){
		RW_LOG_DEBUG("[ftpclnt] Failed to connect(%s:%d) for data,error=%d\r\n",
			datahost.c_str(),dataport,sr);
		return SOCKSERR_FTP_DATACONN;
	}

	buflen=sprintf(buf,"RETR %s\r\n",retr);
	if(!sendCommand(150,buf,buflen,FTP_MAX_COMMAND_SIZE))
		return SOCKSERR_FTP_RETR;
	
	long received=0; 
	datasock.setParent(this->parent());
	while(datasock.status()==SOCKS_CONNECTED && 
		  this->status()==SOCKS_CONNECTED ) 
	{
		int iret=datasock.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//read data sent by client
		iret=datasock.Receive(buf,4096,-1);
		if(iret<0) break;
		if(receiveBytes>0)
		{
			if( (received+iret)>=receiveBytes )
			{
				iret=receiveBytes-received;
				::fwrite(buf,sizeof(char),iret,fp);
				break;
			}
		}
		::fwrite(buf,sizeof(char),iret,fp);
	}//?while
	datasock.Close();
	if(!sendCommand(226,buf,0,FTP_MAX_COMMAND_SIZE)) SOCKSERR_FTP_RETR;
	return SOCKSERR_OK;
}
//****************************************
// function: upload the specified file to destfile.
//****************************************
SOCKSRESULT ftpClient::sendSTOR(const char *destfile,FILE *fp)
{
	char buf[4096];
	int buflen=sprintf(buf,"TYPE I\r\n");
	sendCommand(200,buf,buflen,FTP_MAX_COMMAND_SIZE);
	int dataport=sendPASV(buf,FTP_MAX_COMMAND_SIZE);
	std::string datahost(buf);
	socketProxy datasock; 
	datasock.setProxy(*this); //set代理
	
	SOCKSRESULT sr=datasock.Connect(datahost.c_str(),dataport);
	if(sr<0){
		RW_LOG_DEBUG("[ftpclnt] Failed to connect(%s:%d) for data,error=%d\r\n",
			datahost.c_str(),dataport,sr);
		return SOCKSERR_FTP_DATACONN;
	}

	buflen=sprintf(buf,"STOR %s\r\n",destfile);
	if(!sendCommand(150,buf,buflen,FTP_MAX_COMMAND_SIZE))
		return SOCKSERR_FTP_STOR;
	
	datasock.setParent(this->parent());
	while(datasock.status()==SOCKS_CONNECTED && 
		  this->status()==SOCKS_CONNECTED  )
	{
		int iret=::fread(buf,sizeof(char),4096,fp);
		if(iret>0)
			if( datasock.Send(iret,buf,-1)<0 ) break;
		if(iret<4096) break; //file已读完
		if(m_parent && m_parent->status()<=SOCKS_CLOSED) break;
	}//?while
	datasock.Close();
	if(!sendCommand(226,buf,0,FTP_MAX_COMMAND_SIZE)) SOCKSERR_FTP_RETR;
	return SOCKSERR_OK;
}
*/
