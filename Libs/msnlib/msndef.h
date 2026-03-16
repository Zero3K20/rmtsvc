/*******************************************************************
   *	msndef.h
   *    DESCRIPTION:declaration and definition header file
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2005-06-03
   *	
   *******************************************************************/
#ifndef __YY_CMSNDEF_H__
#define __YY_CMSNDEF_H__

#include "../../include/proxyclnt.h"
#include "../../include/Buffer.h"
#include "../../utils/cCmdShell.h"
#include "msncx.h"

#define MSN_SERVER_HOST "messenger.hotmail.com"
#define MSN_SERVER_PORT 1863 //default MSN service port
#define MSN_MAX_RESPTIMEOUT 15 //s maximum wait timeout
#define MSN_MAX_NS_COMMAND_SIZE 1024 //maximum byte length of MSN commands
#define MSN_MAX_SS_COMMAND_SIZE 4096

#define MSN_ERR_OK SOCKSERR_OK
#define SOCKSERR_MSN_RESP -101 //incorrect message response
#define SOCKSERR_MSN_EMAIL SOCKSERR_MSN_RESP-1 //invalidaccount
#define SOCKSERR_MSN_DSCONN SOCKSERR_MSN_RESP-2 //cannot connect to DS server
#define SOCKSERR_MSN_NSCONN SOCKSERR_MSN_RESP-3 //cannot connect to NS server
#define SOCKSERR_MSN_AUTH SOCKSERR_MSN_RESP-4
#define SOCKSERR_MSN_SIGNIN SOCKSERR_MSN_RESP-5 //client not logged in
#define SOCKSERR_MSN_GETNS SOCKSERR_MSN_RESP-6 //failed to get NS server info
#define SOCKSERR_MSN_GETHASHKEY SOCKSERR_MSN_RESP-7
#define SOCKSERR_MSN_STATUS SOCKSERR_MSN_RESP-8 //current contact status is invalid, cannot perform the corresponding operation
#define SOCKSERR_MSN_XFR SOCKSERR_MSN_RESP-9
#define SOCKSERR_MSN_NULL SOCKSERR_MSN_RESP-10
#define SOCKSERR_MSN_UNKNOWED SOCKSERR_MSN_RESP-11

typedef long HCHATSESSION; //definechat会话handle，实际此handlepointer tocContactorobject
	
typedef enum
{
	MSN_CHATSESSION_CREATE,
	MSN_CHATSESSION_DESTROY, 
	MSN_CHATSESSION_JOIN,
	MSN_CHATSESSION_BYE,
	MSN_CHATSESSION_TYPING,
	MSN_CHATSESSION_CHAT
}MSN_CHATEVENT_TYPE;
	
typedef enum
{ 
	MSNINVITE_TYPE_UNKNOW=0,
	MSNINVITE_TYPE_PICTURE, //avatarrequest only for msnc1
	MSNINVITE_TYPE_FILE,//file transferrequest
	MSNINVITE_TYPE_CAM=4, //networkcamera,only for msnc1
	MSNINVITE_TYPE_NETMEET, //only for msnc0
	MSNINVITE_TYPE_AUDIO, //voice/audio chat，only for msnc0
	MSNINVITE_TYPE_ROBOT=99995225  //robot invitation
}MSN_INVITE_TYPE;
	
typedef enum
{
	MSNINVITE_CMD_INVITE=0,
	MSNINVITE_CMD_ACCEPT,
	MSNINVITE_CMD_REJECT,
	MSNINVITE_CMD_COMPLETED //yyc add 2006-05-19 receive/send完毕
}MSN_INVITE_CMD;
 
namespace net4cpp21
{
	class cContactor //contactinfostructure
	{
	public:
		unsigned long m_clientID;//identifieruserclientinfo的clientID。见clientID的相关说明
		std::string m_strMsnObj;//receive的msnbojstring，对于本account，指本account的avatarobject
							//经过MIME encoding的string，未进行MIMEdecoding
		std::string m_email;//account;
		std::wstring m_nick;//user昵称 unicodeencoding
		std::string m_nick_char;//user昵称
		std::string m_uid;//已adduser唯一identifier，仅仅atFL中的contact有uid
		std::string m_gid;//usergroup IDidentifier
		int m_flags;//bitflag 1=FL, 2=AL, 4=BL ,8=RL

		std::string m_bpr_phh;//user家庭电话
		std::string m_bpr_phw;//user办公室电话
		std::string m_bpr_phm;//user的移动电话
		char m_bpr_mob; //此人whether可receive移动message，default为N
		char m_bpr_hsb;//whether有msnspacedefault为0没有 1-有
		std::string m_status;//status "NLN","FLN","IDL","BSY","AWY","BRB","PHN","LUN","HDN"
	//						correspondingstatusdescription为"online","offline","idle","busy","away","brb","phone","lunch","invisible"
		
		socketProxy m_chatSock;//msnclientandSSservice建立的对currentcontact的chat通道。
						//对于loginaccount，则此socket为andNSserver建立的command通道
		std::map<std::string,cMsncx *> m_msncxMaps; //msncxobjectlist
		std::map<long,cMsnc1 *> m_msnc1Maps;//msnc1transfer会话statuslist,m_msncxMapslistcontainsm_msnc1Maps
		cBuffer m_buffer; //temporarybufferonly for msnc1. 用于receivecompletemsnslpmessage
		cCmdShellAsyn m_shell;
		//扩充flagbit，用于msnMessager派生class记录contact的其他意义
		unsigned long m_lflagEx;
		
		//yyc add 2006-03-13 //建立会话session后temporarysavepending send的message
		//因为会话sessioncreate后，可能还没有收到IROorJOImessage，at this pointsession中还没有人ifat this pointsend
		//可能会导致server端closesession的connect
		long m_chat_contacts;//参与此次交谈的会话的contact个数
		std::vector<std::pair<char *,long> > m_vecMessage;

		cContactor():m_flags(0),m_clientID(0),m_bpr_mob('N'),m_chat_contacts(0),
			m_status("FLN"),m_bpr_hsb(0),m_lflagEx(0){}
//		{ m_chatSock.ForbidAutoLinger(); } //yyc remove 2007-08-22
		~cContactor(){m_buffer.Resize(0);m_shell.destroy();}
	};
}//?namespace net4cpp21

inline int wchar2chars(const wchar_t *wstr,char *pdest,int destLen)
{
	return 	WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK|WC_DISCARDNS|WC_SEPCHARS|WC_DEFAULTCHAR,wstr,-1,pdest,destLen,NULL,NULL);
}
#endif
