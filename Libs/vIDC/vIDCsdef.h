/*******************************************************************
   *	vidcsdef.h
   *    DESCRIPTION:declaration and definition header file
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-06-03
   *	
   *******************************************************************/
#ifndef __YY_VIDCSDEF_H__
#define __YY_VIDCSDEF_H__

#include "mportsvr.h"

const char msg_err_221[]="221 %d require password.\r\n";
const char msg_err_405[]="405 %d wrong client version\r\n"; //wrong version
const char msg_err_500[]="500 command unrecognized.\r\n";
const char msg_err_501[]="501 can not find mportSvr\r\n";
const char msg_err_502[]="502 failed to new Object\r\n";
const char msg_err_503[]="503 not support\r\n"; //feature temporarily not supported
const char msg_err_504[]="504 %d failed to map\r\n"; //mapping failed
const char msg_err_ok[]="200 %d %s success to map\r\n"; //mapping successful
const char msg_ok_200[]="200 command ok\r\n";

namespace net4cpp21
{
	class vidccSession;
	class mportTCP_vidcs : public mportTCP
	{
	public:
		explicit mportTCP_vidcs(vidccSession *psession);
		virtual ~mportTCP_vidcs(){}
		
	protected:
		virtual socketTCP * connectAppsvr(char *strHost,socketTCP *psock);

	private:
		vidccSession * m_psession; //vidccSession associated with this map port
	};

	class vidccSession
	{
	public:
		explicit vidccSession(socketTCP *psock,int ver,const char *strname,const char *strDesc);
		~vidccSession(){}
		int vidccVer() { return m_vidccVer; }
		const char *vidccName() { return m_strName.c_str(); }
		const char *vidccDesc() { return m_strDesc.c_str(); }
		const char *vidccIP() { return m_psock_command->getRemoteIP(); }
		time_t ConnectTime() { return m_tmConnected; }
		bool isConnected() { return (m_psock_command->status()==SOCKS_CONNECTED); }
		void Close() { if(m_psock_command) m_psock_command->Close(); }
		void setIfLogdata(bool b); //set whether to log all services mapped in this session
		void xml_list_mapped(cBuffer &buffer);
		
		void parseCommand(const char *ptrCommand);
		void Destroy(); //destroy and free resources
		bool AddPipe(socketTCP *pipe); //add一个idle管道
		bool DelPipe(socketTCP *pipe); //delete一个管道
		socketTCP *GetPipe();
		
		long docmd_sslc(const char *strSSLC,const char *received,long receivedByte);
	private:
		void docmd_bind(const char *param);
		void docmd_unbind(const char *param);
		void docmd_addr(const char *param);
		void docmd_ipfilter(const char *strParam);
		void docmd_mdhrsp(const char *strParam);
		void docmd_mdhreq(const char *strParam);
		void docmd_vnop(const char *param);
		void docmd_unknowed(const char *ptrCommand);
		
	private:
		time_t m_tmConnected; //此vIDCcconnectstarttime
		int m_vidccVer; //connect的vidcclientversion
		std::string m_strName;
		std::string m_strDesc; //vIDCc的nameordescription
		socketTCP * m_psock_command; //主socket，command通道socket
		std::map<std::string,mportTCP_vidcs *> m_tcpsets; //TCPservicemapset
		std::vector<socketTCP *> m_pipes; //idle管道set
		cMutex m_mutex;
	};
}//?namespace net4cpp21
#endif
