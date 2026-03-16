/*******************************************************************
   *	mportdef.h
   *    DESCRIPTION:constants, structures and enum definitions for port mapping
   *				
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_MPORTDEF_H__
#define __YY_MPORTDEF_H__

#define MAX_TRANSFER_BUFFER 4096
#define MAX_CONNECT_TIMEOUT 15
#define MAX_RECVREQ_TIMEOUT 10
 
typedef enum //port mapping type definitions
{
	MPORTTYPE_UNKNOW=0,//unknown
	MPORTTYPE_UDP,
	MPORTTYPE_TCP,
	MPORTTYPE_WWW,
	MPORTTYPE_FTP
}MPORTTYPE; 

typedef enum
{
	TCPSVR_TCPSVR=0, //no service conversion
	TCPSVR_SSLSVR,   //convert plain TCP service to SSL-encrypted service
	SSLSVR_TCPSVR    //convert SSL-encrypted service to plain TCP service
}SSLTYPE;

typedef enum 
{
	VIDC_MAPTYPE_TCP=0,		//TCP mapping
	VIDC_MAPTYPE_UDP,		//UDPmap
	VIDC_MAPTYPE_PROXY		//内网代理map
}VIDC_MAPTYPE;


#endif

