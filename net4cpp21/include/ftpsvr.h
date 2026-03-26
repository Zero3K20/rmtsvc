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
			char m_dataMode;//data transfer mode: S-stream, C-compressed, B-Block
			char m_dataconnMode; //data transfer connection mode: FTP_DATACONN_PORT/FTP_DATACONN_PASV
			char m_dataType;//data type used for the continuation data transfer
					// A--ASCII, E-EBCDIC text, I-IMAGE binary data represented as a series of 8-bit bytes
					// L-LOCAL: raw binary data using variable byte size
			char m_sslMode;//SSL data transfer encryption mode: 'C' -- no encryption, 'P' -- encrypted data transfer
			char m_opMode;//current data operation mode
						//L--LIST: m_filename points to the directory/file to list
						//S--STOR: file upload, m_filename points to the upload filename
						//R--RETR: file download, m_filename points to the download filename
			std::string m_filename;//temporarily stores the current operation file/directory name
			long m_startPoint;//start download or upload file start point
							//for LIST operations, this parameter specifies the directory listing permissions; if 0, m_filename is a virtual directory
			
			explicit cFtpSession(socketTCP *psock,cFtpsvr *psvr);
			~cFtpSession(){}
			cFtpsvr *pserver() const { return m_psvr; }
			const char *getvpath() const {return m_relativePath.c_str();}

			SOCKSRESULT setvpath(const char *vpath);//set the current virtual directory
			SOCKSRESULT getRealPath(std::string &vpath);
			SOCKSRESULT ifvpath(std::string &vpath);//whether it is a configured virtual directory
			void list();//list sub-virtual directories
		private:
			std::string m_relativePath;//current virtual directory path; last character must be /
			std::string m_realPath;//actual path corresponding to the current virtual directory path
			long m_iAccess;//operation permissions corresponding to the current path

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
		//set the data transfer port range [startport, endport] for the FTP service
		//if set to [0,0], the system auto-assigns a random port
		//if set to [startport, 0], the assigned port >= startport
		//if set to [0, endport], the assigned port <= endport
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
		//currently connected clients exceeds the configured maximum connections
		void onManyClient(socketTCP *psock);
		
		virtual bool onNewTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return false;
		}
		//extended command handler; if returns true, the command is a recognized extended command; otherwise it is unrecognized
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
		int m_dataport_start; //specified data transfer port range for the FTP service
		int m_dataport_end;	//if [0,0], assign randomly; otherwise assign within the specified range
		//account info for this FTP service
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
