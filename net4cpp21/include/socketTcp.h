/*******************************************************************
   *	socketTcp.h
   *    DESCRIPTION:TCP socket class definition
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-02
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SOCKET_TCP_H__
#define __YY_SOCKET_TCP_H__

#ifndef _NOSSL_D
#define _SUPPORT_OPENSSL_ //Define this macro to enable SSL support
						//Also add the TLSSC header path to the compiler options:
						//<net4cpp2.1 directory>/tlssc
#endif
#ifdef _SUPPORT_OPENSSL_
	#include "tlssc/tls_server.h"
	#pragma comment( lib, "ws2_32.lib" )
#endif
#include "socketBase.h"

namespace net4cpp21
{
	class socketTcp : public socketBase
	{
	public:
		socketTcp(){}
		socketTcp(socketTcp &sockTcp):socketBase(sockTcp){ return; }
		socketTcp & operator = (socketTcp &sockTcp) { socketBase::operator = (sockTcp); return *this; }
		virtual ~socketTcp(){}

		//Connect to the specified TCP service. Returns local socket port on success, otherwise error
		virtual SOCKSRESULT Connect(const char *host,int port,time_t lWaitout=-1)
		{
			SOCKSRESULT iret=(host)?setRemoteInfo(host,port):SOCKSERR_OK;
			if(iret==SOCKSERR_OK) iret=Connect(lWaitout,0,NULL);
			return iret;
		}
		virtual SOCKSRESULT Accept(time_t lWaitout,socketTcp *psock);
		//TCP listen; returns the listening service port
		SOCKSRESULT ListenX(int port,BOOL bReuseAddr,const char *bindIP);
		SOCKSRESULT ListenX(int startport,int endport,BOOL bReuseAddr,const char *bindIP);
		SOCKSRESULT Connect(const char *host,int port,int bindport,const char *bindip)
		{
			SOCKSRESULT iret=(host)?setRemoteInfo(host,port):SOCKSERR_OK;
			if(iret==SOCKSERR_OK) iret=Connect(-1,bindport,bindip);
			return iret;
		}
	protected:
		//Returns local socket port on success, otherwise error
//		SOCKSRESULT Connect(time_t lWaitout); //yyc remove 2007-08-07
		SOCKSRESULT Connect(time_t lWaitout,int bindport,const char *bindip); //yyc add 2007-08-07
	};

#ifdef _SUPPORT_OPENSSL_
	class socketSSL : public socketTcp
	{
	public:
		socketSSL();
		socketSSL(socketSSL &sockSSL);
		socketSSL & operator = (socketSSL &sockSSL);
		virtual ~socketSSL();

		virtual void Close();
		bool ifSSL() const { return m_ssltype!=SSL_INIT_NONE; }
		bool ifSSLVerify() const { return false; } //TLSSC does not support client certificate verification
		//Set the TLS certificate and private key.
		//bNotfile=true with NULL cert/key: use built-in default P-256 certificate.
		//bNotfile=false: strCaCert is path to DER certificate file,
		//                strCaKey is path to raw 32-byte P-256 private key file (or PEM EC key file).
		//bNotfile=true, non-NULL: strCaCert is PEM or DER certificate data,
		//                         strCaKey is PEM EC private key data or raw 32-byte key data.
		//strCaKeypwd, strCaRootFile, strCRLfile are accepted for API compatibility but ignored.
		void setCacert(const char *strCaCert,const char *strCaKey,const char *strCaKeypwd,bool bNotfile,
					   const char *strCaRootFile=NULL,const char *strCRLfile=NULL);
		void setCacert(socketSSL *psock,bool bOnlyCopyCert);
		//Perform TLS handshake after connecting or accepting a connection; returns true on success
		bool SSL_Associate();
		//Initialize SSL; bInitServer specifies whether to initialize server or client side
		//If psock!=NULL, use psock's certificate to initialize the SSL server
		bool initSSL(bool bInitServer,socketSSL *psock=NULL);

		//Override Connect to capture hostname for SNI
		virtual SOCKSRESULT Connect(const char *host,int port,time_t lWaitout=-1)
		{
			if(host) m_sni_host.assign(host);
			return socketTcp::Connect(host,port,lWaitout);
		}
		
		SOCKSRESULT Accept(time_t lWaitout,socketSSL *psock)
		{
			SOCKSRESULT sr=socketTcp::Accept(lWaitout,psock);
			if(sr>0 && psock){
				psock->m_ssltype=SSL_INIT_SERV;
				psock->m_cert_der=this->m_cert_der;
				memcpy(psock->m_privkey,this->m_privkey,32);
				psock->m_has_cert=this->m_has_cert;
			}
			return sr;
		}

	protected:
		virtual size_t v_read(char *buf,size_t buflen);
		virtual size_t v_peek(char *buf,size_t buflen);
		virtual size_t v_write(const char *buf,size_t buflen);
		void freeSSL();
	private:
		SSL_INIT_TYPE m_ssltype;       //TLS initialization type
		tls_client      *m_tls_client; //client-side TLS connection
		tls_server_conn *m_tls_server; //server-side TLS connection
		//Certificate and key storage (server-side)
		std::vector<unsigned char> m_cert_der; //DER-encoded X.509 certificate
		unsigned char m_privkey[32];           //raw 32-byte P-256 private key
		bool m_has_cert;                       //true if cert/key have been loaded
		//Peek buffer for v_peek implementation
		char m_peekbuf[4096];
		int  m_peeklen;
		//Remote hostname for SNI
		std::string m_sni_host;
	};
	
	typedef socketSSL socketTCP;
#else
	typedef socketTcp socketTCP;
#endif
}//?namespace net4cpp21

#endif

