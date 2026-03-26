/*******************************************************************
   *	ipdef.h
   *    DESCRIPTION:constants, structures and enum definitions for the IP protocol
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_IPDEF_H__
#define __YY_IPDEF_H__

#ifndef IPPROTO_IP

#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* internet group management protocol */
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */

#endif

#define IP_MAX_PACKAGE_SIZE 1500

#define tOptionType unsigned char
typedef struct _IPOption
{
	tOptionType		OptionType;
	unsigned char	OptionLength;
	unsigned char	OptionData;
} IPOption;

//IP Options flags (1bit)
#define IPOption_COPY 128
#define IPOption_DONT_COPY 0

//IP Options class (2 bits)
#define IPOption_CONTROL 0
#define IPOption_RESERVED 2
#define IPOption_DEBUGGING 64
#define IPOption_RESERVED2 6

//IP options type
/*The Type of Service provides an indication of the abstract
  parameters of the quality of service desired.  These parameters are
  to be used to guide the selection of the actual service parameters
  when transmitting a datagram through a particular network.  Several
  networks offer service precedence, which somehow treats high
  precedence traffic as more important than other traffic (generally
  by accepting only traffic above a certain precedence at time of high
  load).  The major choice is a three way tradeoff between low-delay,
  high-reliability, and high-throughput.

  The use of the Delay, Throughput, and Reliability indications may
  increase the cost (in some sense) of the service.  In many networks
  better performance for one of these parameters is coupled with worse
  performance on another.  Except for very unusual cases at most two
  of these three indications should be set.

  The type of service is used to specify the treatment of the datagram
  during its transmission through the internet system.  Example
  mappings of the internet type of service to the actual service
  provided on networks such as AUTODIN II, ARPANET, SATNET, and PRNET
  is given in "Service Mappings" [8].

  The Network Control precedence designation is intended to be used
  within a network only.  The actual use and control of that
  designation is up to each network. The Internetwork Control
  designation is intended for use by gateway control originators only.
  If the actual use of these precedence designations is of concern to
  a particular network, it is the responsibility of that network to
  control the access to, and use of, those precedence designations.*/

#define IPOption_END_OPTION 0 //End of option list
#define IPOption_NO_OPERATION 1 //Do nothing
#define IPOption_SECURITY 2 //Security information
#define IPOption_LOOSE_ROUTING 3 //Loose routing options
#define IPOption_STRICT_ROUTING 9 //Strict source routing
#define IPOption_RECORD_ROUTE 7 //Record route on datagram
#define IPOption_STREAM 8 //Used to carry stream identifier
#define IPOption_TIMESTAMP 4 //Internet timestamp

//IP options extensions - Security
/*Specifies one of 16 levels of security (eight of which are
  reserved for future use).
  Compartments (C field):  16 bits

  An all zero value is used when the information transmitted is
  not compartmented.  Other values for the compartments field
  may be obtained from the Defense Intelligence Agency.

  Handling Restrictions (H field):  16 bits

	The values for the control and release markings are
    alphanumeric digraphs and are defined in the Defense
    Intelligence Agency Manual DIAM 65-19, "Standard Security
    Markings".

  Transmission Control Code (TCC field):  24 bits

	Provides a means to segregate traffic and define controlled
    communities of interest among subscribers. The TCC values are
    trigraphs, and are available from HQ DCA Code 530.

    Must be copied on fragmentation.  This option appears at most
    once in a datagram.*/

#define IPOption_SECURITY_LENGTH 11

#define IPOption_SECURITY_UNCLASSIFIED 0
#define IPOption_SECURITY_CONFIDENTIAL 0x1111000100110101b
#define IPOption_SECURITY_EFTO 0x0111100010011010b
#define IPOption_SECURITY_MMMM 0x1011110001001101b
#define IPOption_SECURITY_PROG 0x0101111000100110b
#define IPOption_SECURITY_RESTRICTED 0x1010111100010011b
#define IPOption_SECURITY_SECRET 0x1101011110001000b
#define IPOption_SECURITY_TOPSECRET 0x0110101111000101b
#define IPOption_SECURITY_RESERVED1 0x0011010111100010b
#define IPOption_SECURITY_RESERVED2 0x1001101011110001b 
#define IPOption_SECURITY_RESERVED3 0x0100110101111000b
#define IPOption_SECURITY_RESERVED4 0x0010010010111101b
#define IPOption_SECURITY_RESERVED5 0x0001001101011110b
#define IPOption_SECURITY_RESERVED6 0x1000100110101111b
#define IPOption_SECURITY_RESERVED7 0x1100010011010110b
#define IPOption_SECURITY_RESERVED8 0x1110001001101011b

/*This option provides a way for the 16-bit SATNET stream
identifier to be carried through networks that do not support
the stream concept.

Must be copied on fragmentation.  Appears at most once in a
datagram.*/

//IP options extensions - Stream ID
#define IPOption_STREAM_LENGTH 4

/*The loose source and record route (LSRR) option provides a means
for the source of an internet datagram to supply routing
information to be used by the gateways in forwarding the
datagram to the destination, and to record the route
information.

The option begins with the option type code.  The second octet
is the option length which includes the option type code and the
length octet, the pointer octet, and length-3 octets of route
data.  The third octet is the pointer into the route data
indicating the octet which begins the next source address to be
processed.  The pointer is relative to this option, and the
smallest legal value for the pointer is 4.

A route data is composed of a series of internet addresses.
Each internet address is 32 bits or 4 octets.  If the pointer is
greater than the length, the source route is empty (and the
recorded route full) and the routing is to be based on the
destination address field.

If the address in destination address field has been reached and
the pointer is not greater than the length, the next address in
the source route replaces the address in the destination address
field, and the recorded route address replaces the source
address just used, and pointer is increased by four.

The recorded route address is the internet module's own internet
address as known in the environment into which this datagram is
being forwarded.

This procedure of replacing the source route with the recorded
route (though it is in the reverse of the order it must be in to
be used as a source route) means the option (and the IP header
as a whole) remains a constant length as the datagram progresses
through the internet.

This option is a loose source route because the gateway or host
IP is allowed to use any route of any number of other
intermediate gateways to reach the next address in the route.

Must be copied on fragmentation.  Appears at most once in a
datagram.*/

/*The strict source and record route (SSRR) option provides a
means for the source of an internet datagram to supply routing
information to be used by the gateways in forwarding the
datagram to the destination, and to record the route
information.

The option begins with the option type code.  The second octet
is the option length which includes the option type code and the
length octet, the pointer octet, and length-3 octets of route
data.  The third octet is the pointer into the route data
indicating the octet which begins the next source address to be
processed.  The pointer is relative to this option, and the
smallest legal value for the pointer is 4.

A route data is composed of a series of internet addresses.
Each internet address is 32 bits or 4 octets.  If the pointer is
greater than the length, the source route is empty (and the

recorded route full) and the routing is to be based on the
destination address field.

If the address in destination address field has been reached and
the pointer is not greater than the length, the next address in
the source route replaces the address in the destination address
field, and the recorded route address replaces the source
address just used, and pointer is increased by four.

The recorded route address is the internet module's own internet
address as known in the environment into which this datagram is
being forwarded.

This procedure of replacing the source route with the recorded
route (though it is in the reverse of the order it must be in to
be used as a source route) means the option (and the IP header
as a whole) remains a constant length as the datagram progresses
through the internet.

This option is a strict source route because the gateway or host
IP must send the datagram directly to the next address in the
source route through only the directly connected network
indicated in the next address to reach the next gateway or host
specified in the route.

Must be copied on fragmentation.  Appears at most once in a
datagram.*/

//IP options extensions - Strict routing
#define IPOption_STRICT_ROUTING_LENGTH 3
#define IPOption_STRICT_ROUTING_POINTER 4

/*The Timestamp is a right-justified, 32-bit timestamp in
milliseconds since midnight UT.  If the time is not available in
milliseconds or cannot be provided with respect to midnight UT
then any time may be inserted as a timestamp provided the high
order bit of the timestamp field is set to one to indicate the
use of a non-standard value.

The originating host must compose this option with a large
enough timestamp data area to hold all the timestamp information
expected.  The size of the option does not change due to adding

timestamps.  The intitial contents of the timestamp data area
must be zero or internet address/zero pairs.

If the timestamp data area is already full (the pointer exceeds
the length) the datagram is forwarded without inserting the
timestamp, but the overflow count is incremented by one.

If there is some room but not enough room for a full timestamp
to be inserted, or the overflow count itself overflows, the
original datagram is considered to be in error and is discarded.
In either case an ICMP parameter problem message may be sent to
the source host [3].

The timestamp option is not copied upon fragmentation.  It is
carried in the first fragment.  Appears at most once in a
datagram.*/

//IP options extensions - Time Stamp
#define IPOption_TIMESTAMP_LENGTH 5

#define IPOption_TIMESTAMP_ONLY 0
#define IPOption_TIMESTAMP_EACH 1
#define IPOption_TIMESTAMP_PRE 2

#define IPOption_TIMESTAMP_SIZE 8

//IP packet structure
/* 0                   1                   2                   3   
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
   7Bit------>0Bit
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Identification        |Flags|      Fragment Offset    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Time to Live |    Protocol   |         Header Checksum       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Source Address                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Destination Address                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                    Example Internet Datagram Header        
*/
//Some IP constants
//Version
#define IpVersion 4
//Service types
#define IpService_NETWORK_CONTROL 111
#define IpService_INTERNETWORK_CONTROL 110
#define IpService_CRITIC_ECP 101
#define IpService_FLASH_OVERIDE 100
#define IpService_FLASH 011
#define IpService_IMMEDIATE 010
#define IpService_PRIORITY 001
#define IpService_ROUTINE 0
//Fragmetation flag
#define IpFragFlag_MAY_FRAG 0x0000
#define IpFragFlag_MORE_FRAG 0x2000
#define IpFragFlag_LAST_FRAG 0x5000
#define IpFragFlag_DONT_FRAG 0x4000
#define IP_DEF_TTL 128 //default TTL value
#define IPOption_WRAPSIZE 4
#define IPOption_SIZE 40
#define IPOption_MAX_ROUTES 10
#define IpV4_Min_Header_Length 20
typedef struct _IpV4Header //IP message header defined according to the IPv4 protocol
{
	unsigned char IHL_Version;//unsigned char  IHL:4,Version:4; // 4-bit header length + 4-bit IP version number
	unsigned char  TypeOfService;//8-bit service type TOS
	unsigned short TotalLength;//16-bit total length (bytes)
	unsigned short Identification;//identifier
	unsigned short Frag_and_flags;//unsigned short FragmentOffset:13,Flags:3;
	unsigned char  TimeToLive;//time to live TTL
	unsigned char  Protocol;
	unsigned short HeaderChecksum;//IP header checksum
	unsigned long   SourceAddress;
	unsigned long   DestinationAddress;
	
	unsigned short get_Flags()
	{
		unsigned short ret=(Frag_and_flags & 0xe000)>>13;
		return ret;
	}
	void set_Flags(unsigned short v)
	{
		Frag_and_flags=Frag_and_flags|((v & 0x0007)<<13);
	}
	unsigned short get_FragmentOffset()
	{
		return (Frag_and_flags & 0x1fff);
	}
	void set_FragmentOffset(unsigned short v)
	{
		Frag_and_flags=Frag_and_flags|(v & 0x1fff);
	}
	unsigned char get_IpHeaderLen()
	{
		return (IHL_Version & 0x0f);
	}
	void set_IpHeaderLen(unsigned char v)
	{
		IHL_Version=IHL_Version|(v & 0x0f);
	}
	unsigned char get_Version()
	{
		return (IHL_Version & 0xf0)>>4;
	}
	void set_Version(unsigned char v)
	{
		IHL_Version=IHL_Version|((v & 0x0f)<<4);
	}
}IpV4Header,*LPIpV4Header;

//TCP data packet structure
/*
    0                   1                   2                   3   
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
    7Bit----->0Bit
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                            TCP Header Format
*/ 
/*
A TCP data packet includes a TCP header, followed by options and data. A TCP header contains 6 flag bits. Their meanings are:

   SYN: flag bit used to establish a connection, letting both sides synchronize sequence numbers. If SYN=1 and ACK=0, the packet is a connection request; if SYN=1 and ACK=1, it means the connection is accepted.

   FIN: indicates that the sender has no more data to transfer and wishes to release the connection.

   RST: used to reset a connection. A packet with the RST flag set is called a reset packet. Generally, if TCP receives a fragment that clearly does not belong to any connection on this host, it sends a reset packet to the remote end.

   URG: urgent data flag. If set to 1, the packet contains urgent data, and the urgent data pointer is valid.

   ACK: confirmation flag bit. If set to 1, the acknowledgment number in the packet is valid; otherwise, the acknowledgment number is invalid.

   PSH: if set, the receiver should pass the data to the application layer as soon as possible.

TCP connection establishment

   TCP is a connection-oriented reliable transfer protocol. Connection-oriented means that two application endpoints must establish a TCP connection before using TCP to transfer data. TCP's reliability is provided through checksums, timers, data sequence numbers, and acknowledgments. By assigning a sequence number to each sent byte and having the receiver send an acknowledgment after receiving data, TCP guarantees reliable data transfer. Sequence numbers are used to ensure data ordering and eliminate duplicate data. In a TCP session, there are two data streams (each connection endpoint receives data from and sends data to the other), so when establishing a connection, an ISN (Initial Sequence Number) must be allocated for each data stream. To understand the implementation process, assuming client C wants to establish a connection with server S, we analyze the connection establishment process (commonly called the three-way handshake):

   1: C --SYN XX --> S

   2: C <-- SYN YY/ACK XX+1 --- S

   3: C ---ACK YY+1 --> S

   1: C sends a TCP packet (SYN request) to S, with the SYN (synchronize sequence number) flag set. The SYN request specifies the server port number the client wants to connect to and the client's ISN (XX is an example).

   2: The server sends back a response containing its own SYN info ISN (YY) and an acknowledgment of C's SYN, returning the next expected byte sequence number (YY+1).

   3: C acknowledges the SYN from S; data transfer begins.

   Implementation details

   Most TCP/IP implementations follow these principles:

   1: when a SYN or FIN packet arrives at a closed port, TCP discards the packet and sends a RST packet.

   2: when a RST packet arrives at a listening port, the RST is discarded.

   3: when a RST packet arrives at a closed port, the RST is discarded.

   4: when a packet containing ACK arrives at a listening port, the packet is discarded and a RST packet is sent.

   5: when a packet with SYN bit cleared arrives at a listening port, the packet is discarded.

   6: when a SYN packet arrives at a listening port, the normal three-way handshake continues; a SYN ACK packet is sent in reply.

   7: when a FIN packet arrives at a listening port, the packet is discarded. "FIN behavior" (closed port returns RST, listening port discards the packet) also occurs when URG and PSH flag bits are set. All TCP packets with URG, PSH, FIN flags, or with no flags at all, will trigger "FIN behavior".

*/
const int Tcp_Min_Header_Length = 20;
typedef struct _TcpHeader 
{
	unsigned short SourcePort;//source port
	unsigned short DestinationPort;//destination port
	unsigned long   SequenceNumber;//32-bit sequence number
	unsigned long   AcknowledgementNumber;//32-bit acknowledgment number
	//unsigned short FIN:1,SYN:1,RST:1,PSH:1,ACK:1,URG:1,Reserved:6,DataOffset:4;
	unsigned short th_flag_res_offset;
	unsigned short Window;
	unsigned short Checksum;
	unsigned short UrgentPointer;

	unsigned short get_FIN()
	{
		return (th_flag_res_offset & 0x0100)>>8;
	}
	void set_FIN(unsigned short v)
	{
		th_flag_res_offset=th_flag_res_offset|((v & 0x0001)<<8);
	}
	unsigned short get_SYN()
	{
		return (th_flag_res_offset & 0x0200)>>9;
	}
	void set_SYN(unsigned short v)
	{
		th_flag_res_offset=th_flag_res_offset|((v & 0x0001)<<9);
	}
	unsigned short get_RST()
	{
		return (th_flag_res_offset & 0x0400)>>10;
	}
	void set_RST(unsigned short v)
	{
		th_flag_res_offset=th_flag_res_offset|((v & 0x0001)<<10);
	}
	unsigned short get_PSH()
	{
		return (th_flag_res_offset & 0x0800)>>11;
	}
	void set_PSH(unsigned short v)
	{
		th_flag_res_offset=th_flag_res_offset|((v & 0x0001)<<11);
	}
	unsigned short get_ACK()
	{
		return (th_flag_res_offset & 0x1000)>>12;
	}
	void set_ACK(unsigned short v)
	{
		th_flag_res_offset=th_flag_res_offset|((v & 0x0001)<<12);
	}
	unsigned short get_URG()
	{
		return (th_flag_res_offset & 0x2000)>>13;
	}
	void set_URG(unsigned short v)
	{
		th_flag_res_offset=th_flag_res_offset|((v & 0x0001)<<13);
	}
	unsigned short get_DataOffset()
	{
		return (th_flag_res_offset & 0x00f0)>>4;
	}
	void set_DataOffset(unsigned short v)
	{
		th_flag_res_offset=th_flag_res_offset|((v & 0x000f)<<4);
	}
}TcpHeader,*LPTcpHeader; 

//UDP data packet structure
/*               0      7 8     15 16    23 24    31  
                 +--------+--------+--------+--------+ 
                 |     Source      |   Destination   | 
                 |      Port       |      Port       | 
                 +--------+--------+--------+--------+ 
                 |                 |                 | 
                 |     Length      |    Checksum     | 
                 +--------+--------+--------+--------+ 
                 |                                     
                 |          data octets ...            
                 +---------------- ...                 
*/
const int Udp_Min_Header_Length = 8;
typedef struct _UdpHeader 
{
	unsigned short  SourcePort;
	unsigned short  DestinationPort;
	unsigned short  Length;
	unsigned short  Checksum;
}UdpHeader,*LPUdpHeader; 

#endif
