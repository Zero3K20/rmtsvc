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
			cFtpsvr *m_psvr; //pointer to cFtpsvr
		public:
			FTPACCOUNT *m_paccount;//account info associated with this session; if login succeeds, m_paccount points to the specified account info
			socketTCP * m_pcmdsock;//command transfer socket
			socketTCP m_datasock;//data transfer socket
			time_t m_tmLogin;//login time
			char m_dataMode;//datatransfer模式 S-stream C-compressed B-Block
			char m_dataconnMode; //datatransfer模式 FTP_DATACONN_PORT/FTP_DATACONN_PASV
			char m_dataType;//continuation datatransfer使用的datatype
					// A--ascII，E-EBCDIC文本 I-IMAGE 一系列8bitbyte表示的原始二进制data
					// L-LOCAL 使用可变bytesize的原始二进制data
			char m_sslMode;//SSL data transfer encryption mode: 'C' -- no encryption, 'P' -- encrypted data transfer
			char m_opMode;//currentdata操作动作 
						//L--LIST m_filenamepointer to要list的directory/file
						//S--STORfile上载 m_filenamepointer to上载的filename
						//R--RETRfile download m_filenamepointer tofile download的filename
			std::string m_filename;//temporary存储current操作file/directory name
			long m_startPoint;//startdownloador上载file的起始点
							//对于LIST操作此parameterspecifies要listdirectory的permissions,if为0说明m_filename为虚directory
			
			explicit cFtpSession(socketTCP *psock,cFtpsvr *psvr);
			~cFtpSession(){}
			cFtpsvr *pserver() const { return m_psvr; }
			const char *getvpath() const {return m_relativePath.c_str();}

			SOCKSRESULT setvpath(const char *vpath);//setcurrent虚directory
			SOCKSRESULT getRealPath(std::string &vpath);
			SOCKSRESULT ifvpath(std::string &vpath);//whether为set的虚directory
			void list();//list sub-virtual directories
		private:
			std::string m_relativePath;//current虚directorypath,!!!last一个character为/
			std::string m_realPath;//current虚directorypathcorrespondingtrue实path
			long m_iAccess;//currentpathcorresponding操作permissions

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
		//specifiedFTPservice的datatransferport的范围[startport,endport]
		//if设为[0,0]则由systemauto随机分配port
		//if设为[startport,0],则分配的port>=startport
		//if设为[0,endport],则分配的port<=endport
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
		//add new account info
		FTPACCOUNT *newAccount(const char *struser);
		SOCKSRESULT delAccount(const char *struser);
		//a new client connected to this service
		void onConnect(socketTCP *psock,time_t tmOpened,unsigned long curConnection,unsigned long maxConnection);
		//currentconnect数大于current设定的maximum connections
		void onManyClient(socketTCP *psock);
		
		virtual bool onNewTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return false;
		}
		//扩展commandhandle,ifreturntrue则yes可识别的扩展command，otherwise为not可识别的扩展command
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
		int m_dataport_start; //specifiedFTPservice的datatransferport的范围
		int m_dataport_end;	//if[0,0]则随机分配，otherwise按照specified的区间分配port
		//此FTPservice的accountinfo
		std::map<std::string,FTPACCOUNT> m_accounts;

		static void dataTask(cFtpSession *psession);
	};

	class ftpServer : public socketSvr,public cFtpsvr
	{
	public:
		ftpServer();
		virtual ~ftpServer();
	private:
		//triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock)
		{
			cFtpsvr::onConnect(psock,m_tmOpened,curConnection(),maxConnection());
			return;
		}
		//triggered when current connection count exceeds the configured maximum connections
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
