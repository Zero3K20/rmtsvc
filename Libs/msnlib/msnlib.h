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
		
		std::string m_photofile;//filename of this account's avatar
		unsigned long m_msgID;//message ID
		char m_Connectivity;//whether this client can be connected directly (i.e. whether this client is behind NAT or proxy)
						//default 'N', 'Y': can connect directly
		std::string m_clientIP; //IP address of this client's outgoing connection (public IP)

		//default chat session font name and appearance
		std::string m_fontName;//font name
		std::string m_encodeFontname;//经过encoding过的font namestring
		DWORD m_fontColor;//font color
		std::string m_fontEF;//font effects: B=bold, empty=normal, I=italic, S=strikethrough, U=underline. Can be combined

		bool sendcmd_VER();
		bool sendcmd_CVR(const char *strAccount);
		//send command to get the NS server address
		int sendcmd_USR(const char *strAccount,std::string &strNShost,int &iNSport);
		//send command to get HTTPS hash key
		bool sendcmd_USR(const char *strAccount,std::string &hashkey);
		bool sendcmd_USR(std::string &hashkey);
		//send status change message "NLN","FLN","IDL","BSY","AWY","BRB","PHN","LUN","HDN"
		bool sendcmd_CHG(const char *sta);
		bool sendcmd_UUX();
		bool sendcmd_PNG();
		//nick --- 经过utf8andmimeencoding的昵称string
		bool sendcmd_ADC(const char *email,const char *nick_mime,long waittimeout);
		bool sendcmd_ADC(const char *email,const char *strFlag);
		bool sendcmd_REM(const char *email,const char *strFlag);
		bool sendcmd_XFR(socketProxy &chatSock,const char *email);
		
		cContactor * _newContact(const char *email,const wchar_t *nick);
		bool connectSvr(socketProxy &sock,const char *strhost,int iport);

		//nscmd_xxx return message ID, i.e. trID
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
		static void sessionThread(void *param); //handle command interaction between this client and SS server
		static void shellThread(void *param);
		static SOCKSRESULT passport_auth(std::string &strKey,const char *strAccount,const char *pwd);
		static bool MSNP11Challenge(std::string &strChallenge,const char *szClientID,const char *szClientCode);
		unsigned long msgID() { return ++m_msgID; }
		void eraseCond(unsigned long trID){
			m_mutex.lock(); m_conds.erase(trID); m_mutex.unlock();
			return; }

	protected:
		SOCKSRESULT m_lasterrorcode;
		cThreadPool m_threadpool;//servicethread pool
		cContactor m_curAccount;//currentloginaccountinfo
		std::map<std::string, cContactor *> m_contacts;//contact list: first=contact email, second=contact info structure pointer
		std::map<std::string,std::wstring> m_groups;//group list: first=group ID, second=group name
		
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
		virtual void signout();//this method cannot be called within an event
		//set chat font
		bool setChatFont(const char *fontName,const char *fontEF,long fontColor);
		bool changeNick(const char *nick);
		bool changeNickW(const wchar_t *nickW,int nicklen=0);
		bool changeStatus(const char *sta)
		{
			return sendcmd_CHG(sta);
		}
		//block an account; remove it from AL and add to BL
		bool blockEmail(const char *email);
		bool unblockEmail(const char *email);
		//delete an account; Block--whether to also block this account
		bool remEmail(const char *email,bool ifBlock);
		//add contact
		//if waittimeout=0, do not wait for server response
		//>0 wait for the specified time, otherwise wait MSN_MAX_TIMEOUT
		bool addEmail(const char *email,long waittimeout);
		//initiate a chat with a user; returns HCHATSESSION on success, 0 otherwise
		//the user being chatted with must have a valid status (not FLN) and be in my Forward List
		HCHATSESSION createChat(const char *email);
		//invite someone into a chat session
		bool inviteChat(HCHATSESSION hchat,const char *email);

		void destroyChat(HCHATSESSION hchat);
		//send chat content; if strMsg==NULL, send typing notification
		bool sendChatMsg(HCHATSESSION hchat,const char *strMsg,const char *dspname=NULL);
		bool sendChatMsg(HCHATSESSION hchat,std::string &strMsg,const char *dspname=NULL);
		bool sendChatMsgW(HCHATSESSION hchat,const wchar_t *strMsg,const wchar_t *dspname=NULL);
		bool sendChatMsgW(HCHATSESSION hchat,std::wstring &strMsg,const wchar_t *dspname=NULL);
		//set/cancel this account's avatar
		bool setPhoto(const char *imagefile);
		//get a contact's avatar
		bool getPhoto(HCHATSESSION hchat,const char *filename);
		bool getPhoto(const char *email,const char *filename);
		bool sendFile(HCHATSESSION hchat,const char *filename);
//-----------------------virtual function--------------------------------------
		//after login success, received SYN response from server; returns number of contacts and groups for this account
		virtual void onSYN(long contactors,long groups) { return; }
		//get group info from server-side
		virtual void onLSG(const wchar_t *groupname,const char *groupid){ return; }
		//get contact info from server-side
		virtual void onLST(const char *email,const wchar_t *nick,int flags){ return; }
		//after login success, all contact lists received, this event is triggered; user should send online message. 
		virtual void onSIGN(){
			sendcmd_CHG("NLN");//send online message and UUX message
			sendcmd_UUX(); return;
		}
		virtual void onSIGNOUT(){ return; }
		//a user is online/went online
		virtual void onLine(HCHATSESSION hchat,const char *email) { return; }
		//a user went offline
		virtual void offLine(HCHATSESSION hchat,const char *email) { return; }
		//a user's status/name changed: 1=status changed, 2=nickname changed, 4=clientID changed, 8=avatar changed
		//if the highest bit is 1, the received message is an ILN message
		virtual void onNLN(HCHATSESSION hchat,const char *email,long flags){ return; }
		//triggered when a user deletes you. Return 0 to do nothing. Return
		//1 --- delete this user //2 --- block this user //3 --- delete and block this user
		virtual int onREM(HCHATSESSION hchat,const char *email){ return 1;}
		//a user added this account; return true to accept, otherwise reject this person from seeing you
		virtual bool onADC(HCHATSESSION hchat,const char *email){ return true; }
		//this account actively added a user; notification event after success
		virtual void onADD(HCHATSESSION hchat,const char *email){ return; }
		//chattype --- indicates the type of this event
		//	CHATSESSION_CREATE: a new chat session was created. email=contact account email. lparam is meaningless
		//	CHATSESSION_DESTROY: a chat session was destroyed. email/lparam: meaningless
		//	CHATSESSION_JOIN: 一个contact加入此chat会话.email ---contactaccountemail. lparam:current会话的chat者个数(long)
		//	CHATSESSION_BYE: 一个contactexit了此chat会话。email ---contactaccountemail. lparam:current会话的chat者个数(long)
		//	CHATSESSION_TYPING: 一个contactin progressinputmessage.email ---contactaccountemail. lparam:无意义
		virtual void onChatSession(HCHATSESSION hchat,MSN_CHATEVENT_TYPE chattype,const char *email,long lParam)
		{
			return;
		}
		//收到一个chatinfoevent,char *yes未经过mimeandutf8decoding的string，由派生class自己handle
		//chat文字yesutf-8encoding的，fontformatis through过MIME及utf-8encoding的
		virtual void onChat(HCHATSESSION hchat,const char *email, char *undecode_szMsg,
			int &szMsglen,char *undecode_szFomat,char *undecode_szdspName) { return; }
	
		//ifinvitecmd==INVITE_CMD_INVITE,则此functionreturntrue为acceptinvitation，otherwiserejectinvitation
		//其他commandreturntruefalse无意义
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

