/*******************************************************************
   *	IPRules.h
   *    DESCRIPTION: IP application rule filter class
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"  
#include "include/IPRules.h"
#include "utils/utils.h"

using namespace std;
using namespace net4cpp21;

bool iprules::check(unsigned long ip,unsigned short port,RULETYPE rt)
{
	std::vector<IPRule>::iterator it=m_rules.begin();
	for(;it!=m_rules.end();it++)
	{
		IPRule &iprule=*it;
		if( ( iprule.ruleType & rt )==0 ) continue;
		if( iprule.port_src!=0 && iprule.port_src!=port) continue;
		if( (iprule.IPMask_src & ip)==iprule.IPAddr_src )
		{
			return iprule.bEnabled;
		}
	}//?for(...
	return m_defaultEnabled;
}

bool iprules::check(unsigned long ip_fr,unsigned short port_fr,
					unsigned long ip_to,unsigned short port_to,
					RULETYPE rt)
{
	std::vector<IPRule>::iterator it=m_rules.begin();
	for(;it!=m_rules.end();it++)
	{
		IPRule &iprule=*it;
		if( ( iprule.ruleType & rt )==0 ) continue;
		if(iprule.bBidirection){
			if( ((iprule.port_src==0 || iprule.port_src==port_fr) && 
				(iprule.port_to==0 || iprule.port_to==port_to)) ||
				((iprule.port_src==0 || iprule.port_src==port_to) && 
				(iprule.port_to==0 || iprule.port_to==port_fr)) )
				NULL;
			else continue;
			if( ((iprule.IPMask_src & ip_fr)==iprule.IPAddr_src &&
				(iprule.IPMask_to & ip_to)==iprule.IPAddr_to )  ||
				((iprule.IPMask_src & ip_to)==iprule.IPAddr_src &&
				(iprule.IPMask_to & ip_fr)==iprule.IPAddr_to ) )
				return iprule.bEnabled;
			else continue;
		}else{
			if( (iprule.port_src==0 || iprule.port_src==port_fr) && 
				(iprule.port_to==0 || iprule.port_to==port_to) )
				NULL;
			else continue;
			if( (iprule.IPMask_src & ip_fr)==iprule.IPAddr_src &&
				(iprule.IPMask_to & ip_to)==iprule.IPAddr_to )
				return iprule.bEnabled;
			else continue;
		}
	}//?for(...
	return m_defaultEnabled;
}


//Add a single filter rule
//Format: [ip] [mask] [port] [type] [enabled]
bool iprules::addRule(const char *strRule)
{
	if(strRule==NULL || strRule[0]==0) return false;
	IPRule iprule; int icount=0;
	const char *ptrBegin=cUtils::strTrimLeft(strRule);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,' ');
		if(ptr) *(char *)ptr='\0';
		if(icount==0)
			iprule.IPAddr_src=inet_addr(ptrBegin);
		else if(icount==1)
			iprule.IPMask_src=inet_addr(ptrBegin);
		else if(icount==2)
			iprule.port_src=atoi(ptrBegin);
		else if(icount==3)
		{
			if(strcasecmp(ptrBegin,"RULETYPE_TCP")==0)
				iprule.ruleType=RULETYPE_TCP;
			else if(strcasecmp(ptrBegin,"RULETYPE_UDP")==0)
				iprule.ruleType=RULETYPE_UDP;
			else if(strcasecmp(ptrBegin,"RULETYPE_ICMP")==0)
				iprule.ruleType=RULETYPE_ICMP;
			else iprule.ruleType =RULETYPE_ALL;
		}
		else if(icount==4)
		{
			if(strcasecmp(ptrBegin,"true")==0)
				iprule.bEnabled=true;
			else if(strcasecmp(ptrBegin,"false")==0)
				iprule.bEnabled=false;
			else
				iprule.bEnabled=(atoi(ptrBegin)==0)?false:true;
		}
		if(ptr) *(char *)ptr=' ';
		if(++icount>4) break;
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
	if(iprule.IPAddr_src==INADDR_NONE) return false;
	m_rules.push_back(iprule);
	return true;
}
//Add multiple filter rules separated by commas
//Format: [ip] [mask] [port] [type] [enabled],...
long iprules::addRules(const char *strRules)
{
	m_rules.clear();
	if(strRules==NULL || strRules[0]==0) return 0;
	const char *ptrBegin=cUtils::strTrimLeft(strRules);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,',');
		if(ptr) *(char *)ptr='\0';
		addRule(ptrBegin);
		if(ptr) *(char *)ptr=',';
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
	return m_rules.size();
}

inline void splitIPstr(const char *str,unsigned long &ipaddr,unsigned long &ipmask,unsigned short &port)
{
	ipaddr=ipmask=0; port=0;
	if(str[0]==0) return;
	const char *ptr=strchr(str,':');
	if(ptr){ port=atoi(ptr+1); *(char *)ptr=0; }
	ipaddr=inet_addr(str);
	if(ptr) *(char *)ptr=':';
	ipmask=ipaddr;
	unsigned char *p1=(unsigned char *)&ipmask;
	for(int i=0;i<sizeof(ipmask);i++)
		if(*(p1+i)!=0) *(p1+i)=0xff;
	return;
}
//Add a single bidirectional filter rule
//Format: [ip:port]<->[ip:port] [type] [enabled]
//The mask is derived from the non-zero octets of the IP address, e.g., ip=192.168.1.0 yields mask=255.255.255
bool iprules::addRule_new(const char *strRule)
{

	if(strRule==NULL || strRule[0]==0) return false;
	IPRule iprule; int icount=0;
	const char *ptrBegin=cUtils::strTrimLeft(strRule);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,' ');
		if(ptr) *(char *)ptr='\0';
		
		if(icount==0)
		{
			const char *p0=strstr(ptrBegin,"<->");
			if(p0){ //Bidirectional filter rule
				*(char *)p0=0;
				splitIPstr(ptrBegin,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
				const char *p1=p0+3; while(*p1==' ') p1++;
				splitIPstr(p1,iprule.IPAddr_to,iprule.IPMask_to,iprule.port_to);
				iprule.bBidirection=true;
				*(char *)p0='<';
			}
			else if( (p0=strstr(ptrBegin,"->")) )
			{
				*(char *)p0=0;
				splitIPstr(ptrBegin,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
				const char *p1=p0+2; while(*p1==' ') p1++;
				splitIPstr(p1,iprule.IPAddr_to,iprule.IPMask_to,iprule.port_to);
				*(char *)p0='-';
			}
			else if( (p0=strstr(ptrBegin,"<-")) )
			{
				*(char *)p0=0;
				splitIPstr(ptrBegin,iprule.IPAddr_to,iprule.IPMask_to,iprule.port_to);
				const char *p1=p0+2; while(*p1==' ') p1++;
				splitIPstr(p1,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
				*(char *)p0='<';
			}
			else
				splitIPstr(ptrBegin,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
		}//?if(icount==0)
		else if(icount==1)
		{
			if(strcasecmp(ptrBegin,"RULETYPE_TCP")==0)
				iprule.ruleType=RULETYPE_TCP;
			else if(strcasecmp(ptrBegin,"RULETYPE_UDP")==0)
				iprule.ruleType=RULETYPE_UDP;
			else if(strcasecmp(ptrBegin,"RULETYPE_ICMP")==0)
				iprule.ruleType=RULETYPE_ICMP;
			else iprule.ruleType =RULETYPE_ALL;
		}
		else if(icount==2)
		{
			if(strcasecmp(ptrBegin,"true")==0)
				iprule.bEnabled=true;
			else if(strcasecmp(ptrBegin,"false")==0)
				iprule.bEnabled=false;
			else
				iprule.bEnabled=(atoi(ptrBegin)==0)?false:true;
		}

		if(ptr) *(char *)ptr=' ';
		if(++icount>2) break;
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
//	printf("aaaaaaaaaaaaaaaaaa\r\n");
//	printf("src-0x%x 0x%x:%d dst-0x%x 0x%x:%d \r\nbi=%d type=%d,b=%d\r\n",
//		iprule.IPMask_src,iprule.IPAddr_src,iprule.port_src ,
//		iprule.IPMask_to,iprule.IPAddr_to,iprule.port_to ,
//		iprule.bBidirection,iprule.ruleType,iprule.bEnabled);
	if(iprule.IPAddr_src==INADDR_NONE || iprule.IPAddr_to==INADDR_NONE) 
		return false;
	m_rules.push_back(iprule);
	return true;
}

//Add multiple bidirectional filter rules separated by commas
long iprules::addRules_new(const char *strRules)
{
	m_rules.clear();
	if(strRules==NULL || strRules[0]==0) return 0;
	const char *ptrBegin=cUtils::strTrimLeft(strRules);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,',');
		if(ptr) *(char *)ptr='\0';
		addRule_new(ptrBegin);
		if(ptr) *(char *)ptr=',';
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
	return m_rules.size();
}

//Resolve the specified domain name, only for IPV4
unsigned long Host2IP(const char *host)
{
	unsigned long ipAddr=inet_addr(host);
	if(ipAddr!=INADDR_NONE) return ipAddr;
	//The specified string is not a valid IP address; may be a hostname
	struct hostent * p=gethostbyname(host);
	if(p==NULL) return INADDR_NONE;
	ipAddr=(*((struct in_addr *)p->h_addr)).s_addr;
	return ipAddr;
}
//Alternative format for adding IP filter rules
//ipaccess: whether the IPs in strIP are accessible; 0=not accessible, 1=accessible
//strIP format: <ip>[:<port>],<ip>[:<port>],<ip>[:<port>]
//Example: 192.168.0.12,192.169.1.*,192.166.*.5 
//          * only matches one octet of the IP address. Notation like 192.168.1.1* is not supported
//		www.sina.com.cn,192.168.0.3
long iprules::addRules_new(RULETYPE rt,int ipaccess,const char *strIP)
{
	m_rules.clear();//Clear all filter rules
	m_defaultEnabled=true;
	if(strIP==NULL || strIP[0]==0) return 0;
	m_defaultEnabled=(ipaccess==0)?true:false;
	
	const char *ptrBegin=strIP; unsigned long iport=0;
	const char *p1,*p2,*p3,*p4,*p0=strchr(ptrBegin,',');
	while(true)
	{
		if(p0) *(char *)p0=0;
		while(*ptrBegin==' ') ptrBegin++; //Strip leading spaces
		p4=strchr(ptrBegin,':'); //Points to the port
		unsigned long iport=0;
		if(p4 && atoi(p4+1)>0 ) iport=atoi(p4+1);
		
		IPRule iprule; char ipr[64];
		//Check whether ptrBegin points to a hostname
		unsigned long IPAddr=Host2IP(ptrBegin);
		if(IPAddr!=INADDR_NONE) //Points to a domain name
		{
			iprule.IPAddr_src=IPAddr;
			iprule.IPMask_src=INADDR_NONE; //255.255.255.255	
		}else{//May point to an IP address
			p1=p2=p3=p4=NULL;
			p1=strchr(ptrBegin,'.');
			if(p1) p2=strchr(p1+1,'.');
			if(p2) p3=strchr(p2+1,'.');
			if(p1 && p2 && p3)
			{
				*(char *)p1=0; p1++;
				*(char *)p2=0; p2++;
				*(char *)p3=0; p3++;
			
				sprintf(ipr,"%s.%s.%s.%s",
					((ptrBegin[0]!='*')?ptrBegin:"0"),
					((p1[0]!='*')?p1:"0"),
					((p2[0]!='*')?p2:"0"),
					((p3[0]!='*')?p3:"0") );
				iprule.IPAddr_src=inet_addr(ipr);
				sprintf(ipr,"%d.%d.%d.%d",
					((ptrBegin[0]!='*')?255:0),
					((p1[0]!='*')?255:0),
					((p2[0]!='*')?255:0),
					((p3[0]!='*')?255:0) );
				iprule.IPMask_src=inet_addr(ipr);
				
				*(char *)(p1-1)='.';
				*(char *)(p2-1)='.';
				*(char *)(p3-1)='.';
			}
		}//?...else
		
		iprule.port_src=(unsigned short)iport;
		iprule.ruleType=rt;
		iprule.bEnabled=(ipaccess==0)?false:true;
		if(iprule.IPAddr_src!=INADDR_NONE) m_rules.push_back(iprule);

		if(p4) *(char *)p4=':';
		if(p0==NULL) break; else *(char *)p0=',';
		ptrBegin=p0+1;
		p0=strchr(ptrBegin,',');
	}//?while
	return m_rules.size();
}
/*
long iprules::addRules_new(RULETYPE rt,int ipaccess,const char *strIP)
{
	m_rules.clear();//Clear all filter rules
	m_defaultEnabled=true;
	if(strIP==NULL || strIP[0]==0) return 0;
	m_defaultEnabled=(ipaccess==0)?true:false;
	
	const char *ptrBegin=strIP;
	const char *p1,*p2,*p3,*p4,*p0=strchr(ptrBegin,',');
	while(true)
	{
		if(p0) *(char *)p0=0;
		p1=p2=p3=p4=NULL;
		p1=strchr(ptrBegin,'.');
		if(p1) p2=strchr(p1+1,'.');
		if(p2) p3=strchr(p2+1,'.');
		if(p3) p4=strchr(p3+1,':');
		if(p4) *(char *)p4++=0;
		if(p1 && p2 && p3)
		{
			*(char *)p1=0; p1++;
			*(char *)p2=0; p2++;
			*(char *)p3=0; p3++;
			
			IPRule iprule; char ipr[64];
			sprintf(ipr,"%s.%s.%s.%s",
				((ptrBegin[0]!='*')?ptrBegin:"0"),
				((p1[0]!='*')?p1:"0"),
				((p2[0]!='*')?p2:"0"),
				((p3[0]!='*')?p3:"0") );
			iprule.IPAddr_src=inet_addr(ipr);
			sprintf(ipr,"%d.%d.%d.%d",
				((ptrBegin[0]!='*')?255:0),
				((p1[0]!='*')?255:0),
				((p2[0]!='*')?255:0),
				((p3[0]!='*')?255:0) );
			iprule.IPMask_src=inet_addr(ipr);
			if(p4 && atoi(p4)>0)
				iprule.port_src=(unsigned short)atoi(p4);
			else iprule.port_src=0;
			iprule.ruleType=rt;
			iprule.bEnabled=(ipaccess==0)?false:true;
			if(iprule.IPAddr_src!=INADDR_NONE) m_rules.push_back(iprule);
			
			*(char *)(p1-1)='.';
			*(char *)(p2-1)='.';
			*(char *)(p3-1)='.';
			if(p4) *(char *)(p4-1)=':';
		}
		if(p0==NULL) break; else *(char *)p0=',';
		ptrBegin=p0+1;
		p0=strchr(ptrBegin,',');
	}//?while
	return m_rules.size();
}
*/