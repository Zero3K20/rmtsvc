/*******************************************************************
   *	ftpsvr.h
   *    DESCRIPTION:FTP protocol server declaration
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-16
   *
   *	net4cpp 2.1
   *	File Transfer Protocol
   *******************************************************************/

#ifndef __YY_FTP_SERVER_H__
#define __YY_FTP_SERVER_H__

#include "ftpdef.h"
#include "socketSvr.h"

namespace net4cpp21
{

	class cFtpsvr
	{
	public:
		class cFtpSession //an FTP client session info object
		{
			cFtpsvr *m_psvr; //指向cFtpsvr的指针
		public:
			FTPACCOUNT *m_paccount;//此session关联的accountinfo，如果登录success则m_paccount关联specified的accountinfo
			socketTCP * m_pcmdsock;//命令传输socket
			socketTCP m_datasock;//data传输socket
			time_t m_tmLogin;//登录time
			char m_dataMode;//data传输模式 S-stream C-compressed B-Block
			char m_dataconnMode; //data传输模式 FTP_DATACONN_PORT/FTP_DATACONN_PASV
			char m_dataType;//后续data传输使用的datatype
					// A--ascII，E-EBCDIC文本 I-IMAGE 一系列8位字节表示的原始二进制data
					// L-LOCAL 使用可变字节size的原始二进制data
			char m_sslMode;//SSLdata传输加密模式 'C' -- 不加密data传输通道 'P'加密data传输通道
			char m_opMode;//currentdata操作动作 
						//L--LIST m_filename指向要list的目录/文件
						//S--STOR文件上载 m_filename指向上载的filename
						//R--RETRfile download m_filename指向file download的filename
			std::string m_filename;//临时存储current操作文件/directory name
			long m_startPoint;//startdownload或上载文件的起始点
							//对于LIST操作此参数指明要list目录的permissions,如果为0说明m_filename为虚目录
			
			explicit cFtpSession(socketTCP *psock,cFtpsvr *psvr);
			~cFtpSession(){}
			cFtpsvr *pserver() const { return m_psvr; }
			const char *getvpath() const {return m_relativePath.c_str();}

			SOCKSRESULT setvpath(const char *vpath);//设置current虚目录
			SOCKSRESULT getRealPath(std::string &vpath);
			SOCKSRESULT ifvpath(std::string &vpath);//是否为设置的虚目录
			void list();//List子虚目录
		private:
			std::string m_relativePath;//current虚目录path,!!!最后一个字符为/
			std::string m_realPath;//current虚目录path对应的真实path
			long m_iAccess;//currentpath对应的操作permissions

			const char *cvtRelative2Absolute(std::string &vpath);
			long cvtVPath2RPath(std::string &vpath);
		};

	public:
		cFtpsvr();
		virtual ~cFtpsvr(){}
		void setHelloTip(const char *strTip){
			if(strTip) m_helloTip.assign(strTip);
			return;
		}
		//specifiedFTP服务的data传输port的范围[startport,endport]
		//如果设为[0,0]则由系统自动随机分配port
		//如果设为[startport,0],则分配的port>=startport
		//如果设为[0,endport],则分配的port<=endport
		void setDataPort(int startport,int endport)
		{
			if( (m_dataport_start=startport)< 0) 
				m_dataport_start=0;
			if( (m_dataport_end=endport)< 0) 
				m_dataport_end=0;
			return;
		}
		
	protected:
		//get account info, return the specified account object
		FTPACCOUNT *getAccount(const char *struser);
		//添加新accountinfo
		FTPACCOUNT *newAccount(const char *struser);
		SOCKSRESULT delAccount(const char *struser);
		//有一个新的客户connect此服务
		void onConnect(socketTCP *psock,time_t tmOpened,unsigned long curConnection,unsigned long maxConnection);
		//currentconnect数大于current设定的maximum connections
		void onManyClient(socketTCP *psock);
		
		virtual bool onNewTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return false;
		}
		//扩展命令handle,如果返回真则是可识别的扩展命令，否则为不可识别的扩展命令
		virtual bool onCommandEx(socketTCP *psock,const char *strCommand
			,cFtpSession &clientSession)
		{
			return false;
		}
		virtual void onLogEvent(long eventID,cFtpSession &session)
		{
			return;
		}

	private:
		void parseCommand(cFtpSession &clientSession,socketTCP *psock
									  ,const char *ptrCommand);
		bool docmd_user(socketTCP *psock,const char *strUser,cFtpSession &clientSession);
		void docmd_type(socketTCP *psock,const char *strType,cFtpSession &clientSession);
		void docmd_cwd(socketTCP *psock,const char *strDir,cFtpSession &clientSession);
		void docmd_mkd(socketTCP *psock,const char *strDir,cFtpSession &clientSession);
		unsigned long docmd_rmd(socketTCP *psock,const char *strDir,cFtpSession &clientSession);
		unsigned long  docmd_dele(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_rnfr(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_rnto(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_size(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_rest(socketTCP *psock,const char *strRest,cFtpSession &clientSession);
		void docmd_prot(socketTCP *psock,const char *strParam,cFtpSession &clientSession);
		void docmd_list(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_retr(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_stor(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_pswd(socketTCP *psock,const char *strpwd,cFtpSession &clientSession);
		void docmd_port(socketTCP *psock,char *strParam,cFtpSession &clientSession);
		
		void docmd_pasv(socketTCP *psock,cFtpSession &clientSession);
		void docmd_pwd(socketTCP *psock,cFtpSession &clientSession);
		void docmd_abor(socketTCP *psock,cFtpSession &clientSession);
		void docmd_cdup(socketTCP *psock,cFtpSession &clientSession);
		void docmd_rein(socketTCP *psock,cFtpSession &clientSession);
		void docmd_sitelist(socketTCP *psock,cFtpSession &clientSession);
		void docmd_authssl(socketTCP *psock);
		void docmd_feat(socketTCP *psock);
		void docmd_quit(socketTCP *psock);

		void resp_noLogin(socketTCP *psock);
		void resp_OK(socketTCP *psock);
		void resp_noImplement(socketTCP *psock);
		void resp_unknowed(socketTCP *psock);

	private:
		std::string m_helloTip;
		int m_dataport_start; //specifiedFTP服务的data传输port的范围
		int m_dataport_end;	//如果[0,0]则随机分配，否则按照specified的区间分配port
		//此FTP服务的accountinfo
		std::map<std::string,FTPACCOUNT> m_accounts;

		static void dataTask(cFtpSession *psession);
	};

	class ftpServer : public socketSvr,public cFtpsvr
	{
	public:
		ftpServer();
		virtual ~ftpServer();
	private:
		//当有一个新的客户connect此服务触发此函数
		virtual void onAccept(socketTCP *psock)
		{
			cFtpsvr::onConnect(psock,m_tmOpened,curConnection(),maxConnection());
			return;
		}
		//如果currentconnect数大于current设定的maximum connections则触发此事件
		virtual void onTooMany(socketTCP *psock)
		{
			cFtpsvr::onManyClient(psock);
			return;
		}
		virtual bool onNewTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return (m_threadpool.addTask(pfunc,pargs,THREADLIVETIME)!=0);
		}
	};
}//?namespace net4cpp21

#endif
