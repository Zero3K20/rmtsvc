/*******************************************************************
   *	telnetserver.h 
   *    DESCRIPTION: Telnet service
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *	
   *******************************************************************/

#include "rmtsvc.h" 
#include "shellCommandEx.h"
#include "other/Wutils.h"

class clsOutput_sock : public clsOutput
{
public:
	clsOutput_sock(socketTCP *psock):m_psock(psock){};
	virtual ~clsOutput_sock(){}
	int print(const char *buf,int len)
	{
		if(m_psock==NULL) return 0;
		return m_psock->Send(len,buf,-1);
	}
	socketBase *psocket(){ return m_psock; }
private:
	socketTCP *m_psock;
};

void cTelnetEx :: onCommand(const char *strCommand,socketTCP *psock)
{
	//check if output is redirected to file
	const char *ptr_outfile=strchr(strCommand,'>');
	if(ptr_outfile){ *(char *)ptr_outfile++=0;
		while(*ptr_outfile==' ') ptr_outfile++; //remove leading spaces
	}

	int cmdlen=strlen(strCommand); //remove trailing spaces
	while(cmdlen>0 && *(strCommand+cmdlen-1)==' ') cmdlen--;
	if(cmdlen<=0) return; else *((char *)strCommand+cmdlen)=0;
//----------  extended control commands start------------------------------
	BOOL bRet=FALSE; std::string strOutput;
	const char *strCmd=strCommand;
	const char *strParam=strchr(strCommand,' ');
	if(strParam) { *(char *)strParam=0; strParam++; }

	if(strcasecmp(strCmd,"update")==0 || strcasecmp(strCmd,"down")==0) //upgrade or download
	{
		if(strParam==NULL) return;
		bool bUpdate=(strcasecmp(strCmd,"update")==0);
		while(*strParam==' ') strParam++;
		int iType=0; const char *strurl=strParam;
		if(strncasecmp(strurl,"http://",7)==0) iType=1;
		else if(strncasecmp(strurl,"https://",8)==0) iType=1;
		else if(strncasecmp(strurl,"ftp://",6)==0) iType=2;
		std::string strSaveas;
		if(iType>0){//download the specified file
			clsOutput_sock sout(psock);
			const char *ptr=strchr(strurl,' ');
			if( ptr ){ *(char *)ptr=0; ptr+=1;
				while(*ptr==' ') ptr++; strSaveas.assign(ptr); 
			}
			if(strSaveas==""){ if( (ptr=strrchr(strurl,'/')) ) strSaveas.assign(ptr+1); }
			if(strSaveas[0]!='\\' && strSaveas[1]!=':') strSaveas.insert(0,g_savepath);
			if(bUpdate) strSaveas.append(".upd"); //防止download的fileand要升级的程序重名
			bRet=(iType==2)?downfile_ftp(strurl,strSaveas.c_str(),sout):downfile_http(strurl,strSaveas.c_str(),sout);
			strurl=(bRet)?strSaveas.c_str():NULL;
		}else if(!bUpdate) strOutput.append("Failed , wrong URLs.\r\n");
		//升级rmtsvc
		if(bUpdate)  bRet=updateRV(strurl,strOutput);
	}else if(strcasecmp(strCmd,"telnet")==0) //开启telnet
		bRet=FALSE;
	else //执行扩展command
		bRet=doCommandEx(strCmd,strParam,strOutput);
//----------  扩展控制command  end ------------------------------
	
	if(bRet)
	{
		FILE *fp=(ptr_outfile)?::fopen(ptr_outfile,"w"):NULL;
		if(fp){
			::fwrite(strOutput.c_str(),sizeof(char),strOutput.length(),fp);
			::fclose(fp);
			strOutput.assign("output >> "); strOutput.append(ptr_outfile);
		}
		strOutput.append("\r\n****Success to action****\r\n");
	}else strOutput.append("****Failed to action*****\r\n");
	if(strOutput!="") psock->Send(strOutput.length(),strOutput.c_str(),-1);
	return;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
telServerEx :: telServerEx()
{
	m_strSvrname.assign("Telnet Server");
	m_bindip="";
	m_svrport=0;
}
telServerEx :: ~telServerEx()
{
	 Stop(); m_threadpool.join();
}

//start service
bool telServerEx :: Start() 
{
	if(m_svrport==0) return true; //notstart service
	
	const char *ip=(m_bindip=="")?NULL:m_bindip.c_str();
	BOOL bReuseAddr=(ip)?SO_REUSEADDR:FALSE;//绑定了IP则允许port重用
	SOCKSRESULT sr=Listen( ((m_svrport<0)?0:m_svrport) ,bReuseAddr,ip);
	return (sr>0)?true:false;
}


void telServerEx ::revConnectionThread(socketTCP *psock)
{
	telServerEx *ptelsvr=(telServerEx *)psock->parent();
	MyService *pmysvr=MyService::GetService();
	socketBase *pevent=(pmysvr)?pmysvr->GetSockEvent():NULL;
	psock->setParent(pevent);
	ptelsvr->onConnect(psock);
	delete psock; return;
}
SOCKSRESULT telServerEx :: revConnect(const char *host,int port,time_t lWaitout)
{
	socketTCP *psock=new socketTCP;
	if(psock==NULL) return SOCKSERR_INVALID;
	else psock->setParent(this);
	SOCKSRESULT sr=psock->Connect(host,port,lWaitout);
	if(sr>0)
	if(m_threadpool.addTask((THREAD_CALLBACK *)&revConnectionThread,(void *)psock,THREADLIVETIME)==0)
		sr=SOCKSERR_THREAD;
	
	if(sr<=0) delete psock;
	return sr;
}

//settelnet service的相关info
//command format: 
//	telnet [port=<serviceport>] [bindip=<本service绑定的local machineIP>]  [account=<访问account:password>] 
//port=<serviceport>    : setserviceport，if not set则default为0.set为0则notstart web service <0则随即分配port
//bindip=<local machine IP for this service> : set the local machine IP to bind, default binds all IPs if not specified
//account=<访问account:password>
void telServerEx :: docmd_sets(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("port"))!=maps.end())
	{//set service port
		m_svrport=atoi((*it).second.c_str());
	}
	if( (it=maps.find("bindip"))!=maps.end())
	{//set service binding IP
		m_bindip=(*it).second;
	}
	
	if( (it=maps.find("account"))!=maps.end())
	{
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr){
			*(char *)ptr=0;
			setTelAccount((*it).second.c_str(),ptr+1);
			*(char *)ptr=':';
		}else setTelAccount(NULL,NULL); //无需account password
	}
	
	return;
}
//setservice的ip过滤规则or针对某个account的IP filter rules
//command format:
//	iprules [access=0|1] ipaddr="<IP>,<IP>,..."
//access=0|1     : whether to deny or allow IPs matching the following conditions
//例如:
// iprules access=0 ipaddr="192.168.0.*,192.168.1.10"
void telServerEx :: docmd_iprules(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	int ipaccess=1;
	if( (it=maps.find("access"))!=maps.end())
		ipaccess=atoi((*it).second.c_str());
	
	if( (it=maps.find("ipaddr"))!=maps.end())
	{
		std::string ipRules=(*it).second;
		this->rules().addRules_new(RULETYPE_TCP,ipaccess,ipRules.c_str());
	}else this->rules().addRules_new(RULETYPE_TCP,ipaccess,NULL);

	return;
}
