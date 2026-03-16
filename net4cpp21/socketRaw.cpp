/*******************************************************************
   *	socketRaw.cpp
   *    DESCRIPTION:Raw socket class implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/
#ifdef WIN32 //Windows platform
/*
 * winsock2.h must be included before windows.h
 */
	#include <winsock2.h>
/*
 * ws2tcpip.h must be explicitly included after winsock2.h
 */
	#include <ws2tcpip.h>
/*
 * mstcpip.h must be explicitly included after winsock2.h
 */
	#include "mstcpip.h"
#elif defined MAC //Not yet supported
	//....
#else  //Unix/Linux platform
	//This header is only needed when setting the socket to promiscuous mode
	//On SUN Unix, if.h defines 'struct map' which conflicts with the STL map definition
	//This program does not use struct map, so it is renamed via a macro!!! 
	#define map unix_netmap
	#include <net/if.h>
	#undef map //map
	//SIOCSIFFLAGS is needed for promiscuous mode; Linux and Unix require different headers
	#include <sys/ioctl.h>
	//On Sun OS, include sys/sockio.h
	//On Linux, include sys/ioctl.h
	#ifndef SIOCSIFFLAGS
		    #include <sys/sockio.h>
	#endif
#endif

#include "include/sysconfig.h"
#include "include/socketRaw.h"
#include "include/cLogger.h"

using namespace std;
using namespace net4cpp21;

socketRaw :: socketRaw()
{
	m_TTL=IP_DEF_TTL;
	m_SourceAddress.assign(socketBase::getLocalHostIP());
	//Zero out
	memset((void *)&m_IpV4Header,0,sizeof(m_IpV4Header));
	memset((void *)&m_ProtocolHeader,0,sizeof(m_ProtocolHeader));
}

//Returns the actual user data size after successfully decoding an IPv4 packet
unsigned short socketRaw :: dataLen_ipv4()
{ 
	switch(m_IpV4Header.Protocol)
	{
		case IPPROTO_TCP://Get TCP header length
			{
			//Get IP header length
			char ipHeaderLen=m_IpV4Header.get_IpHeaderLen();
			//Get TCP header length
			char tcpHeaderLen=(char)m_ProtocolHeader.m_TcpHeader.get_DataOffset();
			return m_IpV4Header.TotalLength-(ipHeaderLen+tcpHeaderLen)*4;
			}
		case IPPROTO_UDP:
			return m_ProtocolHeader.m_UdpHeader.Length-8;
		default:
			//Get IP header length
			char ipHeaderLen=m_IpV4Header.get_IpHeaderLen();
			return m_IpV4Header.TotalLength-ipHeaderLen*4;
	}//?switch
	return 0;
}
//Decode an IPv4 protocol packet
//TCP/IP specifies a unified network byte order defined as big-endian: high byte first, low byte last
char * socketRaw :: decode_ipv4(char *buf,int len)
{
	char *ptr=buf;
	if(ptr!=NULL&&len>=IpV4_Min_Header_Length)
	{
		m_IpV4Header.IHL_Version=*ptr; ptr++;
		m_IpV4Header.TypeOfService=*ptr; ptr++;
		
		m_IpV4Header.TotalLength=*((unsigned short *)ptr); ptr+=2;
		m_IpV4Header.TotalLength=ntohs(m_IpV4Header.TotalLength);
		
		m_IpV4Header.Identification=*((unsigned short *)ptr); ptr+=2;
		m_IpV4Header.Identification=ntohs(m_IpV4Header.Identification);
		
		m_IpV4Header.Frag_and_flags=*((unsigned short *)ptr); ptr+=2;
		m_IpV4Header.Frag_and_flags=ntohs(m_IpV4Header.Frag_and_flags);
		
		m_IpV4Header.TimeToLive=*ptr; ptr++;
		
		m_IpV4Header.Protocol=*ptr; ptr++;
		
		m_IpV4Header.HeaderChecksum=*((unsigned short *)ptr); ptr+=2;
		m_IpV4Header.HeaderChecksum=ntohs(m_IpV4Header.HeaderChecksum);
		
		m_IpV4Header.SourceAddress=*((unsigned long *)ptr); ptr+=4;
		//m_IpV4Header.SourceAddress=ntohl(m_IpV4Header.SourceAddress);
		
		m_IpV4Header.DestinationAddress=*((unsigned long *)ptr); ptr+=4;
		//m_IpV4Header.DestinationAddress=ntohl(m_IpV4Header.DestinationAddress);
		
		//Get IP header length
		char ipHeaderLen=m_IpV4Header.get_IpHeaderLen();
		if(len>=m_IpV4Header.TotalLength)
		{
			//Check if the received data length is >= the IP packet length
			//Get the start address of the encapsulated protocol
			ptr=buf+ipHeaderLen*4;
			switch(m_IpV4Header.Protocol)
			{
				case IPPROTO_TCP:
				{
					m_ProtocolHeader.m_TcpHeader.SourcePort=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_TcpHeader.SourcePort=ntohs(m_ProtocolHeader.m_TcpHeader.SourcePort);
					
					m_ProtocolHeader.m_TcpHeader.DestinationPort=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_TcpHeader.DestinationPort=ntohs(m_ProtocolHeader.m_TcpHeader.DestinationPort);
					
					m_ProtocolHeader.m_TcpHeader.SequenceNumber=*((unsigned int *)ptr); ptr+=4;
					m_ProtocolHeader.m_TcpHeader.SequenceNumber=ntohl(m_ProtocolHeader.m_TcpHeader.SequenceNumber);
					
					m_ProtocolHeader.m_TcpHeader.AcknowledgementNumber=*((unsigned int *)ptr); ptr+=4;
					m_ProtocolHeader.m_TcpHeader.AcknowledgementNumber=ntohl(m_ProtocolHeader.m_TcpHeader.AcknowledgementNumber);
					
					m_ProtocolHeader.m_TcpHeader.th_flag_res_offset=*((unsigned short *)ptr); ptr+=2;
					//yyc remove: skip byte-order swap
//					m_ProtocolHeader.m_TcpHeader.th_flag_res_offset=ntohs(m_ProtocolHeader.m_TcpHeader.th_flag_res_offset);
					
					m_ProtocolHeader.m_TcpHeader.Window=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_TcpHeader.Window=ntohs(m_ProtocolHeader.m_TcpHeader.Window);
					
					m_ProtocolHeader.m_TcpHeader.Checksum=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_TcpHeader.Checksum=ntohs(m_ProtocolHeader.m_TcpHeader.Checksum);
					
					m_ProtocolHeader.m_TcpHeader.UrgentPointer=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_TcpHeader.UrgentPointer=ntohs(m_ProtocolHeader.m_TcpHeader.UrgentPointer);
					
					//Get TCP header length
					char tcpHeaderLen=(char)m_ProtocolHeader.m_TcpHeader.get_DataOffset();
					//Get start address of TCP payload; user data length = m_IpV4Header.TotalLength-(ipHeaderLen+tcpHeaderLen)*4
					ptr=buf+ipHeaderLen*4+tcpHeaderLen*4;
					return ptr;
				}
					break;
				case IPPROTO_UDP:
				{
					m_ProtocolHeader.m_UdpHeader.SourcePort=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_UdpHeader.SourcePort=ntohs(m_ProtocolHeader.m_UdpHeader.SourcePort);
					
					m_ProtocolHeader.m_UdpHeader.DestinationPort=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_UdpHeader.DestinationPort=ntohs(m_ProtocolHeader.m_UdpHeader.DestinationPort);
					
					m_ProtocolHeader.m_UdpHeader.Length=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_UdpHeader.Length=ntohs(m_ProtocolHeader.m_UdpHeader.Length);
					
					m_ProtocolHeader.m_UdpHeader.Checksum=*((unsigned short *)ptr); ptr+=2;
					m_ProtocolHeader.m_UdpHeader.Checksum=ntohs(m_ProtocolHeader.m_UdpHeader.Checksum);
					
					//Get the start address of UDP payload
					//ptr=ptr+0; //user data length = m_ProtocolHeader.m_UdpHeader.Length-8
					return ptr;
				}
					break;
				default:
					return ptr; //For other protocols, return the start of the protocol data directly
			}
		}//?if(len>=m_IpV4Header.TotalLength)
	}//?if(ptr!=NULL&&len>=IpV4_Min_Header_Length)
	return NULL;
}
//Encode user data into an IPv4 packet byte stream; returns the encoded stream size
//data --- pointer to user data; datalen --- user data length
//encodebuf --- buffer for the encoded IP packet byte stream; typically 4096 bytes is sufficient (IP packets <= 1500 bytes)
int socketRaw :: encode_ipv4(const char *data,int datalen,char *encodebuf)
{
	if(encodebuf==NULL) return 0;
	if(data==NULL) datalen=0;
	char *ptr=encodebuf;
	*ptr=m_IpV4Header.IHL_Version; ptr++;
	*ptr=m_IpV4Header.TypeOfService; ptr++;
	m_IpV4Header.TotalLength=0;
	*((unsigned short *)ptr)=htons(m_IpV4Header.TotalLength); ptr+=2;
	*((unsigned short *)ptr)=htons(m_IpV4Header.Identification); ptr+=2;
	*((unsigned short *)ptr)=htons(m_IpV4Header.Frag_and_flags); ptr+=2;
	*ptr=m_IpV4Header.TimeToLive; ptr++;
	*ptr=m_IpV4Header.Protocol; ptr++;
			
	m_IpV4Header.HeaderChecksum=0;
	*((unsigned short *)ptr)=htons(m_IpV4Header.HeaderChecksum); ptr+=2;	
	*((unsigned int *)ptr)=m_IpV4Header.SourceAddress; ptr+=4; //htonl(m_IpV4Header.SourceAddress); ptr+=4;
	*((unsigned int *)ptr)=m_IpV4Header.DestinationAddress; ptr+=4;//htonl(m_IpV4Header.DestinationAddress); ptr+=4;

	switch(m_IpV4Header.Protocol)
	{
		case IPPROTO_TCP:
		{
			char *pstart_tcp=ptr;//Pointer to the start of the TCP header
			//Set TCP header length to 5 (32-bit words)
			m_ProtocolHeader.m_TcpHeader.set_DataOffset(Tcp_Min_Header_Length/4);
			
			//Fill in pseudo-header (used for checksum calculation only, not actually sent)
			*((unsigned int *)ptr)=htonl(m_IpV4Header.SourceAddress);
			*((unsigned int *)(ptr+4))=htonl(m_IpV4Header.DestinationAddress);
			*(ptr+8)=0;
			*(ptr+9)=IPPROTO_TCP;//Protocol type
			*((unsigned short *)(ptr+10))=htons((m_ProtocolHeader.m_TcpHeader.th_flag_res_offset & 0x00f0)>>4);//TCP header length
			
			ptr+=12;//Pseudo-header is 12 bytes long
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_TcpHeader.SourcePort); ptr+=2;
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_TcpHeader.DestinationPort); ptr+=2;
			
			*((unsigned int *)ptr)=htonl(m_ProtocolHeader.m_TcpHeader.SequenceNumber); ptr+=4;
			*((unsigned int *)ptr)=htonl(m_ProtocolHeader.m_TcpHeader.AcknowledgementNumber); ptr+=4;
			
			//yyc modify
//			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_TcpHeader.th_flag_res_offset); ptr+=2;
			*((unsigned short *)ptr)=m_ProtocolHeader.m_TcpHeader.th_flag_res_offset; ptr+=2;

			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_TcpHeader.Window); ptr+=2;
			m_ProtocolHeader.m_TcpHeader.Checksum=0;
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_TcpHeader.Checksum); ptr+=2;
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_TcpHeader.UrgentPointer); ptr+=2;
			if(datalen>0)
			{//Fill in data
				memcpy(ptr,data,datalen); ptr+=datalen; }
			//Generate TCP checksum
			m_ProtocolHeader.m_TcpHeader.Checksum=socketRaw::checksum((unsigned short *)pstart_tcp,ptr-pstart_tcp);
			*((unsigned short *)(pstart_tcp+12+16))=htons(m_ProtocolHeader.m_TcpHeader.Checksum);
			//Remove pseudo-header
			memmove((void *)pstart_tcp,(void *)(pstart_tcp+12),ptr-pstart_tcp-12);
			m_IpV4Header.TotalLength=ptr-encodebuf-12;//subtract pseudo-header length of 12
		}
		break;
		case IPPROTO_UDP:
		{
			char *pstart_udp=ptr;
			//UDP length
			m_ProtocolHeader.m_UdpHeader.Length=datalen+8;
			//Fill in pseudo-header (used for checksum calculation only, not actually sent)
			*((unsigned int *)ptr)=htonl(m_IpV4Header.SourceAddress);
			*((unsigned int *)(ptr+4))=htonl(m_IpV4Header.DestinationAddress);
			*(ptr+8)=0;
			*(ptr+9)=IPPROTO_UDP;//Protocol type
			*((unsigned short *)(ptr+10))=htons(m_ProtocolHeader.m_UdpHeader.Length);//UDP length
			
			ptr+=12;//Pseudo-header is 12 bytes long
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_UdpHeader.SourcePort); ptr+=2;
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_UdpHeader.DestinationPort); ptr+=2;
			
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_UdpHeader.Length); ptr+=2;
			m_ProtocolHeader.m_UdpHeader.Checksum=0;
			*((unsigned short *)ptr)=htons(m_ProtocolHeader.m_UdpHeader.Checksum); ptr+=2;
			if(datalen>0)
			{//Fill in data
				memcpy(ptr,data,datalen); ptr+=datalen; }
			//Generate UDP checksum
			m_ProtocolHeader.m_UdpHeader.Checksum=socketRaw::checksum((unsigned short *)pstart_udp,m_ProtocolHeader.m_UdpHeader.Length+12);
			*((unsigned short *)(pstart_udp+12+6))=htons(m_ProtocolHeader.m_UdpHeader.Checksum); 
			//Remove pseudo-header
			memmove((void *)pstart_udp,(void *)(pstart_udp+12),m_ProtocolHeader.m_UdpHeader.Length);
			m_IpV4Header.TotalLength=ptr-encodebuf-12;//subtract pseudo-header length of 12
		}
		break;
		default:
			if(datalen>0)
			{//Fill in data
				memcpy(ptr,data,datalen); ptr+=datalen; }
			m_IpV4Header.TotalLength=ptr-encodebuf;
		break;
	}//?switch
	if(m_IpV4Header.TotalLength>0){
		*((unsigned short *)(encodebuf+2))=htons(m_IpV4Header.TotalLength);
		//Generate IP header checksum
		m_IpV4Header.HeaderChecksum=socketRaw::checksum((unsigned short *)encodebuf,(m_IpV4Header.IHL_Version & 0x0f)*4);
		*((unsigned short *)(encodebuf+10))=htons(m_IpV4Header.HeaderChecksum);
		if(m_IpV4Header.Protocol==IPPROTO_TCP && datalen==0){
			::memset(encodebuf+m_IpV4Header.TotalLength,0,4); return m_IpV4Header.TotalLength+4;}
	}
	return m_IpV4Header.TotalLength;
}

void socketRaw :: ConstructIPV4Header(unsigned char  ucProtocol,unsigned char  ucHeaderLength)
{
	memset((void *)&m_IpV4Header,0,sizeof(IpV4Header));
	m_IpV4Header.IHL_Version=ucHeaderLength/4 + IpVersion*16;
	m_IpV4Header.Protocol=ucProtocol;
	m_IpV4Header.Frag_and_flags=0;//usFragmentationFlags;
	m_IpV4Header.Identification=rand();//usIdentification;
	m_IpV4Header.TypeOfService=IpService_ROUTINE;

	m_IpV4Header.TimeToLive=m_TTL;
	m_IpV4Header.SourceAddress=socketBase::Host2IP(m_SourceAddress.c_str());
	return;
}

//size -- size in bytes, not unsigned short count
unsigned short socketRaw :: checksum(unsigned short *buffer, int size)
{
	unsigned long cksum=0;
	while (size>1) 
	{
		cksum += *buffer++;
		size -= sizeof(unsigned short);
	}
	if(size) //Odd number of bytes
	{
		cksum += *(unsigned char*)buffer;
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >>16);
	return (unsigned short)(~cksum);
}
//bindip==NULL binds all IPs; =="" binds the first local IP
//Otherwise bind to the specified IP
bool socketRaw :: create(int IPPROTO_type,const char *bindip)
{
	socketBase::create(SOCKS_RAW);
	int af=m_localAddr.sin_family;
	//On Linux, only root can create a RAW_SOCKET
#ifndef WIN32   //You must be root user to create RAW_SOCKET
	if(geteuid() != 0) return false;//return SOCKSERR_NOROOT;
#endif
	if( (m_sockfd=::socket(af,SOCK_RAW ,IPPROTO_type))!=INVALID_SOCKET )
	{
		if(bindip){
			if(bindip[0]!=0) m_SourceAddress.assign(bindip);
			bindip=m_SourceAddress.c_str();
		}
		if(this->Bind(0,FALSE,bindip)>=0)
		{
			int on=1; //Set that the application will send the IP header
			if(setsockopt(m_sockfd, IPPROTO_IP, IP_HDRINCL, (char *) &on,sizeof(on))!=SOCKET_ERROR)
			{
				m_socktype=SOCKS_RAW;
				m_sockstatus=SOCKS_OPENED;
				return true;
			}
			RW_LOG_DEBUG(0,"Failed to setsockopt(...IPPROTO_IP, IP_HDRINCL...)\r\n");
		}
		else
			RW_LOG_DEBUG("Failed to Bind %s\r\n",bindip);
	}
	else 
		RW_LOG_DEBUG("Failed to create %d IPPROTO.\r\n",IPPROTO_type);
	return false;
/*  Get protocol number by protocol name
	struct protoent *p = NULL; //yyc add 2005-08-24
	if(protocol!=NULL && protocol[0]!=0)
	{
		if( (p = getprotobyname( protocol))==NULL )
			RW_LOG_PRINT(LOGLEVEL_ERROR,"failed to getprotobyname(%s)\r\n",protocol);
	}
	int protno = p ? p -> p_proto : 0;//IPPROTO_RAW;//0(IPPROTO_IP);
*/
}

//Set or clear promiscuous mode on the specified fd
//b=true--set b=false---clear
//Returns SOCKSERR_OK(0) on success, otherwise an error code
//!! If setting promiscuous mode, this socketRaw object must be bound to an IP
SOCKSRESULT socketRaw :: Set_Promisc(bool b)
{
	if(m_socktype!=SOCKS_RAW) return SOCKSERR_INVALID;
	
	SOCKSRESULT sr=SOCKSERR_OK;
#ifdef WIN32
	//Set SOCK_RAW to SIO_RCVALL to receive all IP packets		
	DWORD dwValue =(b)?RCVALL_ON:RCVALL_OFF;
	sr=ioctlsocket(m_sockfd, SIO_RCVALL, &dwValue);
#else
	struct ifreq ifr;
	strncpy(ifr.ifr_name,"",1);//Set the network interface name to put into promiscuous mode!!!!!!!
	sr=ioctlsocket(m_sockfd,SIOCGIFFLAGS,&ifr);
	if(sr>=0)
	{
	//Set to promiscuous mode
		if(b)
			ifr.ifr_flags |= IFF_PROMISC;
		else
			ifr.ifr_flags &= ~IFF_PROMISC;
		if( (sr=ioctlsocket(m_sockfd, SIOCSIFFLAGS, &ifr))>=0 ) sr=SOCKSERR_OK;	
	}
#endif
	if(sr==SOCKET_ERROR) m_errcode=SOCK_M_GETERROR;
	return sr;
}
