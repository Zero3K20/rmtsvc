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
#define MSN_MAX_NS_COMMAND_SIZE 1024 //MSN命令最大字节length
#define MSN_MAX_SS_COMMAND_SIZE 4096

#define MSN_ERR_OK SOCKSERR_OK
#define SOCKSERR_MSN_RESP -101 //不正确的消息返回
#define SOCKSERR_MSN_EMAIL SOCKSERR_MSN_RESP-1 //invalidaccount
#define SOCKSERR_MSN_DSCONN SOCKSERR_MSN_RESP-2 //无法connectDSserver
#define SOCKSERR_MSN_NSCONN SOCKSERR_MSN_RESP-3 //无法connectNSserver
#define SOCKSERR_MSN_AUTH SOCKSERR_MSN_RESP-4
#define SOCKSERR_MSN_SIGNIN SOCKSERR_MSN_RESP-5 //client没有登录
#define SOCKSERR_MSN_GETNS SOCKSERR_MSN_RESP-6 //获取NSserverinfofailure
#define SOCKSERR_MSN_GETHASHKEY SOCKSERR_MSN_RESP-7
#define SOCKSERR_MSN_STATUS SOCKSERR_MSN_RESP-8 //current联系人的status无效,不能执行相应的操作
#define SOCKSERR_MSN_XFR SOCKSERR_MSN_RESP-9
#define SOCKSERR_MSN_NULL SOCKSERR_MSN_RESP-10
#define SOCKSERR_MSN_UNKNOWED SOCKSERR_MSN_RESP-11

typedef long HCHATSESSION; //define聊天会话句柄，实际此句柄指向cContactor对象
	
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
	MSNINVITE_TYPE_PICTURE, //头像请求 only for msnc1
	MSNINVITE_TYPE_FILE,//文件传输请求
	MSNINVITE_TYPE_CAM=4, //网络摄像头,only for msnc1
	MSNINVITE_TYPE_NETMEET, //only for msnc0
	MSNINVITE_TYPE_AUDIO, //音频聊天，only for msnc0
	MSNINVITE_TYPE_ROBOT=99995225  //机器人邀请
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
	class cContactor //联系人info结构
	{
	public:
		unsigned long m_clientID;//标识用户clientinfo的clientID。见clientID的相关说明
		std::string m_strMsnObj;//receive的msnboj字符串，对于本account，指本account的头像对象
							//经过MIME编码的字符串，未进行MIME解码
		std::string m_email;//account;
		std::wstring m_nick;//用户昵称 unicode编码
		std::string m_nick_char;//用户昵称
		std::string m_uid;//已添加用户唯一标识，仅仅在FL中的联系人有uid
		std::string m_gid;//用户group ID标识
		int m_flags;//位flag 1=FL, 2=AL, 4=BL ,8=RL

		std::string m_bpr_phh;//用户家庭电话
		std::string m_bpr_phw;//用户办公室电话
		std::string m_bpr_phm;//用户的移动电话
		char m_bpr_mob; //此人是否可receive移动消息，default为N
		char m_bpr_hsb;//是否有msn空间default为0没有 1-有
		std::string m_status;//status "NLN","FLN","IDL","BSY","AWY","BRB","PHN","LUN","HDN"
	//						对应的statusdescription为"online","offline","idle","busy","away","brb","phone","lunch","invisible"
		
		socketProxy m_chatSock;//msnclient和SS服务建立的对current联系人的聊天通道。
						//对于登录account，则此socket为和NSserver建立的命令通道
		std::map<std::string,cMsncx *> m_msncxMaps; //msncx对象列表
		std::map<long,cMsnc1 *> m_msnc1Maps;//msnc1传输会话status列表,m_msncxMaps列表包含m_msnc1Maps
		cBuffer m_buffer; //临时bufferonly for msnc1. 用于receive完整的msnslp消息
		cCmdShellAsyn m_shell;
		//扩充flag位，用于msnMessager派生类记录联系人的其他意义
		unsigned long m_lflagEx;
		
		//yyc add 2006-03-13 //建立会话session后临时保存待send的消息
		//因为会话sessioncreate后，可能还没有收到IRO或JOI消息，此时session中还没有人如果此时send
		//可能会导致server端关闭session的connect
		long m_chat_contacts;//参与此次交谈的会话的联系人个数
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
