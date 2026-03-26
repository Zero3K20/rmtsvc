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
		//0 represents a query, 1 represents a DNS reply
#define DNS_FLAGS_OPCODE 0x78 //bits 6,5,4,3 of the first network byte
		//indicates query kind/class: 0: standard query; 1: reverse query; 2: server status query; 3-15: unused.
#define DNS_FLAGS_AA 0x04 //bit 2 of the first network byte
		//whether this is an authoritative reply
#define DNS_FLAGS_TC 0x02 //bit 1 of the first network byte
		//since a UDP packet is 512 bytes, this bit indicates whether the message is truncated
#define DNS_FLAGS_RD 0x01 //bit 0 of the first network byte
		//this bit is specified in the query and echoed in the reply. Set to 1 to instruct the server to perform a recursive query
#define DNS_FLAGS_RA 0x8000 //bit 7 of the second network byte
		//returned in the DNS reply, indicates whether the DNS server supports recursive queries
#define DNS_FLAGS_Z 0x7000 //bits 6,5,4 of the second network byte
		//reserved field, must be set to 0
#define DNS_FLAGS_RCODE 0x0f00 //bits 3,2,1,0 of the second network byte
		//return code specified in the reply: 0: no error; 1: format error; 2: DNS error; 3: domain name does not exist; 4: DNS does not support this type of query; 5: DNS rejected query; 6-15: reserved

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
#define T_SOA		6		/* start of authority zone, used for the "Start of Authority" of a DNS zone*/
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
	/* Query class values which do not appear in resource records; wildcard for all previously listed types*/
#define C_ANY		255		/* wildcard match */

//T_A, T_MX, T_CNAME in Q_type are the most commonly used; C_IN in Q_class is the most commonly used

typedef struct dns_protocol_header //DNS data header
{
	unsigned short id;
	//identifier, used by the client to match DNS requests with responses
	unsigned short flags;
	//flags: [QR | opcode | AA | TC | RD | RA | zero | rcode]
	unsigned short quests;
	//number of questions
	unsigned short answers;
	//number of resource records
	unsigned short author;
	//number of authority resource records
	unsigned short addition;
	//number of additional resource records
}DNS_HEADER,*PDNS_HEADER;
typedef struct dns_protocol_query //DNS query data record
{
	const char *name;
	//queried domain name, a string with length between 0 and 63
	unsigned short type;
	//query type, see Q_type array definition
	unsigned short classes;
	//query class, see Q_class array; typically class A for querying IP addresses

}DNS_QUERY,*PDNS_QUERY;
typedef struct dns_protocol_response //DNS response data record
{
	const char *name; //domain name in the reply, variable length
	//queried domain name
	unsigned short type; //reply type. 2 bytes, same meaning as in query. Indicates the resource record type in RDATA
	//querytype
	unsigned short classes; //reply class. 2 bytes, same meaning as in query. Indicates the resource record class in RDATA
	//type code
	unsigned long	ttl; //time to live. 4 bytes, indicates the TTL of the resource record in RDATA cache
	//time to live
	unsigned short length; //length. 2 bytes, indicates the length of the RDATA block (not including these 2 bytes)
	//resourcedatalength
	unsigned char *	rdata; //resource record. Variable format, differs based on TYPE,
						  //typically an MX record consists of a 2-byte priority value and a variable-length mail exchanger name
	//resourcedata
}DNS_RESPONSE,*PDNS_RESPONSE;

#endif

/*************************
The name format. A name consists of multiple label sequences; the first byte of each label indicates
its length, followed by ASCII characters. After multiple sequences, a byte of 0 indicates the end of the name.
If the first byte of a label sequence has a length of 0xC0, the next byte indicates an offset pointer
within the received packet, not an ASCII label sequence, and does not end with byte 0.
   For example, bbs.zzsy.com is split by "." into three parts: bbs, zzsy, com. The length of each part is 3, 4, 3.
   In a DNS packet it would appear as: 3 b b s 4 z z s y 3 c o m 0.
   Suppose the name "4 z z s y 3 c o m 0" already appears at byte offset 12 in the packet.
   Then the representation could be: 3 b b s 4 z z s y 0xC0 0x0C.
**************************/
