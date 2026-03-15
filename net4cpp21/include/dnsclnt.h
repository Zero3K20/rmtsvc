/*******************************************************************
   *	dnsclnt.h
   *    DESCRIPTION:DNS protocol client declaration
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
#ifndef __YY_DNS_CLINET_H__
#define __YY_DNS_CLINET_H__

#include "dnsdef.h"
#include "socketUdp.h"

namespace net4cpp21
{
	
	//MX的response中rdata的data结构
	typedef struct dns_rdata_mx
	{
		unsigned short priority; //优先级别
		std::string mxname; //parse的邮件交换servername
	}DNS_RDATA_MX,*PDNS_RDATA_MX;

	class dnsClient : public socketUdp
	{
	public:
		dnsClient();
		virtual ~dnsClient(){}
		void setTimeout(time_t s);
		//域名query 域名--->IP,返回DNS_RCODE_ERR_OK(0)success
		SOCKSRESULT Query(const char *names,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//反向域名parse,返回DNS_RCODE_ERR_OK(0)success
		SOCKSRESULT IQuery(const char *ip,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//邮件交换器query,返回DNS_RCODE_ERR_OK(0)success
		//一个MX记录是由一个2字节的指示该邮件交换器的优先级值及不定长的邮件交换器名组成的
		SOCKSRESULT Query_MX(const char *names,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//获取query返回的dns头info
		PDNS_HEADER resp_dnsh() { return &m_dnsh; }
		//获取query返回的dns 第index个queryinfo
		PDNS_QUERY resp_dnsq(unsigned short index=0);

		//获取query返回的第index结果
		PDNS_RESPONSE resp_dnsr(unsigned short index=0);
		//parseDNS_RESPONSE的rdata域的data
		unsigned long parse_rdata_Q(PDNS_RESPONSE pdnsr=NULL);
		const char * parse_rdata_IQ(PDNS_RESPONSE pdnsr=NULL);
		const char * parse_rdata_MX(PDNS_RDATA_MX pmx,PDNS_RESPONSE pdnsr=NULL);

	private:
		unsigned short m_msgID; //消息ID
		time_t m_lTimeout;//最大等待timeout返回s
		
		std::string m_strnames; //用来临时保存parse的names
		char m_buffer[DNS_MAX_PACKAGE_SIZE];
		DNS_HEADER m_dnsh;
		DNS_QUERY m_dnsq;
		DNS_RESPONSE m_dnsr;
	};

}//?namespace net4cpp21

#endif
