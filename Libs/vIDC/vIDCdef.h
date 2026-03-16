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
#define VIDCC_MIN_VERSION 25 //minimum supported vidcc version

#define VIDC_SERVER_PORT 8080 //default vIDC service port
#define VIDC_MAX_RESPTIMEOUT 10 //s maximum wait timeout
#define VIDC_PIPE_ALIVETIME 20 //s maximum pipe lifetime; released if not bound within the specified time
#define VIDC_MAX_COMMAND_SIZE 256 //maximum byte length of vIDC commands
#define VIDC_MAX_CLIENT_TIMEOUT 180 //if no command or message is received from vIDC client for this long, consider it disconnected
#define VIDC_NOP_INTERVAL 60  //interval in seconds for vIDCc to send heartbeat
//defineVIDC的errorreturninfo
#define SOCKSERR_VIDC_VER -301 //version mismatch
#define SOCKSERR_VIDC_PSWD -302 //incorrect password
#define SOCKSERR_VIDC_RESP -303 //timeout or response error
#define SOCKSERR_VIDC_NAME -304 //invalidname
#define SOCKSERR_VIDC_MEMO -305 //memory allocation failure
#define SOCKSERR_VIDC_MAP  -306 //mapping failed
#define SOCKSERR_VIDC_SUPPORT -307 //this feature is temporarily not supported
#endif

/***********************vIDC handle flow******************************************************

vidcc                              vidcs
           <connect>
	------------------------------->
	       HELO connectauthentication
	------------------------------->
	       200 ... OK
	<-------------------------------
	       ADDR ...
	------------------------------->
		   200 ... OK
	<-------------------------------
	       <start其它command交互>
	       .................
		   <close>
	<------------------------------->


------------------vIDC Server handle流程-----------------------------------------
1、start侦听
2、when有一个connect进来，建立connect
3、waitingreceivecommanddata，ifVIDC_MAX_RESPTIMEOUT内没有收到任何command则close此connect
4、判断commandwhether为HELOorPIPE，ifis not则close connection
5、ifyesHELOcommand，则进行clientconnectauthentication，ifnot通过则returnerror，close connection
6、otherwise建立一个session回话，循环waitinghandlevIDCcommand，并return successmessage
7、ifyesPIPEcommand，
------------------vIDC Client handle流程-----------------------------------------
*****************************************************************************************/