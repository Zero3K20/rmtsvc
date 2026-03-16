/*******************************************************************
   *	dnsdef.h
   *    DESCRIPTION:constants, structures and enum definitions for the DNS protocol
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-02
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_DNSPDEF_H__
#define __YY_DNSPDEF_H__

#define SOCKSERR_DNS_SERVER -101 //invalid IP address
#define SOCKSERR_DNS_IP SOCKSERR_DNS_SERVER-1 //invalid DNS server address
#define SOCKSERR_DNS_NAMES SOCKSERR_DNS_SERVER-2 //invalid domain name to parse

#define DNS_SERVER_PORT	53 //default DNS service port
#define DNS_MAX_PACKAGE_SIZE 512 //size of a single DNS query packet

//----------------constants here define the meaning of DNS_HEADER.flags bit masks for LITTLE-ENDIAN systems-----
#define DNS_FLAGS_QR 0x80 //bit 7 of the first network byte
		//0代表query，1代表DNS回复
#define DNS_FLAGS_OPCODE 0x78 //bits 6,5,4,3 of the first network byte
		//指示query种class：0:standard query；1:reverse query；2:serverstatusquery；3-15:未使用。
#define DNS_FLAGS_AA 0x04 //bit 2 of the first network byte
		//whether权威回复
#define DNS_FLAGS_TC 0x02 //bit 1 of the first network byte
		//因为一个UDP报文为512byte，所以该bit指示whether截掉超过的partial
#define DNS_FLAGS_RD 0x01 //bit 0 of the first network byte
		//此bitatquery中specified，回复时相同。set为1指示server进行递归query
#define DNS_FLAGS_RA 0x8000 //bit 7 of the second network byte
		//由DNS回复returnspecified，说明DNSserverwhether支持递归query
#define DNS_FLAGS_Z 0x7000 //bits 6,5,4 of the second network byte
		//保留字段，必须set为0
#define DNS_FLAGS_RCODE 0x0f00 //bits 3,2,1,0 of the second network byte
		//由回复时specified的return码：0:no error；1:format error；2:DNS error；3:domain name does not exist；4:DNS does not support this type of query；5:DNS rejected query；6-15:保留字段

#define DNS_OPCODE_QUERY 0x0 //standard query
#define DNS_OPCODE_IQUERY 0x08 //reverse query
#define DNS_OPCODE_STATUS 0x10 //statusquery

#define DNS_RCODE_ERR_OK 0x0 //no error
#define DNS_RCODE_ERR_FORMAT 0x0100 //format error
#define DNS_RCODE_ERR_DNS 0x0200 //DNS error
#define DNS_RCODE_ERR_EXIST 0x0300 //domain name does not exist
#define DNS_RCODE_ERR_SUPPORT 0x0400 //DNS does not support this type of query
#define DNS_RCODE_ERR_REJECT 0x0500 //DNS rejected query

//--------------constants here define the meaning of DNS_HEADER.flags bit masks for BIG-ENDIAN systems--------
//#define DNS_FLAGS_QR 0x8000 //bit 7 of the first network byte
//#define DNS_FLAGS_OPCODE 0x7800 //bits 6,5,4,3 of the first network byte
//#define DNS_FLAGS_AA 0x0400 //bit 2 of the first network byte
//#define DNS_FLAGS_TC 0x0200 //bit 1 of the first network byte
//#define DNS_FLAGS_RD 0x0100 //bit 0 of the first network byte
//#define DNS_FLAGS_RA 0x0080 //bit 7 of the second network byte
//#define DNS_FLAGS_Z 0x0070 //bits 6,5,4 of the second network byte
//#define DNS_FLAGS_RCODE 0x000f //bits 3,2,1,0 of the second network byte

//#define DNS_OPCODE_QUERY 0x0 //standard query
//#define DNS_OPCODE_IQUERY 0x0800 //reverse query
//#define DNS_OPCODE_STATUS 0x1000 //statusquery

//#define DNS_RCODE_ERR_OK 0x0 //no error
//#define DNS_RCODE_ERR_FORMAT 0x01 //format error
//#define DNS_RCODE_ERR_DNS 0x02 //DNS error
//#define DNS_RCODE_ERR_EXIST 0x03 //domain name does not exist
//#define DNS_RCODE_ERR_SUPPORT 0x04 //DNS does not support this type of query
//#define DNS_RCODE_ERR_REJECT 0x05 //DNS rejected query
//-----------------------------------------------------------------------
/* querytype Q_type
 * Type values for resources and queries
 */
#define T_A			1		/* host address (computer IP address)*/
#define T_NS		2		/* authoritative server (DNS name server for naming zone)*/
#define T_MD		3		/* mail destination (obsolete, use MX instead)*/
#define T_MF		4		/* mail forwarder (obsolete, use MX instead)*/
#define T_CNAME		5		/* canonical name (for aliases)*/
#define T_SOA		6		/* start of authority zone specified用于 DNS 区域的“起始authorization机构”*/
#define T_MB		7		/* mailbox domain name*/
#define T_MG		8		/* mail group member*/
#define T_MR		9		/* mail rename domain name*/
#define T_NULL		10		/* null resource record (empty resource record)*/
#define T_WKS		11		/* well known service description*/
#define T_PTR		12		/* domain name pointer: if query is IP address, specifies computer name; otherwise pointer to other info*/
#define T_HINFO		13		/* host information (computer CPU and OS type)*/
#define T_MINFO		14		/* mailbox or mailing list information*/
#define T_MX		15		/* mail routing information (mail exchanger)*/
#define T_TXT		16		/* text strings (text info)*/
#define T_RP		17		/* responsible person */
#define T_AFSDB		18		/* AFS cell database */
#define T_X25		19		/* X_25 calling address */
#define T_ISDN		20		/* ISDN calling address */
#define T_RT		21		/* router */
#define T_NSAP		22		/* NSAP address */
#define T_NSAP_PTR	23		/* reverse NSAP lookup (deprecated) */
#define T_SIG		24		/* security signature */
#define T_KEY		25		/* security key */
#define T_PX		26		/* X.400 mail mapping */
#define T_GPOS		27		/* geographical position (withdrawn) */
#define T_AAAA		28		/* IP6 Address */
#define T_LOC		29		/* Location Information */
#define T_NXT		30		/* Next Valid Name in Zone */
#define T_EID		31		/* Endpoint identifier */
#define T_NIMLOC	32		/* Nimrod locator */
#define T_SRV		33		/* Server selection */
#define T_ATMA		34		/* ATM Address */
#define T_NAPTR		35		/* Naming Authority PoinTeR */
	/* non standard */
#define T_UINFO		100		/* user (finger) information*/
#define T_UID		101		/* user ID (user identifier)*/
#define T_GID		102		/* group ID (group identifier for group name)*/
#define T_UNSPEC	103		/* Unspecified format (binary data) */
	/* Query type values which do not appear in resource records */
#define	T_IXFR		251		/* incremental zone transfer */
#define T_AXFR		252		/* transfer zone of authority */
#define T_MAILB		253		/* transfer mailbox records */
#define T_MAILA		254		/* transfer mail agent records */
#define T_ANY		255		/* wildcard match for all data types*/
/* queryclass Q_class
 * Values for class field
 */
#define C_IN		1		/* the arpa internet specifies Internet class*/
#define C_CSNET		2		/* specifies CSNET class. (obsolete)*/
#define C_CHAOS		3		/* for chaos net (MIT) specifies Chaos class*/
#define C_HS		4		/* for Hesiod name server (MIT) (XXX) specifies MIT Athena Hesiod class*/
	/* Query class values which do not appear in resource records specified任何以前列出的通配符*/
#define C_ANY		255		/* wildcard match */

//Q_type中的T_A,T_MX,T_CNAME为常用，Q_class中的C_IN为常用

typedef struct dns_protocol_header //DNSdata报头
{
	unsigned short id;
	//identifier，通过它client可以将DNS的request与应答相匹配
	unsigned short flags;
	//flag：[QR | opcode | AA| TC| RD| RA | zero | rcode ]
	unsigned short quests;
	//问题数目
	unsigned short answers;
	//resource记录数目
	unsigned short author;
	//authorizationresource记录数目
	unsigned short addition;
	//额外resource记录数目
}DNS_HEADER,*PDNS_HEADER;
typedef struct dns_protocol_query //DNS querydata报
{
	const char *name;
	//query的domain name,这is asizeat0到63之间的string
	unsigned short type;
	//querytype，见Q_type_arraydefine
	unsigned short classes;
	//queryclass,见Q_class_array 通常yesAclass既queryIPaddress

}DNS_QUERY,*PDNS_QUERY;
typedef struct dns_protocol_response //DNSresponsedata报
{
	const char *name; //回复query的domain name，not定长
	//query的domain name
	unsigned short type; //回复的type。2byte，与query同义。指示RDATA中的resource记录type
	//querytype
	unsigned short classes; //回复的class。2byte，与query同义。指示RDATA中的resource记录class
	//type码
	unsigned long	ttl; //生存time。4byte，指示RDATA中的resource记录atcache的生存time
	//生存time
	unsigned short length; //length。2byte，指示RDATA块的length(notpacket括本字段的两byte)
	//resourcedatalength
	unsigned char *	rdata; //resource记录。notdefine，依TYPE的not同，此记录的格示not同，
						  //通常一个MX记录yes由一个2byte的指示该邮件交换器的优先级值及not定长的邮件交换器名组成的
	//resourcedata
}DNS_RESPONSE,*PDNS_RESPONSE;

#endif

/*************************
name的组合形式。name由multipleidentifier序列组成，每一个identifier序列的首byte说明该identifier符的length，
接着用yesASCII码表示character，multiple序列after由byte0表示名字end。
其中某一个identifier序列的首character的length若yes0xC0的话，表示下一byte指示is notidentifier符序列，
而yes指示接下partialat本receivepacket内的偏移positionat this pointnot已byte0表示名字end。 
　　比如　bbs.zzsy.com　以.分开bbs、zzsy、com三个partial。eachpartial的length为3、4、3 
　　atDNS报文中的形式就如 3 b b s 4 z z s y 3 c o m 0 
　　false如atpacket内的第12个bytepositionexists有 4 z z s y 3 c o m 0 这样的name了。 
　　那at this point有可能为：3 b b s 4 z z s y 0xC0 0x0C 这样的形式。
**************************/
