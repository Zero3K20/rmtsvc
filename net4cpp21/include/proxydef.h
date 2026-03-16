/*******************************************************************
   *	proxydef.h
   *    DESCRIPTION:constants, structures and enum definitions for the proxy protocol
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_PROXYDEF_H__
#define __YY_PROXYDEF_H__

#include <string>
#include "IPRules.h"

#define SOCKSERR_PROXY_AUTH -201 //authentication negotiation failed
#define SOCKSERR_PROXY_REJECT SOCKSERR_PROXY_AUTH-1
#define SOCKSERR_PROXY_ATYP SOCKSERR_PROXY_AUTH-2 //unsupported address type
#define SOCKSERR_PROXY_USER SOCKSERR_PROXY_AUTH-3
#define SOCKSERR_PROXY_DENY SOCKSERR_PROXY_AUTH-4
#define SOCKSERR_PROXY_EXPIRED SOCKSERR_PROXY_AUTH-5
#define SOCKSERR_PROXY_CONNECTIONS SOCKSERR_PROXY_AUTH-6 //too many connections

#define PROXY_MAX_RESPTIMEOUT 10 //s maximum response delay
#define PROXY_SERVER_PORT	1080 //default proxy service port

typedef enum //proxy type definitions
{
	PROXY_NONE=0,//no proxy
	PROXY_HTTPS=1,
	PROXY_SOCKS4=2,
	PROXY_SOCKS5=4 
}PROXYTYPE; 

typedef enum //proxy request type definitions
{
	PROXYREQ_TCP,
	PROXYREQ_BIND,//
	PROXYREQ_UDP //UDP proxy is only supported by the SOCKS5 proxy protocol
}PROXYREQTYPE;

typedef struct _proxyaccount //proxyaccountinfo
{
	std::string m_username;//account name, case-insensitive (converted to lowercase)
	std::string m_userpwd;//password; if password=="", no password verification is required
	net4cpp21::iprules m_ipRules;//source IP access rules

	unsigned long m_maxratio;//maximum bandwidth K/s, 0 means unlimited
	long m_loginusers;//current number of users logged in with this account; can only delete this account when no users are connected
	long m_maxLoginusers;//limit the maximum simultaneous logged-in users for this account; <=0 means unlimited 
	time_t m_limitedTime;//limit this account to be valid only before a certain date; ==0 means unlimited
	
	net4cpp21::iprules m_dstRules;//destination IP access rules
}PROXYACCOUNT;

//set 1-byte alignment
//#ifdef WIN32
//	#pragma pack(push)
//	#pragma pack(1)
//#endif

	//SOCKS4 request/response structure
	typedef struct _sock4req
	{
	char VN; //version ,must be 4
	char CD; //socks4 command, 1--connect 2--bind
	unsigned short Port;
	unsigned long IPAddr; //SOCKS4 proxy protocol does not support server-side domain name parsing
	char other[1];
	}sock4req;
	typedef struct _sock4ans
	{
	char VN; //should be 0
	char CD; //reply code
			//90: request granted
			//91: request rejected or failed
			//92: request rejected becasue SOCKS server cannot connect to
			//	identd on the client
			//93: request rejected because the client program and identd
			//	report different user-ids
	unsigned short Port; 
	unsigned long IPAddr;
	}sock4ans;
	//in the CONNECT response packet, only the VN and CD fields are meaningful; DSTPORT and DSTIP are ignored
	//******************************
	//SOCKS5 proxy request/response structure
	typedef struct _sock5req
	{
	char Ver;
	char nMethods;
	char Methods[255];
	}sock5req;
	typedef struct _sock5ans
	{
	char Ver;
	char Method;
	}sock5ans;
	typedef struct _sock5req1
	{
	char Ver;
	char Cmd;// 1=CONNECT, 2=TCP BIND, 3=UDP BIND
	char Rsv;// must be 0
	char Atyp;// 1=4 byte IP address, 3=domain name, 4=6 byte IP address
	char other[1]; // depends on address type， x bytes
	//unsigned short port; // should be in BIG ENDIAN format!
	}sock5req1;
	typedef struct _sock5ans1
	{
	char Ver;
	char Rep;
	char Rsv;// must be 0
	char Atyp;// 1=4 byte IP address, 3=domain name, 4=6 byte IP address
	//char other[1]; // depends on address type， x bytes
	//unsigned short port; // should be in BIG ENDIAN format!
	//yyc modify 2003-01-17,only support IP address 
	unsigned long IPAddr;
	unsigned short Port;
	//	unsigned long IPAddr end**************
	}sock5ans1;
	typedef struct _authreq
	{
	char Ver;//ver=1, authentication version not proxy version 
	unsigned char Ulen;// 1..255,usernamelength
	char Name[255];//username,// not NULL terminated!
	unsigned char PLen;//1..255,passwordlength
	char Pass[255];// not NULL terminated!
	}authreq;
	typedef struct _authans
	{
	char Ver;
	char Status;
	}authans;

	//UDP proxy packet structure
	typedef struct _socks5udp
	{
		unsigned short Rsv;  //must be 0
		char Frag; //flag for whether to reassemble fragmented data
		char Atyp;// 1=4 byte IP address, 3=domain name, 4=6 byte IP address
		unsigned long IPAddr;
		unsigned short Port;
	}socks5udp;
//#ifdef WIN32
//	#pragma pack(pop)
//#endif

#endif

/*   RFC SOCKS4
The SOCKS protocol was originally designed by David Koblas and later improved by Ying-Da Lee into SOCKS 4.

SOCKS 4 only supports TCP forwarding.

request message format:

+----+----+----+----+----+----+----+----+----+----+...+----+
| VN | CD | DSTPORT |      DSTIP        | USERID      |NULL|
+----+----+----+----+----+----+----+----+----+----+...+----+
   1    1      2              4           variable       1

VN      SOCKS protocol version number, should be 0x04

CD      SOCKS command, possible values:

        0x01    CONNECT
        0x02    BIND

DSTPORT port info related to CD

DSTIP address info related to CD

USERID  client's user ID

NULL    0x00

response message format:

+----+----+----+----+----+----+----+----+
| VN | CD | DSTPORT |      DSTIP        |
+----+----+----+----+----+----+----+----+
   1    1      2              4

VN      should be 0x00 not 0x04

CD      possible values:

        0x5A    forwarding allowed
        0x5B    forwarding denied, general failure
        0x5C    forwarding denied, SOCKS 4 Server cannot connect to the IDENT service on the SOCKS 4 Client host
                IDENTservice
        0x5D    forwarding denied, USERID in request does not match the IDENT service response

DSTPORT port info related to CD

DSTIP address info related to CD

1) CONNECT command

for CONNECT requests, DSTIP/DSTPORT specifies the forwarding destination.

SOCKS 4 Server evaluates based on source IP, DSTPORT, DSTIP, USERID and info from SOCKS 4 Client's host
IDENT service (RFC 1413) to decide whether to establish the connection or reject forwarding.
forwarding.

if the CONNECT request is allowed, SOCKS 4 Server tries to establish a TCP connection to the forwarding destination, then
sends a response packet to SOCKS 4 Client, indicating whether the forwarding connection was established successfully.

if the CONNECT request is rejected, SOCKS 4 Server also sends a response to SOCKS 4 Client, then
immediately closes the connection.

in the CONNECT response, only VN and CD fields are meaningful; DSTPORT and DSTIP are ignored. If CD equals
0x5A, the forwarding connection was established; SOCKS 4 Client then sends data directly on the current TCP connection.
forwarded data.

2) BIND command

The FTP protocol sometimes requires the FTP Server to actively establish a connection to the FTP Client (the FTP data stream).

FTP Client - SOCKS 4 Client - SOCKS 4 Server - FTP Server

a. FTP Client tries to establish FTP control stream. SOCKS 4 Client sends CONNECT
   request to SOCKS 4 Server; the latter responds and the FTP control stream is established.

   The CONNECT request specifies FTPSERVER.ADDR/FTPSERVER.PORT.

b. FTP Client tries to establish FTP data stream. SOCKS 4 Client creates a new TCP connection to SOCKS 4 Server
   TCP connection, and sends a BIND request on the new TCP connection.

   The BIND request still specifies FTPSERVER.ADDR/FTPSERVER.PORT.

   SOCKS 4 Server receives BIND request and evaluates it based on this info and USERID.
   create a new socket listening on AddrA/PortA, and send to SOCKS 4 Client the first BIND response
   packet.

   if CD in the BIND response is not 0x5A, it indicates failure; DSTPORT and DSTIP fields in the packet are ignored.

   when CD in the BIND response is 0x5A, DSTIP/DSTPORT in the packet corresponds to AddrA/PortA. If DSTIP equals
   to 0 (INADDR_ANY), SOCKS 4 Client should replace it with the SOCKS 4 Server's IP, when SOCKS
   0 (INADDR_ANY), SOCKS 4 Client should replace it with the SOCKS 4 Server's IP (possible when SOCKS 4 Server is not multi-homed).

c. SOCKS 4 Client receives the first BIND response packet.

   FTP Client calls getsockname (not getpeername) to get AddrA/PortA, via FTP control
   sends PORT command to FTP Server via the control stream, telling FTP Server to actively connect to AddrA/PortA
   TCP connection.

d. FTP Server receives PORT command and actively establishes a TCP connection to AddrA/PortA. Assume the TCP connection 4-tuple is:
   4-tuple:

   AddrB, PortB, AddrA, PortA

e. SOCKS 4 Server receives the TCP connection request from FTP Server, checks whether the source IP (
   AddrB) whether it matches FTPSERVER.ADDR, then sends the second BIND response to SOCKS 4 Client
   packet.

   when source IP does not match, CD in the second BIND response is set to 0x5B, then SOCKS 4 Server closes
   the TCP connection used to send the second BIND response, as well as the TCP connection to FTP Server,
   but the main TCP connection (the one related to the CONNECT request) remains open.

   if the source IP matches, CD is set to 0x5A; SOCKS 4 Server starts forwarding the FTP data stream.

   in any case, DSTPORT and DSTIP fields in the second BIND response are ignored.

for CONNECT and BIND requests, SOCKS 4 Server has a timer (current CSTC implementation uses 2 minutes).
if the timer expires and the TCP connection between SOCKS 4 Server and Application Server (outgoing or
incoming) is still not established, SOCKS 4 Server will close the corresponding TCP connection to SOCKS 4 Client and
弃correspondingforwarding.
*/

/*       RFC1928 socks5
The SOCKS protocol sits between the transport layer (TCP/UDP, etc.) and the application layer, above the network layer (IP).
Things like IP packet forwarding and ICMP protocol are too low-level to be relevant to SOCKS.

SOCKS 4 does not support authentication, UDP protocol, or remote FQDN parsing. SOCKS 5 supports all of these.

SOCKS Server listens on 1080/TCP by default. This is the first message sent by SOCKS Client after connecting to SOCKS Server:


+----+----------+----------+
|VER | NMETHODS | METHODS  |
+----+----------+----------+
| 1  |    1     | 1 to 255 |
+----+----------+----------+

For SOCKS 5, VER=0x05; version 4 uses 0x04. NMETHODS specifies the byte
count of the METHODS field. It's unclear if NMETHODS can be 0; it can take values [1,255]. The number of bytes
in METHODS indicates how many authentication mechanisms the SOCKS Client supports.

SOCKS Server selects one byte (one authentication mechanism) from METHODS and sends a response to SOCKS Client:


+----+--------+
|VER | METHOD |
+----+--------+
| 1  |   1    |
+----+--------+

currently available METHOD values:

0x00        NO AUTHENTICATION REQUIRED (no authentication needed)
0x01        GSSAPI
0x02        USERNAME/PASSWORD (username/password authentication)
0x03-0x7F   IANA ASSIGNED
0x80-0xFE   RESERVED FOR PRIVATE METHODS (private authentication methods)
0xFF        NO ACCEPTABLE METHODS (completely incompatible)

if SOCKS Server responds with 0xFF, it means SOCKS Server and SOCKS Client are completely incompatible;
SOCKS Client must close the TCP connection. After authentication negotiation is complete, SOCKS Client and
SOCKS Server perform sub-negotiation for the chosen authentication mechanism (see other documents). For maximum compatibility,
SOCKS Client and SOCKS Server must support 0x01 and should support 0x02.

after authentication sub-negotiation is complete, SOCKS Client submits a forwarding request:

+----+-----+-------+------+----------+----------+
|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
+----+-----+-------+------+----------+----------+
| 1  |  1  | X'00' |  1   | Variable |    2     |
+----+-----+-------+------+----------+----------+

VER         0x05 for version 5

CMD         possible values:

            0x01    CONNECT
            0x02    BIND
            0x03    UDP ASSOCIATE

RSV         reserved field, must be 0x00

ATYP        specifies the type of DST.ADDR field, possible values:

            0x01    IPv4address
            0x03    FQDN (fully qualified domain name)
            0x04    IPv6address

DST.ADDR    address info related to CMD; don't be confused by the name DST

            if IPv4 address, this is 4 bytes of data in big-endian order

            if FQDN, e.g. "www.nsfocus.net", this will be:

            0F 77 77 77 2E 6E 73 66 6F 63 75 73 2E 6E 65 74

            note: no terminating NUL character; not an ASCIZ string; first byte is the length field

            if IPv6 address, this is 16 bytes of data.

DST.PORT    port info related to CMD, 2 bytes in big-endian order

SOCKS Server evaluates the forwarding request from SOCKS Client and sends a response:

+----+-----+-------+------+----------+----------+
|VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
+----+-----+-------+------+----------+----------+
| 1  |  1  | X'00' |  1   | Variable |    2     |
+----+-----+-------+------+----------+----------+

VER         0x05 for version 5

REP         可取如下值:

            0x00        success
            0x01        general failure
            0x02        connection not allowed by ruleset
            0x03        network unreachable
            0x04        host unreachable
            0x05        connection refused
            0x06        TTLtimeout
            0x07        command not supported
            0x08        address type not supported
            0x09-0xFF   unassigned

RSV         reserved field, must be 0x00

ATYP        用于specifiesBND.ADDR域的type

BND.ADDR    CMDrelatedaddressinfo，do not为BND所迷惑

BND.PORT    CMDrelatedportinfo，big-endian序的2bytedata

1) CONNECT command

ifCMD为CONNECT，SOCKS Client、SOCKS Server之间通信的相关4-tuple:

SOCKSCLIENT.ADDR，SOCKSCLIENT.PORT，SOCKSSERVER.ADDR，SOCKSSERVER.PORT

一般SOCKSSERVER.PORTyes1080/TCP。

CONNECTrequestpacket中的DST.ADDR/DST.PORTspecifiesforwarding destination。SOCKS Server可以靠
DST.ADDR、DST.PORT、SOCKSCLIENT.ADDR、SOCKSCLIENT.PORTevaluate，以决定建立
到forwarding destination的TCPconnect还yesrejectforwarding.

if规则forwarding allowed并且successestablish connection toforwarding destination的TCP connection,相关4-tuple:

BND.ADDR，BND.PORT，DST.ADDR，DST.PORT

at this pointSOCKS Server向SOCKS Clientsend的CONNECTresponsepacket中将specifiesBND.ADDR/BND.PORT。
注意，BND.ADDR可能not同于SOCKSSERVER.ADDR，SOCKS Server所on the host可能yes多目(
multi-homed)主机。

ifrejectforwardor未能successestablish connection toforwarding destination的TCP connection,CONNECTresponsepacket中REP字段将
specifies具体原因。

responsepacket中REP非零时表示failure，SOCKS Server必须atsend responsepacket后not久(not超过10s)关
闭与SOCKS Client之间的TCP connection.

responsepacket中REP为零时表示success。afterSOCKS Clientdirectly oncurrentTCPconnect上send待forward数
据。

2) BIND command

ifCMD为BIND。这多用于FTPprotocol，FTPprotocolatsome情况下要求FTP Serveractively establish
到FTP Client的connect，即FTPdata流。

FTP Client - SOCKS Client - SOCKS Server - FTP Server

a. FTP Client tries toestablish FTP control stream。SOCKS Client向SOCKS Serversend CONNECTrequest，
   the latter responds to the request, and the FTP control stream is established.

   The CONNECT request specifies FTPSERVER.ADDR/FTPSERVER.PORT.

b. FTP Client tries toestablish FTP data stream。SOCKS Client建立新的到SOCKS Server的TCP连
   ection, and sends a BIND request on the new TCP connection.

   The BIND request still specifies FTPSERVER.ADDR/FTPSERVER.PORT. SOCKS Server should accordingly
   for evaluation.

   SOCKS Server receives BIND request, creates new socket listening on AddrA/PortA, and sends to SOCKS
   Client sends the first BIND response packet with BND.ADDR/BND.PORT as AddrA/PortA.

c. SOCKS Client收到first个BINDresponsepacket.FTP Clientvia FTP controlto FTP Server via发
   send PORT command，notificationFTP Servershould主动establish connection toAddrA/PortA的TCP connection.

d. FTP Server receives PORT command and actively establishes a TCP connection to AddrA/PortA. Assume the TCP connection 4-tuple is:
   4-tuple:

   AddrB, PortB, AddrA, PortA

e. SOCKS Server收到来自FTP Server的TCPconnectrequest，向SOCKS Clientsend第二个
   BIND response packet with BND.ADDR/BND.PORT as AddrB/PortB. Then SOCKS Server starts forwarding
   forward FTP data stream.

下面yes一些讨论记录:

scz

为什么需要send the second BIND responsepacket，specifiesAddrB/PortB的意义何at。

knightmare@apue

specifiesAddrB/PortB的意义at于，FTP Client出于安全考虑，会checkFTPdata流的源IP、
源port，比如FTPdata流的源端只允许yesFTPSERVER.ADDR/20。

scz

knightmare的答案yescorrect，但我的疑惑可能源于我对SOCKSprotocol的error理解，以至
提出一个产生歧义的问题。事实上should查看David Koblas的原始文档以理解BINDrequest
的全过程。前面关于FTPdata流的descriptionpartial已做了修正，therefore看not出提问的缘由了。

3) UDP ASSOCIATEcommand

ifCMD为UDP ASSOCIATE。at this pointDST.ADDR与DST.PORTspecifiessendUDP报文时的源IP、源
port，而is notUDPforwarding destination，SOCKS Server可以据此evaluate以决定whether进行UDP转
发。ifSOCKS ClientsendUDP ASSOCIATEcommand时无法提供DST.ADDR与DST.PORT，则
必须将这两个域置零。

下面yes一些讨论记录:

scz

什么情况下SOCKS ClientsendUDP ASSOCIATEcommand，又无法提供DST.ADDR与DST.PORT，
or者说出于什么考虑才需要刻意将这两个域置零。有现实例子exists吗。

shixudong@163.com

考虑这种情况:

Application Client - SOCKS Client - NAT - SOCKS Server - Application Server

SOCKS ClientatUDP ASSOCIATEcommand中specifiesDST.ADDR/DST.PORT，SOCKS Server靠这些
info决定whetherforwarda certainUDP报文。上图中SOCKS Client与SOCKS Server之间有NAT，前
者无法预知UDP报文经过NAT后源IP、源port会变成什么样，但肯定会变，therefore前者无
法提前atUDP ASSOCIATEcommand中specifiesDST.ADDR/DST.PORT，if强行specified非零值，后者
会检测到待forwardUDP报文的源IP、源port与DST.ADDR/DST.PORTnot匹配而rejectforwarding.针
对这种情况，RFC 1928建议SOCKS Client将DST.ADDR/DST.PORT置零，SOCKS Server
at this pointno longercheck待forwardUDP报文的源IP、源port。

at一条TCPconnect上SOCKS Client向SOCKS Serversend了UDP ASSOCIATEcommand，后续UDP
forward要求此TCPconnectcontinue维持，此TCPconnectclose时correspondingUDPforward也将中止。换句话说，
UDPforward必然伴随着一个TCP connection,这将消耗额外的resource。

SOCKS Server向SOCKS ClientsendUDP ASSOCIATEresponsepacket，BND.ADDR/BND.PORTspecifies
SOCKS Client应向哪里send待forwardUDP报文。

对于UDPforward，SOCKS Clientsend出去的UDPdata区如下:

+----+------+------+----------+----------+----------+
|RSV | FRAG | ATYP | DST.ADDR | DST.PORT |   DATA   |
+----+------+------+----------+----------+----------+
| 2  |  1   |  1   | Variable |    2     | Variable |
+----+------+------+----------+----------+----------+

RSV         reserved field, must be 0x0000

FRAG        Current fragment number

            0x00        this is a non-fragmented SOCKS UDP packet
            0x01-0x7F   SOCKS fragment sequence number
            0x80-0xFF   high bit set means end of fragment sequence, i.e. this is the last SOCKS fragment

ATYP        specifies the type of DST.ADDR field, possible values:

            0x01    IPv4address
            0x03    FQDN (fully qualified domain name)
            0x04    IPv6address

DST.ADDR    forward目标address

DST.PORT    forward目标port

DATA        原始UDPdata区

SOCKS Server静静地为SOCKS Client进行UDPforward，并notnotification后者forwardcomplete还yes被拒
绝。

FRAG用于支持SOCKS碎片。SOCKS碎片receive方一般implementation有reassemblequeue与reassemble定时器。if
reassemble定时器timeoutor者低序SOCKS碎片后于高序SOCKS碎片到达reassemblequeue，at this point必须reset
reassemblequeue。reassemble定时器not得小于5 seconds。should尽可能地避免出现SOCKS碎片。

whether支持SOCKS碎片yes可选的，if一个SOCKSimplementationnot supportedSOCKS碎片，则必须丢弃所
有receive到的SOCKS碎片，即那些FRAG字段非零的SOCKS UDP报文。

由于SOCKSimplementationat支持UDPforward时会at原始UDPdata区前增加一个SOCKSprotocolrelated头，
therefore为UDPdata区allocate space时要为这个头留足space:

ATYP    头占用byte  原因
0x01    10          IPv4address占4byte，4+6=10
0x03    262         length域is abyte，thereforemaximum0xFF，1+255+6=262
0x04    20          这里我怀疑yes笔误，IPv6address占16byte，16+6=22

我怀疑RFC 1928这里有笔误，写信询问mleech@bnr.ca、ietf-web@ietf.org去了。
*/
/*     RFC 1929
ifSOCKS V5 Client/Servernegotiate采用username/口令authentication机制(0x02)，现atstart相应
子negotiate。

clientsend如下报文:

+----+------+----------+------+----------+
|VER | ULEN |  UNAME   | PLEN |  PASSWD  |
+----+------+----------+------+----------+
| 1  |  1   | 1 to 255 |  1   | 1 to 255 |
+----+------+----------+------+----------+

VER     子negotiate的currentversion，目前yes0x01

ULEN    UNAME字段的length

UNAME   username

PLEN    PASSWD字段的length

PASSWD  口令，注意yes明文transfer的

server-sideauthentication后send response packet如下:

+----+--------+
|VER | STATUS |
+----+--------+
| 1  |   1    |
+----+--------+

VER     子negotiate的currentversion，目前yes0x01

STATUS  可取如下值:

        0x00        success
        0x01-0xFF   failure; the SOCKS Server must then close the TCP connection with the SOCKS Client
                    connect
*/
