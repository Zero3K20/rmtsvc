/*******************************************************************
   *	msnlib.h
   *    DESCRIPTION:MSN protocol handler class definition
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2005-06-03
   *	
   *******************************************************************/

#ifndef __YY_CMSNLIB_H__
#define __YY_CMSNLIB_H__

#include "msndef.h"

#define __SURPPORT_MSNPROXY__
namespace net4cpp21
{
	class msnMessager //MSN client object
	{
#ifdef __SURPPORT_MSNPROXY__
	public:
		void *proxy_createConnection(HCHATSESSION hchat,socketTCP *psock,const char *host);
//				std::vector<std::pair<HCHATSESSION,msnMessager *> > *pipes);
		static void proxy_transferData(void *param,const char *otherdata,long datalen);
		static void proxy_transThread(void *param){ proxy_transferData(param,NULL,0); }
	private:
		unsigned long proxy_senddata(HCHATSESSION hchat,const char *proxyCmd,const char *content,int len);
		void onProxyChat(HCHATSESSION hchat,const char *proxyCmd, char *undecode_szMsg,int szMsglen);
#endif
	
		friend cMsnc1; friend cMsnc0;
		cMutex m_mutex;
		//condition variable used to wait for message reply, key--trID(m_msgID)
		std::map<unsigned long,cCond *> m_conds;
		
		std::string m_photofile;//本account的头像filename称
		unsigned long m_msgID;//消息ID
		char m_Connectivity;//是否能直接connect本client(即本client是否通过NAT或代理出去的)
						//default'N' ,'Y' : 可以直接connect
		std::string m_clientIP; //本clientconnect出去的IP(公网IP)

		//聊天会话defaultsend字体的name外观
		std::string m_fontName;//字体name
		std::string m_encodeFontname;//经过编码过的字体name字符串
		DWORD m_fontColor;//字体颜色
		std::string m_fontEF;//字体效果,B粗体 空常规 I斜体  Sdelete线 U下划线.  可组合

		bool sendcmd_VER();
		bool sendcmd_CVR(const char *strAccount);
		//send command to get the NS server address
		int sendcmd_USR(const char *strAccount,std::string &strNShost,int &iNSport);
		//send command获取https hashkey
		bool sendcmd_USR(const char *strAccount,std::string &hashkey);
		bool sendcmd_USR(std::string &hashkey);
		//sendstatus改表消息 "NLN","FLN","IDL","BSY","AWY","BRB","PHN","LUN","HDN"
		bool sendcmd_CHG(const char *sta);
		bool sendcmd_UUX();
		bool sendcmd_PNG();
		//nick --- 经过utf8和mime编码的昵称字符串
		bool sendcmd_ADC(const char *email,const char *nick_mime,long waittimeout);
		bool sendcmd_ADC(const char *email,const char *strFlag);
		bool sendcmd_REM(const char *email,const char *strFlag);
		bool sendcmd_XFR(socketProxy &chatSock,const char *email);
		
		cContactor * _newContact(const char *email,const wchar_t *nick);
		bool connectSvr(socketProxy &sock,const char *strhost,int iport);

		//nscmd_xxx返回消息的ID即trID
		unsigned long nscmd_sbs(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_msg(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_chl(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_syn(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_lsg(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_lst(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_bpr(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_prp(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_ubx(socketTCP *psock,const char *email,const char *pdata);
		unsigned long nscmd_iln(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_fln(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_nln(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_rem(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_adc(socketTCP *psock,const char *pcmd);
		unsigned long nscmd_rng(socketTCP *psock,const char *pcmd);
		
		unsigned long sscmd_msg(cContactor *pcon,const char *msg_email,char *pcmd,int cmdlen);
		void msnc0_parse(cContactor *pcon,const char *msg_email,char *ptrmsg);
		void msnc1_parse(cContactor *pcon,const char *msg_email,unsigned char *pBinarystuff,
								char *ptrmsg,unsigned long lfooter);

		static void receiveThread(msnMessager *pmsnmessager); //handle command interaction between this client and the NS server
		static void sessionThread(void *param); //handle本client和SSserver的命令交互
		static void shellThread(void *param);
		static SOCKSRESULT passport_auth(std::string &strKey,const char *strAccount,const char *pwd);
		static bool MSNP11Challenge(std::string &strChallenge,const char *szClientID,const char *szClientCode);
		unsigned long msgID() { return ++m_msgID; }
		void eraseCond(unsigned long trID){
			m_mutex.lock(); m_conds.erase(trID); m_mutex.unlock();
			return; }

	protected:
		SOCKSRESULT m_lasterrorcode;
		cThreadPool m_threadpool;//服务thread pool
		cContactor m_curAccount;//current登录accountinfo
		std::map<std::string, cContactor *> m_contacts;//联系人列表 first:联系人email second:联系人info结构指针
		std::map<std::string,std::wstring> m_groups;//组列表 first:组ID，second:组name
		
		void clear();
		bool sendcmd_SS_Typing(cContactor *pcon,const char *type_email);
		bool sendcmd_SS_chatMsg(cContactor *pcon,char *msgHeader,int headerlen,const char *chatMsg,int msglen);
		bool sendcmd_SS_chatMsg(std::vector<cContactor *> &vec,char *msgHeader,int headerlen,const char *chatMsg,int msglen);
		bool sendcmd_SS_chatMsgW(cContactor *pcon,char *msgHeader,int headerlen,const wchar_t *chatMsg,int msglen);
		bool sendcmd_SS_chatMsgW(std::vector<cContactor *> &vec,char *msgHeader,int headerlen,const wchar_t *chatMsg,int msglen);
		
		int encodeChatMsgHead(char *buffer,int buflen,const char *IMFont,const char *dspname);
		int encodeChatMsgHeadW(char *buffer,int buflen,const wchar_t *IMFont,const wchar_t *dspname);
		
		bool createShell(HCHATSESSION hchat);
		bool destroyShell(HCHATSESSION hchat);

		HCHATSESSION createChat(cContactor *pcon);
	public:
		msnMessager();
		virtual ~msnMessager();
		int getLastErrorCode() const { return m_lasterrorcode; }
		const char *thisEmail() { return m_curAccount.m_email.c_str(); }
		bool Connectivity() const { return (m_Connectivity=='Y'); }
		const char *clientIP() const { return m_clientIP.c_str(); }
		bool ifSigned() { 
			return (m_curAccount.m_chatSock.status()==SOCKS_CONNECTED); 
		}

		bool signin(const char *strAccount,const char *strPwd);
		virtual void signout();//不可在事件中调用此方法
		//设置聊天字体
		bool setChatFont(const char *fontName,const char *fontEF,long fontColor);
		bool changeNick(const char *nick);
		bool changeNickW(const wchar_t *nickW,int nicklen=0);
		bool changeStatus(const char *sta)
		{
			return sendcmd_CHG(sta);
		}
		//阻塞某个account，将account从AL中rem，添加到BL中
		bool blockEmail(const char *email);
		bool unblockEmail(const char *email);
		//delete某个account，ifBlock--是否阻止此account
		bool remEmail(const char *email,bool ifBlock);
		//添加联系人
		//如果waittimeout＝0则不等待server的返回
		//>0等待specified的time否则等待MSN_MAX_TIMEOUT
		bool addEmail(const char *email,long waittimeout);
		//向某个用户发起一个聊天,success返回HCHATSESSION，否则返回0
		//能发起聊天的用户必须是有效的用户status不能是FLN，且在我的联系人FL队列中
		HCHATSESSION createChat(const char *email);
		//邀请某人进入一个聊天会话
		bool inviteChat(HCHATSESSION hchat,const char *email);

		void destroyChat(HCHATSESSION hchat);
		//send聊天内容.如果strMsg==NULL则send聊天控制消息，指明本人正在输入聊天消息
		bool sendChatMsg(HCHATSESSION hchat,const char *strMsg,const char *dspname=NULL);
		bool sendChatMsg(HCHATSESSION hchat,std::string &strMsg,const char *dspname=NULL);
		bool sendChatMsgW(HCHATSESSION hchat,const wchar_t *strMsg,const wchar_t *dspname=NULL);
		bool sendChatMsgW(HCHATSESSION hchat,std::wstring &strMsg,const wchar_t *dspname=NULL);
		//设置/取消本account的头像
		bool setPhoto(const char *imagefile);
		//获取某个联系人的头像
		bool getPhoto(HCHATSESSION hchat,const char *filename);
		bool getPhoto(const char *email,const char *filename);
		bool sendFile(HCHATSESSION hchat,const char *filename);
//-----------------------virtual function--------------------------------------
		//登录success后，收到server的SYN的响应，返回此account的联系人个数和组个数
		virtual void onSYN(long contactors,long groups) { return; }
		//从服务端获得某个组info
		virtual void onLSG(const wchar_t *groupname,const char *groupid){ return; }
		//从服务端获得某个联系人info
		virtual void onLST(const char *email,const wchar_t *nick,int flags){ return; }
		//登录success后，receive完所有联系人列表触发此事件,此时用户应send上线消息. 
		virtual void onSIGN(){
			sendcmd_CHG("NLN");//send上线消息和UUX消息
			sendcmd_UUX(); return;
		}
		virtual void onSIGNOUT(){ return; }
		//某个用户在线/上线
		virtual void onLine(HCHATSESSION hchat,const char *email) { return; }
		//某个用户下线
		virtual void offLine(HCHATSESSION hchat,const char *email) { return; }
		//某个用户status/name改变. 1:status改变 2:昵称改变 4:clientID改变 8:头像改变
		//如果最高位为1则说明收到的是ILN消息
		virtual void onNLN(HCHATSESSION hchat,const char *email,long flags){ return; }
		//某个用户delete了你触发此事件.返回0，什么也不作。返回
		//1 --- 将此用户delete //2 --- 将此用户阻塞 //3 --- 将此用户delete并阻塞
		virtual int onREM(HCHATSESSION hchat,const char *email){ return 1;}
		//某个用户添加了本account，返回真receive，否则拒绝此人看到自己
		virtual bool onADC(HCHATSESSION hchat,const char *email){ return true; }
		//本account主动添加某个用户，success后的通知事件
		virtual void onADD(HCHATSESSION hchat,const char *email){ return; }
		//chattype --- 指示此事件的type
		//	CHATSESSION_CREATE : create了一个新的chat session. email---create此chatsession的联系人accountemail. lparam无意义
		//	CHATSESSION_DESTROY: 一个chat session被销毁。 email/lparam：无意义
		//	CHATSESSION_JOIN: 一个联系人加入此聊天会话.email ---联系人accountemail. lparam:current会话的聊天者个数(long)
		//	CHATSESSION_BYE: 一个联系人退出了此聊天会话。email ---联系人accountemail. lparam:current会话的聊天者个数(long)
		//	CHATSESSION_TYPING: 一个联系人正在输入消息.email ---联系人accountemail. lparam:无意义
		virtual void onChatSession(HCHATSESSION hchat,MSN_CHATEVENT_TYPE chattype,const char *email,long lParam)
		{
			return;
		}
		//收到一个聊天info事件,char *是未经过mime和utf8解码的字符串，由派生类自己handle
		//聊天文字是utf-8编码的，字体format是经过MIME及utf-8编码的
		virtual void onChat(HCHATSESSION hchat,const char *email, char *undecode_szMsg,
			int &szMsglen,char *undecode_szFomat,char *undecode_szdspName) { return; }
	
		//如果invitecmd==INVITE_CMD_INVITE,则此函数返回真为接受邀请，否则拒绝邀请
		//其他命令返回真假无意义
		virtual bool onInvite(HCHATSESSION hchat,int invitetype,
			MSN_INVITE_CMD invitecmd,cMsncx *pmsncx)
		{ 
			if(invitecmd==MSNINVITE_CMD_INVITE && 
				(invitetype==MSNINVITE_TYPE_FILE || invitetype==MSNINVITE_TYPE_PICTURE) )
				return true;
			return false; 
		}

		static bool SHA1File(FILE *fp,std::string &strRet);
		static bool SHA1Buf(const char *buf,long len,std::string &strRet);
		static bool MD5Buf(const char *buf,long len,std::string &strRet);
	};
}//?namespace net4cpp21

#endif

