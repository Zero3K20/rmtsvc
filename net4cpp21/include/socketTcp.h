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
#define _SURPPORT_OPENSSL_ //Define this macro to enable SSL support
						//Also add the OpenSSL header and library paths to the compiler options:
						//<net4cpp2.1 directory>/OPENSSL
						//<net4cpp2.1 directory>/OPENSSL/lib
						//tools menu --> Options submenu --> directories page
#endif
#ifdef _SURPPORT_OPENSSL_
	#include <openssl/crypto.h>
	#include <openssl/x509.h>
	#include <openssl/pem.h>
	#include <openssl/ssl.h>
	#include <openssl/err.h>
    #pragma comment( lib, "libeay32lib" )
	#pragma comment( lib, "SSLeay32lib" )
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

#ifdef _SURPPORT_OPENSSL_
	class socketSSL : public socketTcp
	{
	public:
		socketSSL();
		socketSSL(socketSSL &sockSSL);
		socketSSL & operator = (socketSSL &sockSSL);
		virtual ~socketSSL();

		virtual void Close();
		bool ifSSL() const { return m_ctx!=NULL; }
		bool ifSSLVerify() const { return m_bSSLverify; } //Whether SSL service requires client certificate verification
		//Set the SSL certificate private key password
		//bNotfile -- indicates whether strCaCert&strCaKey point to certificate file names or certificate content
		//If bNotfile=true and strCaCert or strCaKey is empty, default certificate and private key are used
		void setCacert(const char *strCaCert,const char *strCaKey,const char *strCaKeypwd,bool bNotfile,
					   const char *strCaRootFile=NULL,const char *strCRLfile=NULL);
		void setCacert(socketSSL *psock,bool bOnlyCopyCert);
		//Perform SSL handshake after connecting or accepting a connection; returns true on success
		bool SSL_Associate();
		//Initialize SSL; bInitServer specifies whether to initialize server or client side
		//If psock!=NULL, use psock's certificate to initialize the SSL server
		bool initSSL(bool bInitServer,socketSSL *psock=NULL);
		
		SOCKSRESULT Accept(time_t lWaitout,socketSSL *psock)
		{
			SOCKSRESULT sr=socketTcp::Accept(lWaitout,psock);
			if(sr>0 && psock){
				psock->m_ssltype=SSL_INIT_NONE;
				psock->m_ctx=this->m_ctx;
			}
			return sr;
		}

	protected:
		virtual size_t v_read(char *buf,size_t buflen);
		//!!! SSL_peek modifies the socket readable flag after peeking; if checked via
		//select to inspect the socket handle, it will always appear unreadable
		virtual size_t v_peek(char *buf,size_t buflen);
		virtual size_t v_write(const char *buf,size_t buflen);
		void freeSSL();
	private:
		SSL_INIT_TYPE m_ssltype;//SSL initialization type
		SSL_CTX *m_ctx;
		SSL *    m_ssl;
		//SSL service certificate, private key, and private key password
		std::string m_cacert;//SSL certificate
		std::string m_cakey;//SSL private key
		std::string m_cakeypass;//SSL private key password
		bool m_bNotfile;//Indicates whether m_cacert&m_cakey point to certificate file names or certificate strings
		bool m_bSSLverify; //Whether SSL verifies client certificates
		std::string m_carootfile; //CA root certificate for verifying client certificate authenticity
		std::string m_crlfile; //CRL list file
	};
	
	typedef socketSSL socketTCP;
#else
	typedef socketTcp socketTCP;
#endif
}//?namespace net4cpp21

#endif

