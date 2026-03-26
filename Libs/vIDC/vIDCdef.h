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
//defines VIDC error return info
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
	       <start other command interactions>
	       .................
		   <close>
	<------------------------------->


------------------vIDC Server handling flow-----------------------------------------
1. start listening
2. when a connection comes in, establish connection
3. waiting to receive command data, close this connection if no command is received within VIDC_MAX_RESPTIMEOUT
4. check if command is HELO or PIPE, if not close connection
5. if HELO command, perform client connection authentication, if it fails return error and close connection
6. otherwise establish a session, loop waiting to handle vIDC commands, and return success message
7. if PIPE command,
------------------vIDC Client handling flow-----------------------------------------
*****************************************************************************************/