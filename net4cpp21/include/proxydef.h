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
	char other[1]; // depends on address type, x bytes
	//unsigned short port; // should be in BIG ENDIAN format!
	}sock5req1;
	typedef struct _sock5ans1
	{
	char Ver;
	char Rep;
	char Rsv;// must be 0
	char Atyp;// 1=4 byte IP address, 3=domain name, 4=6 byte IP address
	//char other[1]; // depends on address type, x bytes
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
discard the corresponding forwarding.
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

REP         possible values:

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

ATYP        specifies the type of the BND.ADDR field

BND.ADDR    address info related to CMD; do not be confused by the name BND

BND.PORT    port info related to CMD, 2 bytes in big-endian order

1) CONNECT command

if CMD is CONNECT, the relevant 4-tuple for communication between SOCKS Client and SOCKS Server:

SOCKSCLIENT.ADDR, SOCKSCLIENT.PORT, SOCKSSERVER.ADDR, SOCKSSERVER.PORT

Typically SOCKSSERVER.PORT is 1080/TCP.

The DST.ADDR/DST.PORT in the CONNECT request packet specifies the forwarding destination. SOCKS Server can evaluate
DST.ADDR, DST.PORT, SOCKSCLIENT.ADDR, SOCKSCLIENT.PORT to decide whether to establish
a TCP connection to the forwarding destination or reject the forwarding.

If forwarding is allowed by rules and the TCP connection to the forwarding destination is successfully established, the relevant 4-tuple is:

BND.ADDR, BND.PORT, DST.ADDR, DST.PORT

At this point, the CONNECT response packet sent by SOCKS Server to SOCKS Client will specify BND.ADDR/BND.PORT.
Note that BND.ADDR may differ from SOCKSSERVER.ADDR; the host on which SOCKS Server resides may be
a multi-homed host.

If forwarding is rejected or the TCP connection to the forwarding destination cannot be successfully established, the REP field in the CONNECT response packet will
specify the specific reason.

A non-zero REP in the response packet indicates failure; SOCKS Server must close
the TCP connection with SOCKS Client shortly after sending the response packet (within 10 seconds).

A zero REP in the response packet indicates success. After that, SOCKS Client directly sends the data to be forwarded on the current TCP connection.

2) BIND command

If CMD is BIND. This is mostly used by the FTP protocol, which in some cases requires the FTP Server to actively establish
a connection to the FTP Client, i.e., the FTP data stream.

FTP Client - SOCKS Client - SOCKS Server - FTP Server

a. FTP Client tries to establish FTP control stream. SOCKS Client sends a CONNECT request to SOCKS Server,
   the latter responds to the request, and the FTP control stream is established.

   The CONNECT request specifies FTPSERVER.ADDR/FTPSERVER.PORT.

b. FTP Client tries to establish FTP data stream. SOCKS Client establishes a new TCP con
   ection, and sends a BIND request on the new TCP connection.

   The BIND request still specifies FTPSERVER.ADDR/FTPSERVER.PORT. SOCKS Server should accordingly
   for evaluation.

   SOCKS Server receives BIND request, creates new socket listening on AddrA/PortA, and sends to SOCKS
   Client sends the first BIND response packet with BND.ADDR/BND.PORT as AddrA/PortA.

c. SOCKS Client receives the first BIND response packet. FTP Client sends a PORT command via FTP control to FTP Server,
   notifying FTP Server to actively establish a TCP connection to AddrA/PortA.

d. FTP Server receives PORT command and actively establishes a TCP connection to AddrA/PortA. Assume the TCP connection 4-tuple is:
   4-tuple:

   AddrB, PortB, AddrA, PortA

e. SOCKS Server receives the TCP connection request from FTP Server, and sends the second
   BIND response packet with BND.ADDR/BND.PORT as AddrB/PortB. Then SOCKS Server starts forwarding
   forward FTP data stream.

Below are some discussion notes:

scz

Why is the second BIND response packet needed, and what is the significance of specifying AddrB/PortB?

knightmare@apue

The significance of specifying AddrB/PortB is that FTP Client, for security reasons, will check the source IP and
source port of the FTP data stream; for example, the FTP data stream's source is only allowed to be FTPSERVER.ADDR/20.

scz

knightmare's answer is correct, but my confusion may have stemmed from a misunderstanding of the SOCKS protocol, which led
to asking an ambiguous question. In fact, one should consult David Koblas's original documentation to understand the full
BIND request process. The earlier description of the FTP data stream has been corrected, so the reason for the question is no longer apparent.

3) UDP ASSOCIATEcommand

If CMD is UDP ASSOCIATE. In this case, DST.ADDR and DST.PORT specify the source IP and source
port when sending UDP packets, not the UDP forwarding destination. SOCKS Server can evaluate this to decide whether to perform UDP
forwarding. If SOCKS Client cannot provide DST.ADDR and DST.PORT when sending the UDP ASSOCIATE command,
these two fields must be set to zero.

Below are some discussion notes:

scz

In what situation would SOCKS Client send a UDP ASSOCIATE command and be unable to provide DST.ADDR and DST.PORT,
or in other words, what consideration would require deliberately setting these two fields to zero? Are there real-world examples?

shixudong@163.com

Consider this scenario:

Application Client - SOCKS Client - NAT - SOCKS Server - Application Server

SOCKS Client specifies DST.ADDR/DST.PORT in the UDP ASSOCIATE command, and SOCKS Server uses this
info to decide whether to forward a given UDP packet. In the diagram above, there is a NAT between SOCKS Client and SOCKS Server; the
client cannot predict what the source IP and source port of the UDP packet will look like after NAT, but they will definitely change, so the client
cannot specify DST.ADDR/DST.PORT in the UDP ASSOCIATE command in advance. If non-zero values are forced, the server
will detect that the source IP and source port of the UDP packet to be forwarded do not match DST.ADDR/DST.PORT and reject forwarding.
For this situation, RFC 1928 recommends that SOCKS Client set DST.ADDR/DST.PORT to zero, and SOCKS Server
will then no longer check the source IP and source port of the UDP packets to be forwarded.

On a TCP connection, SOCKS Client sends a UDP ASSOCIATE command to SOCKS Server, and subsequent UDP
forwarding requires this TCP connection to remain open. When this TCP connection closes, the corresponding UDP forwarding will also stop. In other words,
UDP forwarding is always accompanied by a TCP connection, which consumes additional resources.

SOCKS Server sends a UDP ASSOCIATE response packet to SOCKS Client; BND.ADDR/BND.PORT specifies
where SOCKS Client should send the UDP packets to be forwarded.

For UDP forwarding, the UDP data area sent by SOCKS Client is as follows:

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

DST.ADDR    forwarding destination address

DST.PORT    forwarding destination port

DATA        original UDP data area

SOCKS Server silently performs UDP forwarding for SOCKS Client, without notifying the client whether forwarding
succeeded or was rejected.

FRAG is used to support SOCKS fragmentation. The receiver of SOCKS fragments generally implements a reassembly queue and reassembly timer. If
the reassembly timer expires or a lower-sequence SOCKS fragment arrives at the reassembly queue after a higher-sequence one, the
reassembly queue must be reset. The reassembly timer must not be less than 5 seconds. SOCKS fragmentation should be avoided as much as possible.

Whether to support SOCKS fragmentation is optional. If a SOCKS implementation does not support SOCKS fragmentation, it must discard
all received SOCKS fragments, i.e., those SOCKS UDP packets where the FRAG field is non-zero.

Because a SOCKS implementation will prepend a SOCKS protocol-related header to the original UDP data area when supporting UDP forwarding,
you must reserve enough space for this header when allocating space for the UDP data area:

ATYP    header bytes  reason
0x01    10            IPv4 address takes 4 bytes, 4+6=10
0x03    262           length field is 1 byte, so maximum is 0xFF, 1+255+6=262
0x04    20            suspected typo here; IPv6 address takes 16 bytes, 16+6=22

I suspect there is a typo in RFC 1928 here; I have written to mleech@bnr.ca and ietf-web@ietf.org about it.
*/
/*     RFC 1929
If SOCKS V5 Client/Server negotiation uses the username/password authentication mechanism (0x02), the corresponding
sub-negotiation now begins.

The client sends the following packet:

+----+------+----------+------+----------+
|VER | ULEN |  UNAME   | PLEN |  PASSWD  |
+----+------+----------+------+----------+
| 1  |  1   | 1 to 255 |  1   | 1 to 255 |
+----+------+----------+------+----------+

VER     current version of sub-negotiation, currently 0x01

ULEN    length of the UNAME field

UNAME   username

PLEN    length of the PASSWD field

PASSWD  password, note that it is transferred in plaintext

After server-side authentication, the response packet sent is as follows:

+----+--------+
|VER | STATUS |
+----+--------+
| 1  |   1    |
+----+--------+

VER     current version of sub-negotiation, currently 0x01

STATUS  possible values:

        0x00        success
        0x01-0xFF   failure; the SOCKS Server must then close the TCP connection with the SOCKS Client
                    connect
*/
