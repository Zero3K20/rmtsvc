/*******************************************************************
   *	proxyclnt.cpp
   *    DESCRIPTION:proxy protocol client implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	supports HTTP, SOCKS4, SOCKS5
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/proxyclnt.h"
#include "../include/cCoder.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;


socketProxy :: socketProxy()
{
	m_proxytype=PROXY_NONE;
	m_proxyport=0;
	m_dnsType=0;
}
socketProxy :: ~socketProxy()
{
	Close();
	m_thread.join();
}

bool socketProxy :: setProxy(PROXYTYPE proxytype,const char *proxyhost,
							 int proxyport,const char *user,const char *pwd)
{
	m_proxytype=proxytype;
	if(proxyhost) m_proxyhost.assign(proxyhost);
	m_proxyport=proxyport;
	if(user) m_proxyuser.assign(user);
	if(pwd) m_proxypwd.assign(pwd);
	return m_proxytype!=PROXY_NONE;
}

void socketProxy :: setProxy(socketProxy &socks)
{
	m_proxytype=socks.m_proxytype;
	m_proxyhost=socks.m_proxyhost;
	m_proxyport=socks.m_proxyport;
	m_proxyuser=socks.m_proxyuser;
	m_proxypwd=socks.m_proxypwd;
	return;
}

//connect to the specified TCP service. Returns local socket port (>0) on success, otherwise returns error
SOCKSRESULT socketProxy :: Connect(const char *host,int port,time_t lWaitout)
{
	if(host==NULL||host[0]==0||port<=0) return SOCKSERR_HOST;
	if(m_proxytype==PROXY_NONE) //directly connect to the specified host
		return socketTCP::Connect(host,port,lWaitout);
	//otherwise first connect to the proxy server
	SOCKSRESULT sr=socketTCP::Connect(m_proxyhost.c_str(),m_proxyport,lWaitout);
	if(sr<0){
		RW_LOG_DEBUG("Failed to connect proxy server(%s:%d),error=%d\r\n"
			,m_proxyhost.c_str(),m_proxyport,sr);
		return sr;//connection failed
	}
	SOCKSRESULT sr1=sendReq_Connect(host,port,lWaitout);
	if(sr1==SOCKSERR_OK) return sr;
	this->Close(); 
	return (sr1>0)?SOCKSERR_PROXY_REJECT:sr1;
}
//send SOCKS BIND request to the specified SOCKS service to open a listening port
//returns the opened port (>0) on success, otherwise failure
//[out] svrIP/svrPort returns the port and IP address opened by the SOCKS service
//[in] svrIP, tells the SOCKS service which source IP is allowed to connect to the opened port; svrPort is meaningless
SOCKSRESULT socketProxy :: Bind(std::string &svrIP,int &svrPort,time_t lWaitout)
{
	if(m_proxytype!=PROXY_SOCKS4 && m_proxytype!=PROXY_SOCKS5) 
		return SOCKSERR_NOTSURPPORT;
	//先connectproxy service器
	SOCKSRESULT sr=socketTCP::Connect(m_proxyhost.c_str(),m_proxyport,lWaitout);
	if(sr<0){
		RW_LOG_DEBUG("Failed to connect proxy server(%s:%d),error=%d\r\n"
			,m_proxyhost.c_str(),m_proxyport,sr);
		return sr;//connection failed
	}
	if( (sr=sendReq_Bind(svrIP,svrPort,lWaitout))!=SOCKSERR_OK || svrPort<=0)
	{
		this->Close();
		return (sr>0)?SOCKSERR_PROXY_REJECT:sr;
	}
	return svrPort;
}
//sendUDP代理协商request (UDP proxy is only supported by the SOCKS5 proxy protocol)
//successreturn开立的UDPport>0 otherwisefailure
//[in] svrIP/svrPort localUDPportandIP
//[out] svrIP/svrPort returns the port and IP address opened by the SOCKS service
SOCKSRESULT socketProxy :: UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout)
{
	if(m_proxytype!=PROXY_SOCKS5) return SOCKSERR_NOTSURPPORT;
	//先connectproxy service器
	SOCKSRESULT sr=socketTCP::Connect(m_proxyhost.c_str(),m_proxyport,lWaitout);
	if(sr<0){
		RW_LOG_DEBUG("Failed to connect proxy server(%s:%d),error=%d\r\n"
			,m_proxyhost.c_str(),m_proxyport,sr);
		return sr;//connection failed
	}
	if( (sr=sendReq_UdpAssociate(svrIP,svrPort,lWaitout))!=SOCKSERR_OK || svrPort<=0)
	{
		this->Close();
		return (sr>0)?SOCKSERR_PROXY_REJECT:sr;
	}
	return svrPort;
}

//send代理BINDrequest，returns SOCKSERR_OK on success(0)
//atSOCKS Server上开一个侦听service,svrIP/svrPortreturnsocksservice开的portandIPaddress
//[in] svrIP, tells the SOCKS service which source IP is allowed to connect to the opened port; svrPort is meaningless
SOCKSRESULT socketProxy :: sendReq_Bind(std::string &svrIP,int &svrPort,time_t lWaitout)
{
	SOCKSRESULT sr=SOCKSERR_OK;
	unsigned long ipAddr=INADDR_NONE;
	if(!m_ipv6) ipAddr=socketBase::Host2IP(svrIP.c_str());
	if(ipAddr==INADDR_NONE) return SOCKSERR_HOST;//必须yesvalid的IPv4address
	char buf[256]; int buflen=0;
	if(m_proxytype==PROXY_SOCKS4)
	{//填充sock4reqstructure
		buf[0]=0x04; buf[1]=0x02; //BIND
		unsigned short rp=htons((unsigned short)svrPort);
		::memcpy((void *)(buf+2),(const void *)&rp,sizeof(rp));
		buflen=2+sizeof(rp);
		::memcpy((void *)(buf+buflen),(const void *)&ipAddr,sizeof(ipAddr));
		buflen+=sizeof(ipAddr);
		buf[buflen++]=0;
	}
	else if(m_proxytype==PROXY_SOCKS5)
	{
		if( !socks5_Negotiation(lWaitout) ) return SOCKSERR_PROXY_AUTH;
		buf[0]=0x05; buf[1]=0x02; //填充sock5reqstructure BIND
		buf[2]=0x0; buf[3]=0x1; //specifiesyesipv4的address
		::memcpy((void *)(buf+4),(const void *)&ipAddr,sizeof(ipAddr));
		buflen=4+sizeof(ipAddr);
		unsigned short rp=htons((unsigned short)svrPort);
		::memcpy((void *)(buf+buflen),(const void *)&rp,sizeof(rp));
		buflen+=sizeof(rp);
	}
	else return SOCKSERR_NOTSURPPORT;
	if( (sr=this->Send(buflen,(const char *)buf,-1))<0) return sr;
	if( (sr=this->Receive(buf,256,lWaitout))<0 ) return sr;
	
	if(m_proxytype==PROXY_SOCKS4)
	{
		if(buf[1]==90) //==0x5A
		{
			sr=SOCKSERR_OK;
			//getservicereturn的开立的侦听portandIP
			svrPort=htons(*(unsigned short *)(buf+2));
			ipAddr=*(unsigned long *)(buf+2+sizeof(unsigned short));
			if(ipAddr==INADDR_ANY) ipAddr=this->m_remoteAddr.sin_addr.s_addr;
			svrIP.assign(socketBase::IP2A(ipAddr));
		}
		else sr=buf[1];
	}//?if(m_proxytype==PROXY_SOCKS4)
	else //m_proxytype==PROXY_SOCKS5
	{
		if(buf[1]==0x0) 
		{
			sr=SOCKSERR_OK;
			char atyp=buf[3];
			if(atyp!=0x01) return SOCKSERR_PROXY_ATYP;
			//getservicereturn的开立的侦听portandIP
			ipAddr=*(unsigned long *)(buf+4);
			if(ipAddr==INADDR_ANY) ipAddr=this->m_remoteAddr.sin_addr.s_addr;
			svrIP.assign(socketBase::IP2A(ipAddr));
			svrPort=htons( *(unsigned short *)(buf+4+sizeof(ipAddr)) );
		}
		else sr=buf[1];
	}
	return sr;
}
//sendsocks Bindrequest后waitingconnect到达(即第二个Bindresponse)
//bool BindedEventyesno采用onBindedeventnotification方式
bool socketProxy :: WaitForBinded(time_t lWaitout,bool BindedEvent)
{
//	if(BindedEvent) //startwaitingconnectthread
//		return m_thread.start((THREAD_CALLBACK *)&socketProxy::bindThread,(void *)this);
	
	bool bAccept=false;
	if(this->m_proxytype==PROXY_SOCKS4)
	{
		char buf[8];
		if( this->Receive(buf,8,lWaitout)==8 && buf[1]==0x5a ) 
			bAccept=true;
	}
	if(this->m_proxytype==PROXY_SOCKS5)
	{
		char buf[262];
		if( this->Receive(buf,5,lWaitout)==5 && buf[1]==0)
		{//先receive5byte判断addresstype
			int len=0;
			if(buf[3]=0x01) //ipv4
				len=10; //4+4+2
			else if(buf[3]==0x04) //ipv6
				len=22; //4+16+2
			else if(buf[3]==0x03) //全称域名
				len=7+(unsigned char)buf[4];//4+1+buf[4]+2;
			//实际还未receive完的byte
			if( (len-=5)>0 && this->Receive(buf+5,len,-1)>0)
				bAccept=true;
		}
	}
	return bAccept;
}

//sendsocks Bindrequest后start的waitingconnect到达的thread(即第二个Bindresponse)
//sendBindrequest后if有一个合法userconnect到socksservice上则产生onBindedevent
//此时可进行data交互，就好像user直接连到localservice的port上而notyessocksservice开的port上一样
//bAccept --- bindrequest的第二个responseyes拒绝还yes同意
//void socketProxy :: bindThread(socketProxy *psock)
//{
//	if(psock==NULL) return;
//	psock->onBinded(psock->WaitForBinded(-1,false));
//	return;
//}

//send proxy connect request，returns SOCKSERR_OK on success(0)
SOCKSRESULT socketProxy :: sendReq_Connect(const char *host,int port,time_t lWaitout)
{
	SOCKSRESULT sr=SOCKSERR_OK;
	char buf[256]; int buflen=0;
	if(m_proxytype==PROXY_SOCKS5)
	{
		unsigned long ipAddr=INADDR_NONE;
		if(!m_ipv6) ipAddr=socketBase::Host2IP(host);
		//if要求localparse
		if(ipAddr==INADDR_NONE && m_dnsType==1 ) return SOCKSERR_HOST;
		if( !socks5_Negotiation(lWaitout) ) return SOCKSERR_PROXY_AUTH;
		buf[0]=0x05; buf[1]=0x01;
		buf[2]=0x00; buflen=3;
		if(ipAddr==INADDR_NONE){
			buf[3]=0x03;
			buf[4]=sprintf(buf+5,"%s",host)+1; //containsNULLend符号
			buflen=5+buf[4]; buf[buflen-1]=0;
		}
		else{
			buf[3]=0x01;
			::memcpy((void *)(buf+4),(const void *)&ipAddr,sizeof(ipAddr));
			buflen=4+sizeof(ipAddr);
		}
		unsigned short rp=htons((unsigned short)port);
		::memcpy((void *)(buf+buflen),(const void *)&rp,sizeof(rp));
		buflen+=sizeof(rp);
	}//?if(m_proxytype==PROXY_SOCKS5)
	else if(m_proxytype==PROXY_HTTPS)
	{
		string strAuth;
		if(m_proxyuser!=""){//authenticationinfo
			strAuth=m_proxyuser+string(":")+m_proxypwd;
			if(cCoder::Base64EncodeSize(strAuth.length())>=256)
			{
				strAuth="";
				RW_LOG_PRINT(LOGLEVEL_ERROR,0,"proxy service的account无法base64 encoding，超过bufferlength\r\n");
			}
			else
			{
				buflen=cCoder::base64_encode((char *)strAuth.c_str(),strAuth.length(),buf);
				strAuth.assign("Proxy-Authorization: Basic ");
				strAuth.append(buf,buflen); strAuth.append("\r\n");
			}
		}//?if(m_proxyuser!=""){//authenticationinfo
		buflen=sprintf(buf,"CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n%sUser-Agent: yyproxy\r\n\r\n",
			host,port,host,port,strAuth.c_str());
	}//?else if(m_proxytype==PROXY_HTTPS)
	else //PROXY_SOCKS4
	{
		//sendsocks4 CONNECTrequest，requestconnectspecified的主机
		//socks4Aprotocol可以specifiedremoteconnect主机的域名而not必yesip
		//format如下：(原sock4protocolIPaddresspartial填入0.0.0.x),即buf[4]=buf[5]=buf[6]=0,buf[7]=x
		//[version] [command] [port] [0.0.0.1] [0] [hostdomain] [0]
		
		buf[0]=0x04; buf[1]=0x01; //填充sock4reqstructure
		unsigned short rp=htons((unsigned short)port);
		::memcpy((void *)(buf+2),(const void *)&rp,sizeof(rp));
		buflen=2+sizeof(rp);
		unsigned long ipAddr=INADDR_NONE;
		if(!m_ipv6) ipAddr=socketBase::Host2IP(host);
		if(ipAddr==INADDR_NONE) //也许resolve hostnameinvalid，交给server端parse
		{
			ipAddr=0x01000000;
			::memcpy((void *)(buf+buflen),(const void *)&ipAddr,sizeof(ipAddr));
			buflen+=sizeof(ipAddr); buf[buflen++]=0;
			buflen+=sprintf(buf+buflen,"%s",host);buf[buflen++]=0;
		}
		else
		{
			::memcpy((void *)(buf+buflen),(const void *)&ipAddr,sizeof(ipAddr));
			buflen+=sizeof(ipAddr); buf[buflen++]=0;
		}
	}//?else //PROXY_SOCKS4
	if( (sr=this->Send(buflen,(const char *)buf,-1))<0) return sr;
	if( (sr=this->Receive(buf,256,lWaitout))<0 ) return sr;
	
	if(m_proxytype==PROXY_SOCKS5)
		sr=buf[1]; //ifreturn success则buf[1]应为0
	else if(m_proxytype==PROXY_HTTPS)
	{
		if(strncmp(buf,"HTTP/1.",7)==0 && strncmp(buf+9,"200",3)==0) 
			sr=SOCKSERR_OK;
		else
			sr=SOCKSERR_PROXY_REJECT;
	}
	else //PROXY_SOCKS4
		sr=(buf[1]==90)?SOCKSERR_OK:buf[1];
	return sr;
}

//socks5协商authentication,successreturntrueotherwisereturnfalse
bool socketProxy :: socks5_Negotiation(time_t lWaitout)
{
	char buf[128];
	buf[0]=0x05; buf[1]=0x02;
	buf[2]=0x0; buf[3]=0x02;
	SOCKSRESULT sr=this->Send(4,(const char *)buf,-1);
	if(sr<0) return false;
	//receive2byte的responsedata
	sr=this->Receive(buf,2,lWaitout);
	if(sr!=2) return false;
	if(buf[1]==0x02)
	{//service方要求authentication
		int len=m_proxyuser.length()+m_proxypwd.length()+3;
		if(len>=128) return false; //超出了sendbuffersize
		buf[0]=0x01; buf[1]=(unsigned char)m_proxyuser.length();
		strncpy((buf+2),m_proxyuser.c_str(),m_proxyuser.length());
		
		buf[2+m_proxyuser.length()]=(unsigned char)m_proxypwd.length();//passwordlength
		strncpy( (buf+3+m_proxyuser.length()),m_proxypwd.c_str(),m_proxypwd.length() );
		if( (sr=this->Send(len,(const char *)buf,-1))<0) return false;
		//receive2byte的responsedata
		sr=this->Receive(buf,2,lWaitout);
		if(sr!=2) return false;
		if(buf[1]!=0) return false; //authenticationfailure
	}//?if(buf[1]==0x02)
	else if(buf[1]!=0) //not supported要求的authentication方式
	{
		RW_LOG_PRINT(LOGLEVEL_WARN,"not surpport Authentication (%d).\r\n",buf[1]);
		return false;
	} //buf[1]==0则not要求authentication
	return true;
}

//sendUDPproxy request，returns SOCKSERR_OK on success(0) (UDP proxy is only supported by the SOCKS5 proxy protocol)
//atSOCKS Server上开一个侦听service,svrIP/svrPortreturnsocksservice开的portandIPaddress
//[in] svrIP/svrPort localUDPportandIP
SOCKSRESULT socketProxy :: sendReq_UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout)
{
	SOCKSRESULT sr=SOCKSERR_OK;
//	if(m_proxytype!=PROXY_SOCKS5) return SOCKSERR_NOTSURPPORT;
	if( !socks5_Negotiation(lWaitout) ) return SOCKSERR_PROXY_AUTH;
	char buf[256]; int buflen=10;
	buf[0]=0x05; buf[1]=0x03; //填充sock5reqstructure BIND
	buf[2]=0x0; buf[3]=0x1; //specifiesyesipv4的address
	unsigned long ipAddr=INADDR_NONE;
	if(svrIP!="") ipAddr=socketBase::Host2IP(svrIP.c_str());
	if(ipAddr==INADDR_NONE) ipAddr=INADDR_ANY;
	*(unsigned long *)(buf+4)=ipAddr; //buf[4]=buf[5]=buf[6]=buf[7]=0;
	if(svrPort<0) svrPort=0;  //buf[8]=buf[9]=0;
	*(unsigned short *)(buf+8)=htons(svrPort);
	
	if( (sr=this->Send(buflen,(const char *)buf,-1))<0) return sr;
	if( (sr=this->Receive(buf,256,lWaitout))<0 ) return sr;
	
	if(buf[1]==0x0) 
	{
		sr=SOCKSERR_OK;
		char atyp=buf[3];
		if(atyp!=0x01) return SOCKSERR_PROXY_ATYP; //unsupported address type
		//getservicereturn的开立的侦听portandIP
		unsigned long ipAddr=*(unsigned long *)(buf+4);
		if(ipAddr==INADDR_ANY) ipAddr=this->m_remoteAddr.sin_addr.s_addr;
		svrIP.assign(socketBase::IP2A(ipAddr));
		svrPort=htons( *(unsigned short *)(buf+4+sizeof(ipAddr)) );
	}
	else sr=buf[1];
	return sr;
}

