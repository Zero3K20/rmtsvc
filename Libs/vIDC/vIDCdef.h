/*******************************************************************
   *	vidcdef.h
   *    DESCRIPTION:declaration and definition header file
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-06-03
   *	
   *******************************************************************/
#ifndef __YY_VIDCDEF_H__
#define __YY_VIDCDEF_H__

#define VIDCS_VERSION 26 //vIDC server version
#define VIDCC_VERSION 26 //vIDC client version
#define VIDCC_MIN_VERSION 25 //支持的vidcc最小version

#define VIDC_SERVER_PORT 8080 //defaultvIDC服务port
#define VIDC_MAX_RESPTIMEOUT 10 //s maximum wait timeout
#define VIDC_PIPE_ALIVETIME 20 //s 管道最长存活time，如果在specified的time还没有被绑定则释放
#define VIDC_MAX_COMMAND_SIZE 256 //vIDC命令最大字节length
#define VIDC_MAX_CLIENT_TIMEOUT 180 //如果多久没有收到vIDCclient的命令或消息则认为disconnect了
#define VIDC_NOP_INTERVAL 60  //vIDCcsend心跳的间隔s
//defineVIDC的error返回info
#define SOCKSERR_VIDC_VER -301 //version不匹配
#define SOCKSERR_VIDC_PSWD -302 //password不正确
#define SOCKSERR_VIDC_RESP -303 //timeout或响应error
#define SOCKSERR_VIDC_NAME -304 //invalidname
#define SOCKSERR_VIDC_MEMO -305 //内存分配failure
#define SOCKSERR_VIDC_MAP  -306 //映射failure
#define SOCKSERR_VIDC_SURPPORT -307 //暂时not supported此功能
#endif

/***********************vIDChandle流程******************************************************

vidcc                              vidcs
           <connect>
	------------------------------->
	       HELO connect认证
	------------------------------->
	       200 ... OK
	<-------------------------------
	       ADDR ...
	------------------------------->
		   200 ... OK
	<-------------------------------
	       <start其它命令交互>
	       .................
		   <close>
	<------------------------------->


------------------vIDC Server handle流程-----------------------------------------
1、启动侦听
2、当有一个connect进来，建立connect
3、等待receive命令data，如果VIDC_MAX_RESPTIMEOUT内没有收到任何命令则关闭此connect
4、判断命令是否为HELO或PIPE，如果不是则关闭connect
5、如果是HELO命令，则进行clientconnect认证，如果不通过则返回error，关闭connect
6、否则建立一个session回话，循环等待handlevIDC命令，并返回success消息
7、如果是PIPE命令，
------------------vIDC Client handle流程-----------------------------------------
*****************************************************************************************/