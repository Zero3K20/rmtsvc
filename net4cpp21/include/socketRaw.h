/*******************************************************************
   *	socketRaw.h
   *    DESCRIPTION:Raw socket class definition
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SOCKET_RAW_H__
#define __YY_SOCKET_RAW_H__

#include "socketBase.h"
#include "ipdef.h"

namespace net4cpp21
{
	class socketRaw : public socketBase
	{
	public:
		socketRaw();
		virtual ~socketRaw(){}
		//Set the packet Time to live
		void SetTTL(unsigned char ucTTL) { m_TTL=ucTTL; return;}
		//Set source address for spoofing
		void SetSourceAddress(const char *lpSourceAddress)
		{
			if(lpSourceAddress) m_SourceAddress.assign(lpSourceAddress);
			return;
		}
		
		//Generate checksum
		static unsigned short checksum(unsigned short *buffer, int size);
	private: //Disable copy and assignment
		socketRaw(socketRaw &sockRaw){ return; }
		socketRaw & operator = (socketRaw &sockRaw) { return *this; }

	protected:
		//Decode IP packet; returns pointer to user data in buf on success, NULL for invalid packet
		//buf --- received IP packet byte stream; len --- byte stream length
		//After successful decoding, this function returns a pointer to the actual data; data length is obtained via the datalen function
		char * decode_ipv4(char *buf,int len);
		unsigned short dataLen_ipv4();
		//Encode user data into an IP packet byte stream; returns the encoded stream size
		//data --- pointer to user data; datalen --- user data length
		//encodebuf --- buffer for the encoded IP packet byte stream
		int encode_ipv4(const char *data,int datalen,char *encodebuf);
		void ConstructIPV4Header(unsigned char  ucProtocol,unsigned char  ucHeaderLength);
		//bindip==NULL binds all IPs; =="" binds the first local IP
		//Otherwise bind to the specified IP and create a Raw socket
		bool create(int IPPROTO_type,const char *bindip);
		//Set or clear promiscuous mode on the specified fd
		//Returns SOCKSERR_OK(0) on success, otherwise an error code
		SOCKSRESULT Set_Promisc(bool b);
		
		//Time to live
		unsigned char m_TTL;
		std::string m_SourceAddress;//Source IP defaults to the first local IP
		IpV4Header m_IpV4Header;//IP Header
		union
		{
			TcpHeader m_TcpHeader;
			UdpHeader m_UdpHeader;
		}m_ProtocolHeader; //Protocol Header

	}; 
}//?namespace net4cpp21

#endif

/*
		//Get pointer to the IP header structure
		LPIpV4Header get_ipHeaderPtr()
		{
			return &m_IpV4Header;
		}
		LPTcpHeader get_tcpHeaderPtr()
		{
			if(m_IpV4Header.Protocol==IPPROTO_TCP) return &m_ProtocolHeader.m_TcpHeader;
			return NULL;
		}
		LPUdpHeader get_udpHeaderPtr()
		{
			if(m_IpV4Header.Protocol==IPPROTO_UDP) return &m_ProtocolHeader.m_UdpHeader;
			return NULL;
		} 
*/




