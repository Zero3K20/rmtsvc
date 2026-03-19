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
#define _SUPPORT_TLSCLIENT_ //Define this macro to enable SSL support
                            //TLSClient source: https://github.com/zero3k/tardsplaya
                            //Windows SChannel is used for server-side TLS
#endif
#ifdef _SUPPORT_TLSCLIENT_
	// Forward declaration for tls_client (defined in tlsclient_source.cpp)
	class tls_client;
	// Windows SChannel / SSPI for server-side TLS
	#ifndef SECURITY_WIN32
	#define SECURITY_WIN32
	#endif
	#include <wincrypt.h>
	#include <sspi.h>
	#include <schannel.h>
	#include <vector>
	#pragma comment(lib, "Crypt32.lib")
	#pragma comment(lib, "Secur32.lib")
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

#ifdef _SUPPORT_TLSCLIENT_
	class socketSSL : public socketTcp
	{
	public:
		socketSSL();
		socketSSL(socketSSL &sockSSL);
		socketSSL & operator = (socketSSL &sockSSL);
		virtual ~socketSSL();

		virtual void Close();
		bool ifSSL() const { return m_ssltype != SSL_INIT_NONE; }
		bool ifSSLVerify() const { return m_bSSLverify; } //Whether SSL service requires client certificate verification
		//Set the SSL certificate private key password
		//bNotfile -- indicates whether strCaCert&strCaKey point to certificate file names or certificate content
		//If bNotfile=true and strCaCert or strCaKey is empty, default certificate and private key are used
		void setCacert(const char *strCaCert,const char *strCaKey,const char *strCaKeypwd,bool bNotfile,
					   const char *strCaRootFile=NULL,const char *strCRLfile=NULL);
		void setCacert(socketSSL *psock,bool bOnlyCopyCert);
		//Perform SSL/TLS handshake after connecting or accepting a connection; returns true on success
		bool SSL_Associate();
		//Initialize SSL/TLS; bInitServer specifies whether to initialize server or client side
		//If psock!=NULL, use psock's certificate to initialize the SSL server
		bool initSSL(bool bInitServer,socketSSL *psock=NULL);
		//Override Connect to capture the hostname for TLS SNI
		virtual SOCKSRESULT Connect(const char *host,int port,time_t lWaitout=-1);

		SOCKSRESULT Accept(time_t lWaitout,socketSSL *psock)
		{
			SOCKSRESULT sr=socketTcp::Accept(lWaitout,psock);
			if(sr>0 && psock){
				psock->m_ssltype=SSL_INIT_NONE;
				// Share SChannel credentials with accepted socket for server handshake
				if(m_ssltype==SSL_INIT_SERV && m_credValid){
					psock->m_hCred = m_hCred;
					psock->m_credValid = true;
					psock->m_credOwned = false; // accepted socket doesn't own the credential
					psock->m_ssltype = SSL_INIT_SERV;
					// Copy cert info in case it needs to re-init
					psock->m_cacert = m_cacert;
					psock->m_cakey = m_cakey;
					psock->m_cakeypass = m_cakeypass;
					psock->m_bNotfile = m_bNotfile;
				}
			}
			return sr;
		}

	protected:
		virtual size_t v_read(char *buf,size_t buflen);
		//!!! TLSClient/SChannel peek: reads into internal buffer without consuming
		virtual size_t v_peek(char *buf,size_t buflen);
		virtual size_t v_write(const char *buf,size_t buflen);
		void freeSSL();
	private:
		SSL_INIT_TYPE m_ssltype; //SSL initialization type

		// Client mode: TLSClient
		tls_client *m_tlsclient;
		std::string m_tlsHostname; // stored hostname for SNI

		// Server mode: Windows SChannel
		CredHandle m_hCred;
		CtxtHandle m_hCtx;
		SecPkgContext_StreamSizes m_streamSizes;
		bool m_credValid;
		bool m_ctxValid;
		bool m_credOwned;        // whether this object owns m_hCred
		std::vector<BYTE> m_rawRecvBuf;  // raw encrypted data buffer
		int  m_rawRecvLen;               // bytes in m_rawRecvBuf
		std::vector<BYTE> m_pendingDecrypt; // decrypted plaintext ready to read
		int  m_pendingOffset;            // read offset into m_pendingDecrypt

		//SSL service certificate, private key, and private key password
		std::string m_cacert;  //SSL certificate
		std::string m_cakey;   //SSL private key
		std::string m_cakeypass; //SSL private key password
		bool m_bNotfile; //Indicates whether m_cacert&m_cakey point to certificate file names or certificate strings
		bool m_bSSLverify; //Whether SSL verifies client certificates
		std::string m_carootfile; //CA root certificate for verifying client certificate authenticity
		std::string m_crlfile; //CRL list file
		PCCERT_CONTEXT m_pCertContext; // loaded certificate context (for server mode)

		// SChannel server handshake helper
		bool doSchannelHandshake();
		// Load PEM-encoded certificate/key helpers
		static bool DecodePemToDer(const char* pem, const char* beginMarker, std::vector<BYTE>& der);
		static PCCERT_CONTEXT LoadCertFromPemString(const char* pem);
		static PCCERT_CONTEXT LoadCertFromPemFile(const char* filename);
		static bool DecryptPemKey(const char* pemKey, const char* password, std::vector<BYTE>& derKey);
		static bool ImportPrivateKeyToCert(PCCERT_CONTEXT pCert, const std::vector<BYTE>& derKey);
	};
	
	typedef socketSSL socketTCP;
#else
	typedef socketTcp socketTCP;
#endif
}//?namespace net4cpp21

#endif
