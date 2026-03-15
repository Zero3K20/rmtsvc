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
	TCPSVR_TCPSVR=0, //不进行服务转换
	TCPSVR_SSLSVR,   //将普通TCP服务转换为SSL加密服务
	SSLSVR_TCPSVR    //将SSL加密服务转换为普通TCP服务
}SSLTYPE;

typedef enum 
{
	VIDC_MAPTYPE_TCP=0,		//TCP映射
	VIDC_MAPTYPE_UDP,		//UDP映射
	VIDC_MAPTYPE_PROXY		//内网代理映射
}VIDC_MAPTYPE;


#endif

