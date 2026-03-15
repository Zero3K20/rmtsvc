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
#define SOCKSERR_PROXY_CONNECTIONS SOCKSERR_PROXY_AUTH-6 //connect数太多

#define PROXY_MAX_RESPTIMEOUT 10 //s maximum response delay
#define PROXY_SERVER_PORT	1080 //defaultproxy service的port

typedef enum //代理typedefine
{
	PROXY_NONE=0,//不使用代理
	PROXY_HTTPS=1,
	PROXY_SOCKS4=2,
	PROXY_SOCKS5=4 
}PROXYTYPE; 

typedef enum //代理请求typedefine
{
	PROXYREQ_TCP,
	PROXYREQ_BIND,//
	PROXYREQ_UDP //仅仅socks5代理协议支持UDP代理
}PROXYREQTYPE;

typedef struct _proxyaccount //proxyaccountinfo
{
	std::string m_username;//account,account不区分size写(account转换为小写)
	std::string m_userpwd;//password,如果password==""则，无需password verification
	net4cpp21::iprules m_ipRules;//源IP access rules

	unsigned long m_maxratio;//最大带宽 K/s,如果=0则不限
	long m_loginusers;//current以此account登录proxy服务的用户个数,只有没用用户connect时才能delete此account
	long m_maxLoginusers;//限制此account的最大同时登录用户数,<=0则不限制 
	time_t m_limitedTime;//限制此account只在某个date之前有效，==0不限制
	
	net4cpp21::iprules m_dstRules;//目的IP access rules
}PROXYACCOUNT;

//设定1字节对齐方式
//#ifdef WIN32
//	#pragma pack(push)
//	#pragma pack(1)
//#endif

	//sock4请求应答结构
	typedef struct _sock4req
	{
	char VN; //version ,must be 4
	char CD; //socks4 command, 1--connect 2--bind
	unsigned short Port;
	unsigned long IPAddr; //socks4代理协议not supported服务端域名parse
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
	//CONNECT响应包中只有VN、CD字段有意义，DSTPORT、DSTIP字段被忽略
	//******************************
	//sock5代理请求应答结构
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
	char other[1]; // depends on address type， x字节
	//unsigned short port; // should be in BIG ENDIAN format!
	}sock5req1;
	typedef struct _sock5ans1
	{
	char Ver;
	char Rep;
	char Rsv;// must be 0
	char Atyp;// 1=4 byte IP address, 3=domain name, 4=6 byte IP address
	//char other[1]; // depends on address type， x字节
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

	//UDP代理包结构
	typedef struct _socks5udp
	{
		unsigned short Rsv;  //must be 0
		char Frag; //是否data报分段重组flag
		char Atyp;// 1=4 byte IP address, 3=domain name, 4=6 byte IP address
		unsigned long IPAddr;
		unsigned short Port;
	}socks5udp;
//#ifdef WIN32
//	#pragma pack(pop)
//#endif

#endif

/*   RFC SOCKS4
SOCKS协议最初由David Koblas设计，后经Ying-Da Lee改进成SOCKS 4。

SOCKS 4只支持TCP转发。

请求报文format如下:

+----+----+----+----+----+----+----+----+----+----+...+----+
| VN | CD | DSTPORT |      DSTIP        | USERID      |NULL|
+----+----+----+----+----+----+----+----+----+----+...+----+
   1    1      2              4           variable       1

VN      SOCKS协议version number，应该是0x04

CD      SOCKS命令，可取如下值:

        0x01    CONNECT
        0x02    BIND

DSTPORT CD相关的portinfo

DSTIP   CD相关的addressinfo

USERID  客户方的USERID

NULL    0x00

响应报文format如下:

+----+----+----+----+----+----+----+----+
| VN | CD | DSTPORT |      DSTIP        |
+----+----+----+----+----+----+----+----+
   1    1      2              4

VN      应该为0x00而不是0x04

CD      可取如下值:

        0x5A    允许转发
        0x5B    拒绝转发，一般性failure
        0x5C    拒绝转发，SOCKS 4 Server无法connect到SOCS 4 Client所在主机的
                IDENT服务
        0x5D    拒绝转发，请求报文中的USERID与IDENT服务返回值不相符

DSTPORT CD相关的portinfo

DSTIP   CD相关的addressinfo

1) CONNECT命令

对于CONNECT请求，DSTIP/DSTPORT指明转发目的地。

SOCKS 4 Server根据源IP、DSTPORT、DSTIP、USERID以及可从SOCS 4 Client所在主
机的IDENT服务(RFC 1413)获取的info进行综合评估，以决定建立相应connect还是拒绝
转发。

假设CONNECT请求被允许，SOCKS 4 Server试图建立到转发目的地的TCPconnect，然后向
SOCKS 4 Clientsend response报文，指明是否success建立转发connect。

如果CONNECT请求被拒绝，SOCKS 4 Server也向SOCKS 4 Clientsend response报文，随后
立即关闭connect。

CONNECT响应包中只有VN、CD字段有意义，DSTPORT、DSTIP字段被忽略。如果CD等于
0x5A，表示success建立转发connect，之后SOCKS 4 Client直接在currentTCPconnect上send待转
发data。

2) BIND命令

FTP协议在某些情况下要求FTP Server主动建立到FTP Client的connect，即FTPdata流。

FTP Client - SOCKS 4 Client - SOCKS 4 Server - FTP Server

a. FTP Client试图建立FTP控制流。SOCKS 4 Client向SOCKS 4 ServersendCONNECT
   请求，后者响应请求，最终FTP控制流建立。

   CONNECT请求包中指明FTPSERVER.ADDR/FTPSERVER.PORT。

b. FTP Client试图建立FTPdata流。SOCKS 4 Client建立新的到SOCKS 4 Server的
   TCPconnect，并在新的TCPconnect上sendBIND请求。

   BIND请求包中仍然指明FTPSERVER.ADDR/FTPSERVER.PORT。

   SOCKS 4 Server收到BIND请求，根据这两个info以及USERID对BIND请求进行评估。
   create新套接字，侦听在AddrA/PortA上，并向SOCKS 4 Clientsend第一个BIND响应
   包。

   BIND响应包中CD不等于0x5A时表示failure，包中DSTPORT、DSTIP字段被忽略。

   BIND响应包中CD等于0x5A时，包中DSTIP/DSTPORT对应AddrA/PortA。如果DSTIP等
   于0(INADDR_ANY)，SOCKS 4 Client应将其替换成SOCKS 4 Server的IP，当SOCKS
   4 Server非多目(multi-homed)主机时就可能出现这种情况。

c. SOCKS 4 Client收到第一个BIND响应包。

   FTP Client调用getsockname(不是getpeername)获取AddrA/PortA，通过FTP控制
   流向FTP ServersendPORT命令，通知FTP Server应该主动建立到AddrA/PortA的
   TCPconnect。

d. FTP Server收到PORT命令，主动建立到AddrA/PortA的TCPconnect，假设TCPconnect相关
   四元组是:

   AddrB，PortB，AddrA，PortA

e. SOCKS 4 Server收到来自FTP Server的TCPconnect请求，检查这条入connect的源IP(
   AddrB)是否与FTPSERVER.ADDR匹配，然后向SOCKS 4 Clientsend第二个BIND响应
   包。

   源IP不匹配时第二个BIND响应包中CD字段设为0x5B，然后SOCKS 4 Server关闭这
   条用于send第二个BIND响应包的TCPconnect，同时关闭与FTP Server之间的TCPconnect，
   但主TCPconnect(与CONNECT请求相关的那条TCPconnect)继续保持中。

   源IP匹配时CD字段设为0x5A。然后SOCKS 4 Serverstart转发FTPdata流。

   无论如何，第二个BIND响应包中DSTPORT、DSTIP字段被忽略。

对于CONNECT、BIND请求，SOCKS 4 Server有一个定时器(currentCSTCimplementation采用两分钟)。
假设定时器timeout，而SOCKS 4 Server与Application Server之间的TCPconnect(出connect或
入connect)仍未建立，SOCKS 4 Server将关闭与SOCKS 4 Client之间相应的TCPconnect并放
弃相应的转发。
*/

/*       RFC1928 socks5
SOCKS协议位于传输层(TCP/UDP等)与应用层之间，因而显然地位于网络层(IP)之上。
诸如IP层报文转发、ICMP协议等等都因太低层而与SOCKS协议无关。

SOCKS 4not supported认证、UDP协议以及remoteparseFQDN。SOCKS 5支持。

SOCKS Server缺省侦听在1080/TCP口。这是SOCKS Clientconnect到SOCKS Server之后发
送的第一个报文:

+----+----------+----------+
|VER | NMETHODS | METHODS  |
+----+----------+----------+
| 1  |    1     | 1 to 255 |
+----+----------+----------+

对于SOCKS 5，VER字段为0x05，version4对应0x04。NMETHODS字段specifiedMETHODS域的字节
数。不知NMETHODS可以为0否，看上图所示，可取值[1,255]。METHODS字段有多少字
节(假设不重复)，就意味着SOCKS Client支持多少种认证机制。

SOCKS Server从METHODS字段中选中一个字节(一种认证机制)，并向SOCKS Client发
送响应报文:

+----+--------+
|VER | METHOD |
+----+--------+
| 1  |   1    |
+----+--------+

目前可用METHOD值有:

0x00        NO AUTHENTICATION REQUIRED(无需认证)
0x01        GSSAPI
0x02        USERNAME/PASSWORD(username/口令认证机制)
0x03-0x7F   IANA ASSIGNED
0x80-0xFE   RESERVED FOR PRIVATE METHODS(私有认证机制)
0xFF        NO ACCEPTABLE METHODS(完全不兼容)

如果SOCKS Server响应以0xFF，表示SOCKS Server与SOCKS Client完全不兼容，
SOCKS Client必须关闭TCPconnect。认证机制协商完成后，SOCKS Client与
SOCKS Server进行认证机制相关的子协商，参看其它文档。为保持最广泛兼容性，
SOCKS Client、SOCKS Server必须支持0x01，同时应该支持0x02。

认证机制相关的子协商完成后，SOCKS Client提交转发请求:

+----+-----+-------+------+----------+----------+
|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
+----+-----+-------+------+----------+----------+
| 1  |  1  | X'00' |  1   | Variable |    2     |
+----+-----+-------+------+----------+----------+

VER         对于version5这里是0x05

CMD         可取如下值:

            0x01    CONNECT
            0x02    BIND
            0x03    UDP ASSOCIATE

RSV         保留字段，必须为0x00

ATYP        用于指明DST.ADDR域的type，可取如下值:

            0x01    IPv4address
            0x03    FQDN(全称域名)
            0x04    IPv6address

DST.ADDR    CMD相关的addressinfo，不要为DST所迷惑

            如果是IPv4address，这里是big-endian序的4字节data

            如果是FQDN，比如"www.nsfocus.net"，这里将是:

            0F 77 77 77 2E 6E 73 66 6F 63 75 73 2E 6E 65 74

            注意，没有结尾的NUL字符，非ASCIZ串，第一字节是length域

            如果是IPv6address，这里是16字节data。

DST.PORT    CMD相关的portinfo，big-endian序的2字节data

SOCKS Server评估来自SOCKS Client的转发请求并send response报文:

+----+-----+-------+------+----------+----------+
|VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
+----+-----+-------+------+----------+----------+
| 1  |  1  | X'00' |  1   | Variable |    2     |
+----+-----+-------+------+----------+----------+

VER         对于version5这里是0x05

REP         可取如下值:

            0x00        success
            0x01        一般性failure
            0x02        规则不允许转发
            0x03        网络不可达
            0x04        主机不可达
            0x05        connect拒绝
            0x06        TTLtimeout
            0x07        not supported请求包中的CMD
            0x08        not supported请求包中的ATYP
            0x09-0xFF   unassigned

RSV         保留字段，必须为0x00

ATYP        用于指明BND.ADDR域的type

BND.ADDR    CMD相关的addressinfo，不要为BND所迷惑

BND.PORT    CMD相关的portinfo，big-endian序的2字节data

1) CONNECT命令

假设CMD为CONNECT，SOCKS Client、SOCKS Server之间通信的相关四元组是:

SOCKSCLIENT.ADDR，SOCKSCLIENT.PORT，SOCKSSERVER.ADDR，SOCKSSERVER.PORT

一般SOCKSSERVER.PORT是1080/TCP。

CONNECT请求包中的DST.ADDR/DST.PORT指明转发目的地。SOCKS Server可以靠
DST.ADDR、DST.PORT、SOCKSCLIENT.ADDR、SOCKSCLIENT.PORT进行评估，以决定建立
到转发目的地的TCPconnect还是拒绝转发。

假设规则允许转发并且success建立到转发目的地的TCPconnect，相关四元组是:

BND.ADDR，BND.PORT，DST.ADDR，DST.PORT

此时SOCKS Server向SOCKS Clientsend的CONNECT响应包中将指明BND.ADDR/BND.PORT。
注意，BND.ADDR可能不同于SOCKSSERVER.ADDR，SOCKS Server所在主机可能是多目(
multi-homed)主机。

假设拒绝转发或未能success建立到转发目的地的TCPconnect，CONNECT响应包中REP字段将
指明具体原因。

响应包中REP非零时表示failure，SOCKS Server必须在send response包后不久(不超过10s)关
闭与SOCKS Client之间的TCPconnect。

响应包中REP为零时表示success。之后SOCKS Client直接在currentTCPconnect上send待转发数
据。

2) BIND命令

假设CMD为BIND。这多用于FTP协议，FTP协议在某些情况下要求FTP Server主动建立
到FTP Client的connect，即FTPdata流。

FTP Client - SOCKS Client - SOCKS Server - FTP Server

a. FTP Client试图建立FTP控制流。SOCKS Client向SOCKS ServersendCONNECT请求，
   后者响应请求，最终FTP控制流建立。

   CONNECT请求包中指明FTPSERVER.ADDR/FTPSERVER.PORT。

b. FTP Client试图建立FTPdata流。SOCKS Client建立新的到SOCKS Server的TCP连
   接，并在新的TCPconnect上sendBIND请求。

   BIND请求包中仍然指明FTPSERVER.ADDR/FTPSERVER.PORT。SOCKS Server应该据此
   进行评估。

   SOCKS Server收到BIND请求，create新套接字，侦听在AddrA/PortA上，并向SOCKS
   Clientsend第一个BIND响应包，包中BND.ADDR/BND.PORT即AddrA/PortA。

c. SOCKS Client收到第一个BIND响应包。FTP Client通过FTP控制流向FTP Server发
   送PORT命令，通知FTP Server应该主动建立到AddrA/PortA的TCPconnect。

d. FTP Server收到PORT命令，主动建立到AddrA/PortA的TCPconnect，假设TCPconnect相关
   四元组是:

   AddrB，PortB，AddrA，PortA

e. SOCKS Server收到来自FTP Server的TCPconnect请求，向SOCKS Clientsend第二个
   BIND响应包，包中BND.ADDR/BND.PORT即AddrB/PortB。然后SOCKS Serverstart转
   发FTPdata流。

下面是一些讨论记录:

scz

为什么需要send第二个BIND响应包，指明AddrB/PortB的意义何在。

knightmare@apue

指明AddrB/PortB的意义在于，FTP Client出于安全考虑，会检查FTPdata流的源IP、
源port，比如FTPdata流的源端只允许是FTPSERVER.ADDR/20。

scz

knightmare的答案是正确的，但我的疑惑可能源于我对SOCKS协议的error理解，以至
提出一个产生歧义的问题。事实上应该查看David Koblas的原始文档以理解BIND请求
的全过程。前面关于FTPdata流的description部分已做了修正，因此看不出提问的缘由了。

3) UDP ASSOCIATE命令

假设CMD为UDP ASSOCIATE。此时DST.ADDR与DST.PORT指明sendUDP报文时的源IP、源
port，而不是UDP转发目的地，SOCKS Server可以据此进行评估以决定是否进行UDP转
发。如果SOCKS ClientsendUDP ASSOCIATE命令时无法提供DST.ADDR与DST.PORT，则
必须将这两个域置零。

下面是一些讨论记录:

scz

什么情况下SOCKS ClientsendUDP ASSOCIATE命令，又无法提供DST.ADDR与DST.PORT，
或者说出于什么考虑才需要刻意将这两个域置零。有现实例子存在吗。

shixudong@163.com

考虑这种情况:

Application Client - SOCKS Client - NAT - SOCKS Server - Application Server

SOCKS Client在UDP ASSOCIATE命令中指明DST.ADDR/DST.PORT，SOCKS Server靠这些
info决定是否转发某个UDP报文。上图中SOCKS Client与SOCKS Server之间有NAT，前
者无法预知UDP报文经过NAT后源IP、源port会变成什么样，但肯定会变，因此前者无
法提前在UDP ASSOCIATE命令中指明DST.ADDR/DST.PORT，如果强行specified非零值，后者
会检测到待转发UDP报文的源IP、源port与DST.ADDR/DST.PORT不匹配而拒绝转发。针
对这种情况，RFC 1928建议SOCKS Client将DST.ADDR/DST.PORT置零，SOCKS Server
此时不再检查待转发UDP报文的源IP、源port。

在一条TCPconnect上SOCKS Client向SOCKS Serversend了UDP ASSOCIATE命令，后续UDP
转发要求此TCPconnect继续维持，此TCPconnect关闭时相应的UDP转发也将中止。换句话说，
UDP转发必然伴随着一个TCPconnect，这将消耗额外的资源。

SOCKS Server向SOCKS ClientsendUDP ASSOCIATE响应包，BND.ADDR/BND.PORT指明
SOCKS Client应向哪里send待转发UDP报文。

对于UDP转发，SOCKS Clientsend出去的UDPdata区如下:

+----+------+------+----------+----------+----------+
|RSV | FRAG | ATYP | DST.ADDR | DST.PORT |   DATA   |
+----+------+------+----------+----------+----------+
| 2  |  1   |  1   | Variable |    2     | Variable |
+----+------+------+----------+----------+----------+

RSV         保留字段，必须为0x0000

FRAG        Current fragment number

            0x00        这是一个非碎片的SOCKS UDP报文
            0x01-0x7F   SOCKS碎片序号
            0x80-0xFF   最高位置1表示碎片序列end，即这是最后一个SOCKS碎片

ATYP        用于指明DST.ADDR域的type，可取如下值:

            0x01    IPv4address
            0x03    FQDN(全称域名)
            0x04    IPv6address

DST.ADDR    转发目标address

DST.PORT    转发目标port

DATA        原始UDPdata区

SOCKS Server静静地为SOCKS Client进行UDP转发，并不通知后者转发完成还是被拒
绝。

FRAG用于支持SOCKS碎片。SOCKS碎片receive方一般implementation有重组队列与重组定时器。假设
重组定时器timeout或者低序SOCKS碎片后于高序SOCKS碎片到达重组队列，此时必须重置
重组队列。重组定时器不得小于5 seconds。应该尽可能地避免出现SOCKS碎片。

是否支持SOCKS碎片是可选的，如果一个SOCKSimplementationnot supportedSOCKS碎片，则必须丢弃所
有receive到的SOCKS碎片，即那些FRAG字段非零的SOCKS UDP报文。

由于SOCKSimplementation在支持UDP转发时会在原始UDPdata区前增加一个SOCKS协议相关的头，
因此为UDPdata区分配空间时要为这个头留足空间:

ATYP    头占用字节  原因
0x01    10          IPv4address占4字节，4+6=10
0x03    262         length域是一个字节，因此最大0xFF，1+255+6=262
0x04    20          这里我怀疑是笔误，IPv6address占16字节，16+6=22

我怀疑RFC 1928这里有笔误，写信询问mleech@bnr.ca、ietf-web@ietf.org去了。
*/
/*     RFC 1929
假设SOCKS V5 Client/Server协商采用username/口令认证机制(0x02)，现在start相应
子协商。

clientsend如下报文:

+----+------+----------+------+----------+
|VER | ULEN |  UNAME   | PLEN |  PASSWD  |
+----+------+----------+------+----------+
| 1  |  1   | 1 to 255 |  1   | 1 to 255 |
+----+------+----------+------+----------+

VER     子协商的currentversion，目前是0x01

ULEN    UNAME字段的length

UNAME   username

PLEN    PASSWD字段的length

PASSWD  口令，注意是明文传输的

服务端authentication后send response报文如下:

+----+--------+
|VER | STATUS |
+----+--------+
| 1  |   1    |
+----+--------+

VER     子协商的currentversion，目前是0x01

STATUS  可取如下值:

        0x00        success
        0x01-0xFF   failure，随后SOCKS Server必须关闭与SOCKS Client之间的TCP
                    connect
*/
