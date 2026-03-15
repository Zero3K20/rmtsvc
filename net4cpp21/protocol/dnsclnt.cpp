/*******************************************************************
   *	dnsclnt.cpp
   *    DESCRIPTION:DNS protocol client implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-02
   *
   *	net4cpp 2.1
   *	The Domain Name System (DNS) is a distributed database used by TCP/IP applications,
   *	providing translation between hostnames and IP addresses. Network users typically use
   *	过UDP协议和DNSserver进行通信，而server在特定的53port监听，并返回用户所需的相关info。
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/dnsclnt.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

#define RES_MAX_TIMEOUT 5 //5 seconds
dnsClient::dnsClient():
	m_msgID(0),m_lTimeout(RES_MAX_TIMEOUT)
{
		::memset((void *)&m_dnsh,0,sizeof(DNS_HEADER));
		::memset((void *)&m_dnsq,0,sizeof(DNS_QUERY));
		::memset((void *)&m_dnsr,0,sizeof(DNS_RESPONSE));
}

void dnsClient::setTimeout(time_t s)
{
	m_lTimeout=s;
	if(m_lTimeout<1) m_lTimeout=RES_MAX_TIMEOUT;
	return;
}

//编码DNS header
inline int encodeDNSH(char *buf,DNS_HEADER &dnsh)
{
	int len=sizeof(DNS_HEADER);
	PDNS_HEADER pdnsh=(PDNS_HEADER)buf;
	pdnsh->id=htons(dnsh.id);
	pdnsh->flags=htons(dnsh.flags);
	pdnsh->quests=htons(dnsh.quests);
	pdnsh->answers=htons(dnsh.answers);
	pdnsh->author=htons(dnsh.author);
	pdnsh->addition=htons(dnsh.addition);
	return len;
}

inline int decodeDNSH(const char *buf,DNS_HEADER &dnsh)
{
	int len=0;
	dnsh.id=ntohs(*(unsigned short *)(buf+len));
	len+=sizeof(unsigned short);
	dnsh.flags=ntohs(*(unsigned short *)(buf+len));
	len+=sizeof(unsigned short);
	dnsh.quests=ntohs(*(unsigned short *)(buf+len));
	len+=sizeof(unsigned short);
	dnsh.answers=ntohs(*(unsigned short *)(buf+len));
	len+=sizeof(unsigned short);
	dnsh.author=ntohs(*(unsigned short *)(buf+len));
	len+=sizeof(unsigned short);
	dnsh.addition=ntohs(*(unsigned short *)(buf+len));
	len+=sizeof(unsigned short);
	return len;
}
//返回dnsname组合的size，包括最后的'\0'
inline int LenOfDnsStr(unsigned char *buf)
{
//	if(buf==NULL) return 0;
	unsigned char *ptr=buf;
	while( ptr[0]>0 )
	{
		if(ptr[0]==0xc0)
		{	ptr++; break; }
		else
			ptr+=(ptr[0]+1);
	}//?while
	ptr++; //跳过最后一个'\0'或0xc0后面的Position字节
	return (ptr-buf);
}
//将域名/IPstring conversion为dns支持的name组合形式
int cvtString2DnsStr(unsigned char *buf,const char *names)
{
	if(names==NULL) { buf[0]=0; return 1; }
	int len=0;
	const char *ptr=names;
	while( (ptr=strchr(names,'.')) )
	{
		buf[len++]=ptr-names;
		::memcpy((void *)(buf+len),names,ptr-names);
		len+=(ptr-names);
		names=ptr+1;
	}//?while
	if( (buf[len++]=(unsigned char)strlen(names))>0 ){
		len+=sprintf((char *)buf+len,"%s",names);
		buf[len++]=0;
	}
	return len;
}
//将dns支持的name组合形式 转换为 域名/IP字符串
//packageStart --- 整个DNS包的起始位置，因为0xC0指向的偏移位置是相对整个包的
inline int cvtDnsStr2String(unsigned char *buf,std::string &names,
							unsigned char *packageStart)
{
	//	if(buf==NULL) return 0;
	unsigned char *ptr=buf;
	while( ptr[0]>0 )
	{
		if(ptr[0]==0xc0)
		{
			cvtDnsStr2String(packageStart+ptr[1],names,packageStart);
			ptr+=2; break;
		}
		else
		{
			if(names=="")
				names.assign((char *)ptr+1,ptr[0]);
			else
			{
				names.append(".");
				names.append((char *)ptr+1,ptr[0]);
			}
			ptr+=(ptr[0]+1);
		}//?
	}//?while
	return names.length();
}

//编码DNS协议query域
inline int encodeDNSQ(unsigned char *buf,DNS_QUERY &dnsq)
{
	int len=cvtString2DnsStr(buf,dnsq.name);

	*(unsigned short *)(buf+len)=htons(dnsq.type);
	len+=sizeof(unsigned short);
	*(unsigned short *)(buf+len)=htons(dnsq.classes);
	len+=sizeof(unsigned short);
	return len;
}

//域名query 域名--->IP,返回DNS_RCODE_ERR_OK(0)success
//否则发生error
SOCKSRESULT dnsClient :: Query(const char *names,const char *dnssvr,int dnsport)
{
	if(this->status()!=SOCKS_OPENED) return SOCKSERR_INVALID;
	unsigned long ipAddr=inet_addr(dnssvr);
	if(ipAddr==INADDR_NONE || dnsport<=0 ) return SOCKSERR_DNS_SERVER;

	if(names==NULL || names[0]==0) SOCKSERR_DNS_NAMES;
	::memset((void *)&m_dnsh,0,sizeof(DNS_HEADER));
	::memset((void *)&m_dnsq,0,sizeof(DNS_QUERY));
	::memset((void *)&m_dnsr,0,sizeof(DNS_RESPONSE));

	//组织要send的querydata
	char *buffer=m_buffer;
	m_dnsh.id=++m_msgID;
	m_dnsh.flags=DNS_OPCODE_QUERY;
	m_dnsh.quests=0x01;
	int buflen=encodeDNSH(buffer,m_dnsh);
	//填充query结构data
	m_dnsq.name=names;
	m_dnsq.type=T_A; m_dnsq.classes=C_IN;
	buflen+=encodeDNSQ((unsigned char *)buffer+buflen,m_dnsq);
	//设置send目的info
	this->setRemoteInfo(dnssvr,dnsport);
	//sendDNS querydata包
	SOCKSRESULT sr=Send(buflen,buffer,-1);
	if(sr<0) return sr;
	if(sr==0) return SOCKSERR_ZEROLEN;
	sr=Receive(buffer,DNS_MAX_PACKAGE_SIZE,m_lTimeout);
	if(sr<0) return sr;
	if(sr==0) return SOCKSERR_ZEROLEN;
	RW_LOG_PRINTBINARY(buffer,sr);
	//parse返回的data
	decodeDNSH(buffer,m_dnsh);
	return (m_dnsh.flags & DNS_FLAGS_OPCODE);
}
//反向域名parse
SOCKSRESULT dnsClient :: IQuery(const char *ip,const char *dnssvr,int dnsport)
{
	if(this->status()!=SOCKS_OPENED) return SOCKSERR_INVALID;
	unsigned long ipAddr=inet_addr(dnssvr);
	if(ipAddr==INADDR_NONE || dnsport<=0 ) return SOCKSERR_DNS_SERVER;

	if(ip==NULL || ip[0]==0 || inet_addr(ip)==INADDR_NONE) return SOCKSERR_DNS_IP;
	::memset((void *)&m_dnsh,0,sizeof(DNS_HEADER));
	::memset((void *)&m_dnsq,0,sizeof(DNS_QUERY));
	::memset((void *)&m_dnsr,0,sizeof(DNS_RESPONSE));

	//组织要send的querydata
	char *buffer=m_buffer;
	m_dnsh.id=++m_msgID;
	m_dnsh.flags=DNS_OPCODE_IQUERY;
	m_dnsh.quests=0x01;
	int buflen=encodeDNSH(buffer,m_dnsh);
	//填充query结构data
	m_dnsq.name=ip;
	m_dnsq.type=T_A; m_dnsq.classes=C_IN;
	buflen+=encodeDNSQ((unsigned char *)buffer+buflen,m_dnsq);
	//设置send目的info
	this->setRemoteInfo(dnssvr,dnsport);
	//sendDNS querydata包
	SOCKSRESULT sr=Send(buflen,buffer,-1);
	if(sr<0) return sr;
	if(sr==0) return SOCKSERR_ZEROLEN;
	sr=Receive(buffer,DNS_MAX_PACKAGE_SIZE,m_lTimeout);
	if(sr<0) return sr;
	if(sr==0) return SOCKSERR_ZEROLEN;
	RW_LOG_PRINTBINARY(buffer,sr);
	//parse返回的data
	decodeDNSH(buffer,m_dnsh);
	return (m_dnsh.flags & DNS_FLAGS_OPCODE);

}

//邮件交换器query
SOCKSRESULT dnsClient :: Query_MX(const char *names,const char *dnssvr,int dnsport)
{
	if(this->status()!=SOCKS_OPENED) return SOCKSERR_INVALID;
	unsigned long ipAddr=inet_addr(dnssvr);
	if(ipAddr==INADDR_NONE || dnsport<=0 ) return SOCKSERR_DNS_SERVER;

	if(names==NULL || names[0]==0) SOCKSERR_DNS_NAMES;
	::memset((void *)&m_dnsh,0,sizeof(DNS_HEADER));
	::memset((void *)&m_dnsq,0,sizeof(DNS_QUERY));
	::memset((void *)&m_dnsr,0,sizeof(DNS_RESPONSE));

	//组织要send的querydata
	char *buffer=m_buffer;
	m_dnsh.id=++m_msgID;
	m_dnsh.flags=DNS_OPCODE_QUERY;
	m_dnsh.quests=0x01;
	int buflen=encodeDNSH(buffer,m_dnsh);
	//填充query结构data
	m_dnsq.name=names;
	m_dnsq.type=T_MX; m_dnsq.classes=C_IN;
	buflen+=encodeDNSQ((unsigned char *)buffer+buflen,m_dnsq);
	//设置send目的info
	this->setRemoteInfo(dnssvr,dnsport);
	//sendDNS querydata包
	SOCKSRESULT sr=this->Send(buflen,buffer,-1);
	if(sr<0) return sr;
	if(sr==0) return SOCKSERR_ZEROLEN;
	sr=this->Receive(buffer,DNS_MAX_PACKAGE_SIZE,m_lTimeout);
	if(sr<0) return sr;
	if(sr==0) return SOCKSERR_ZEROLEN;
//	RW_LOG_PRINTBINARY(buffer,sr);
	//parse返回的data
	decodeDNSH(buffer,m_dnsh);
	return (m_dnsh.flags & DNS_FLAGS_OPCODE);
}


//获取query返回的dns 第index个queryinfo
PDNS_QUERY dnsClient :: resp_dnsq(unsigned short index)
{
	if(index>=m_dnsh.quests) return NULL;//下标超界
	//定位返回info中的第一个query条件记录
	unsigned char *buffer=(unsigned char *)m_buffer+sizeof(DNS_HEADER);
	for(int i=0;i<index;i++)
	{
		buffer+=LenOfDnsStr(buffer);
		buffer+=2*sizeof(unsigned short); //加上type和classes的length
	}//?for(int i=0;i<index;i++)
	//此时buffer定位到要获取的第index个query结构起始位置
	
	m_strnames="";
	cvtDnsStr2String(buffer,m_strnames,(unsigned char *)m_buffer);
	buffer+=LenOfDnsStr(buffer);
	//此时buffer指向DNS_QUERY.type
	m_dnsq.type=ntohs(*(unsigned short *)buffer);
	buffer+=sizeof(unsigned short);
	m_dnsq.classes=ntohs(*(unsigned short *)buffer);
	m_dnsq.name=m_strnames.c_str();
	return &m_dnsq;
}

//获取query返回的第index结果
PDNS_RESPONSE dnsClient :: resp_dnsr(unsigned short index)
{
	if(index>=m_dnsh.answers) return NULL;//下标超界

	//定位返回info中的第一个回复记录的起始位置
	unsigned char *buffer=(unsigned char *)m_buffer+sizeof(DNS_HEADER);
	for(int i=0;i<m_dnsh.quests;i++)
	{
		buffer+=LenOfDnsStr(buffer);
		buffer+=2*sizeof(unsigned short); //加上type和classes的length
	}//?for(int i=0;i<index;i++)
	//此时buffer定位到第一个回复记录的起始位置
	//定位第index记录的起始位置
	for(int j=0;j<index;j++)
	{
		buffer+=LenOfDnsStr(buffer);
		buffer+=2*sizeof(unsigned short); //加上type和classes的length
		buffer+=sizeof(unsigned long);
		unsigned short length=ntohs(*(unsigned short *)buffer);
		buffer+=(sizeof(unsigned short)+length);
	}//?for(int i=0;i<index;i++)
	//此时buffer定位到要获取的第index个回复记录起始位置

	m_strnames="";
	cvtDnsStr2String(buffer,m_strnames,(unsigned char *)m_buffer);
	buffer+=LenOfDnsStr(buffer);
	m_dnsr.type=ntohs(*(unsigned short *)buffer);
	buffer+=sizeof(unsigned short);
	m_dnsr.classes=ntohs(*(unsigned short *)buffer);
	buffer+=sizeof(unsigned short);
	m_dnsr.ttl=ntohl(*(unsigned long *)buffer);
	buffer+=sizeof(unsigned long);
	m_dnsr.length=ntohs(*(unsigned short *)buffer);
	buffer+=sizeof(unsigned short);
	
	m_dnsr.rdata=(unsigned char *)buffer;
	m_dnsr.name=m_strnames.c_str();
	return &m_dnsr;
}

//parseDNS_RESPONSE的rdata域的data
unsigned long dnsClient :: parse_rdata_Q(PDNS_RESPONSE pdnsr)
{
	if(pdnsr==NULL) pdnsr=&m_dnsr;
	if(pdnsr->rdata==NULL) return INADDR_NONE;
	unsigned long ipAddr=*(unsigned long *)pdnsr->rdata;
	return ipAddr;
}
const char * dnsClient :: parse_rdata_IQ(PDNS_RESPONSE pdnsr)
{
	if(pdnsr==NULL) pdnsr=&m_dnsr;
	unsigned char *prdata=pdnsr->rdata;
	if(prdata==NULL) return NULL;
	m_strnames="";
	cvtDnsStr2String(prdata,m_strnames,(unsigned char *)m_buffer);
	return m_strnames.c_str();
}

const char * dnsClient :: parse_rdata_MX(PDNS_RDATA_MX pmx,PDNS_RESPONSE pdnsr)
{
	if(pdnsr==NULL) pdnsr=&m_dnsr;
	unsigned char *prdata=pdnsr->rdata;
	if(prdata==NULL) return NULL;
	
	if(pmx) pmx->priority=ntohs(*(unsigned short *)prdata);
	prdata+=sizeof(unsigned short);
	m_strnames="";
	cvtDnsStr2String(prdata,m_strnames,(unsigned char *)m_buffer);
	if(pmx) pmx->mxname=m_strnames;
	return m_strnames.c_str();
}
