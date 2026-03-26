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
   *	communicate with the DNS server via UDP; the server listens on port 53 and returns the requested information.
   *******************************************************************/
#ifndef __YY_DNS_CLINET_H__
#define __YY_DNS_CLINET_H__

#include "dnsdef.h"
#include "socketUdp.h"

namespace net4cpp21
{
	
	//data structure of rdata in MX responses
	typedef struct dns_rdata_mx
	{
		unsigned short priority; //priority level
		std::string mxname; //parsed mail exchange server name
	}DNS_RDATA_MX,*PDNS_RDATA_MX;

	class dnsClient : public socketUdp
	{
	public:
		dnsClient();
		virtual ~dnsClient(){}
		void setTimeout(time_t s);
		//domain namequery domain name--->IP,returnDNS_RCODE_ERR_OK(0)success
		SOCKSRESULT Query(const char *names,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//reverse DNS lookup; returns DNS_RCODE_ERR_OK (0) on success
		SOCKSRESULT IQuery(const char *ip,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//mail exchanger query; returns DNS_RCODE_ERR_OK (0) on success
		//an MX record consists of a 2-byte priority value for the mail exchanger and a variable-length mail exchanger name
		SOCKSRESULT Query_MX(const char *names,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//get the DNS header info returned by the query
		PDNS_HEADER resp_dnsh() { return &m_dnsh; }
		//get the index-th DNS query info returned by the query
		PDNS_QUERY resp_dnsq(unsigned short index=0);

		//get the index-th result returned by the query
		PDNS_RESPONSE resp_dnsr(unsigned short index=0);
		//parse the rdata field data of DNS_RESPONSE
		unsigned long parse_rdata_Q(PDNS_RESPONSE pdnsr=NULL);
		const char * parse_rdata_IQ(PDNS_RESPONSE pdnsr=NULL);
		const char * parse_rdata_MX(PDNS_RDATA_MX pmx,PDNS_RESPONSE pdnsr=NULL);

	private:
		unsigned short m_msgID; //message ID
		time_t m_lTimeout;//maximum wait timeout return in seconds
		
		std::string m_strnames; //used to temporarily save parsed names
		char m_buffer[DNS_MAX_PACKAGE_SIZE];
		DNS_HEADER m_dnsh;
		DNS_QUERY m_dnsq;
		DNS_RESPONSE m_dnsr;
	};

}//?namespace net4cpp21

#endif
