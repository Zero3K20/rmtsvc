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

#include "include/sysconfig.h"
#include "include/socketTcp.h"
#include "include/cLogger.h"

using namespace std;
using namespace net4cpp21;

//TCP listen; returns the listening service port
SOCKSRESULT socketTcp::ListenX(int port,BOOL bReuseAddr,const char *bindIP)
{
	//Create a TCP socket handle
	if( !create(SOCKS_TCP) ) return SOCKSERR_INVALID;
	SOCKSRESULT sr=Bind(port,bReuseAddr,bindIP);
	if(sr<=0){ Close(); return sr; }

	if( ::listen(m_sockfd, SOMAXCONN ) == SOCKET_ERROR)
	{ 
		m_errcode=SOCK_M_GETERROR; 
		Close(); 
		return SOCKSERR_LISTEN; 
	}
	m_sockstatus=SOCKS_LISTEN;
	return sr;
}
SOCKSRESULT socketTcp::ListenX(int startport,int endport,BOOL bReuseAddr,const char *bindIP)
{
	//Create a TCP socket handle
	if( !create(SOCKS_TCP) ) return SOCKSERR_INVALID;
	SOCKSRESULT sr=Bind(startport,endport,bReuseAddr,bindIP);
	if(sr<=0){ Close(); return sr; }

	if( ::listen(m_sockfd, SOMAXCONN ) == SOCKET_ERROR)
	{ 
		m_errcode=SOCK_M_GETERROR; 
		Close(); 
		return SOCKSERR_LISTEN; 
	}
	m_sockstatus=SOCKS_LISTEN;
	return sr;
}
//Wait for an incoming connection; returns the current connection port
//If psock==NULL, close the current listen service and accept the connection on this socket
//Otherwise accept the connection on the specified socketTcp
SOCKSRESULT socketTcp::Accept(time_t lWaitout,socketTcp *psock)
{
	if( m_sockstatus!=SOCKS_LISTEN ) return SOCKSERR_INVALID;
	int fd=1;
	if(lWaitout>=0)
	{
		time_t t=time(NULL);
		while( (fd=checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ))== 0 )
		{//Check if the handle is readable
			if( (time(NULL)-t)>lWaitout ) return SOCKSERR_TIMEOUT; //Check if timeout has been reached
		}//?while
	}//?if(lWaitout>=0)
	if(fd!=1)
	{
		if(fd==-1) m_errcode=SOCK_M_GETERROR;
		return fd; //An error occurred
	}

	SOCKADDR_IN addr; int addrlen = sizeof(addr);
	fd=::accept(m_sockfd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
	if(fd==-1) {m_errcode=SOCK_M_GETERROR; return SOCKSERR_ERROR;} //System error occurred
	if(psock)
		psock->m_parent=this;
	else psock=this;
	psock->Close(); 
	psock->m_sockfd=fd; 
	psock->m_socktype=SOCKS_TCP;
	psock->m_sockstatus=SOCKS_CONNECTED;
	psock->m_sockflag |=SOCKS_TCP_IN;
	memcpy((void *)&psock->m_remoteAddr,(const void *)&addr,sizeof(SOCKADDR_IN));
	return psock->getSocketInfo();//Get the local IP and port bound to this socket
}

SOCKSRESULT socketTcp::Connect(time_t lWaitout,int bindport,const char *bindip)
{
	if(m_remoteAddr.sin_addr.s_addr==INADDR_NONE) 
		return SOCKSERR_HOST;//Invalid host IP
	//Create a TCP socket handle
	if( !create(SOCKS_TCP) ) return SOCKSERR_INVALID;	
	//Bind the specified IP and port
	if(bindport>0 || (bindip!=NULL && bindip[0]!=0) ) 
		if( Bind(bindport,SO_REUSEADDR,bindip)<=0) return SOCKSERR_BIND;
	SOCKSRESULT sr=SOCKSERR_OK;
	//Connect to the specified host
	if(lWaitout>=0) //Connection timeout has been configured
	{
		setNonblocking(true); //Set socket to non-blocking mode
		sr=::connect(m_sockfd,(struct sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr));
		//Non-blocking mode usually does not return 0; it returns an error instead
		if(sr==SOCKSERR_OK || SOCK_M_GETERROR==WSAEWOULDBLOCK )
		{
			time_t t=time(NULL);
			while( (sr=checkSocket(SCHECKTIMEOUT,SOCKS_OP_WRITE))== 0 )
			{//Check if the handle is writable
				if( (time(NULL)-t)>(unsigned long)lWaitout ) break; //Check if timeout has been reached
			}//?while
			if(sr==0)
				sr=SOCKSERR_TIMEOUT;
			else if(sr>0)
				sr=SOCKSERR_OK;	
		}//?if(::connect(...
		else sr=SOCKSERR_CONN;
		setNonblocking(false);//Restore to blocking mode
	}//?if(lWaitout>=0)
	else if(::connect(m_sockfd,(struct sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr))!=0) //Connection failed
	{
		RW_LOG_DEBUG("Failed to connect(), error=%d\r\n",SOCK_M_GETERROR);
		sr=SOCKSERR_CONN;
	}

	if(sr==SOCKSERR_OK){
		m_sockstatus=SOCKS_CONNECTED;
		m_sockflag &=(~SOCKS_TCP_IN);
		sr=getSocketInfo();
	}//?if(sr==SOCKSERR_OK)
	else Close();
	if(sr==SOCKSERR_ERROR) m_errcode=SOCK_M_GETERROR;
	return sr;
}

//-----------------------------socketSSL--------------------------------
#ifdef _SUPPORT_OPENSSL_

static char default_cacert[]="-----BEGIN CERTIFICATE-----\n"
					"MIIDgzCCAuygAwIBAgIBADANBgkqhkiG9w0BAQQFADCBjjELMAkGA1UEBhMCQ04x\n"
					"EDAOBgNVBAgTB2JlaWppbmcxEDAOBgNVBAcTB3hpY2hlbmcxDTALBgNVBAoTBGhv\n"
					"bWUxDTALBgNVBAsTBGhvbWUxGDAWBgNVBAMTD3l5Y25ldC55ZWFoLm5ldDEjMCEG\n"
					"CSqGSIb3DQEJARYUeXljbWFpbEAyNjMuc2luYS5jb20wHhcNMDUwNDA3MDExMzI3\n"
					"WhcNMTUwNDA1MDExMzI3WjCBjjELMAkGA1UEBhMCQ04xEDAOBgNVBAgTB2JlaWpp\n"
					"bmcxEDAOBgNVBAcTB3hpY2hlbmcxDTALBgNVBAoTBGhvbWUxDTALBgNVBAsTBGhv\n"
					"bWUxGDAWBgNVBAMTD3l5Y25ldC55ZWFoLm5ldDEjMCEGCSqGSIb3DQEJARYUeXlj\n"
					"bWFpbEAyNjMuc2luYS5jb20wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAOFb\n"
					"JLOA6+M4XVGS4L60ERmzzE6dIWZX3WSvsUnOsgGWEoaUBq24SKoWP5CbuX7+4awm\n"
					"N7DbBTW1TjdHV26yuo50kWnJBgkxKnwLcg+Ddhqdy3yLdlft6NsVmjd8BJ5i9GVt\n"
					"UatwiO4sTnSz2aA2vDb5esqUnJU99Y1dOiu7Uc/vAgMBAAGjge4wgeswHQYDVR0O\n"
					"BBYEFEtQsbWgZC5WSkGnybXMUVJV+jmrMIG7BgNVHSMEgbMwgbCAFEtQsbWgZC5W\n"
					"SkGnybXMUVJV+jmroYGUpIGRMIGOMQswCQYDVQQGEwJDTjEQMA4GA1UECBMHYmVp\n"
					"amluZzEQMA4GA1UEBxMHeGljaGVuZzENMAsGA1UEChMEaG9tZTENMAsGA1UECxME\n"
					"aG9tZTEYMBYGA1UEAxMPeXljbmV0LnllYWgubmV0MSMwIQYJKoZIhvcNAQkBFhR5\n"
					"eWNtYWlsQDI2My5zaW5hLmNvbYIBADAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEB\n"
					"BAUAA4GBAGEEmmxsYUpvTIRJ3gLWp+IoDqqyldkfV9PkLhDpUePs3viTg0WkxQla\n"
					"JFlMslz/HAdZ/GPXcLsAJqeMKzWQq4EXOH3AJ8VEd089zmd8xf8n8dKID8WNovgR\n"
					"b/ko8Vo1D2Mrm2u2yTd0ZYR7NQhsUInQLIUrnznMN2ryEhoaA21A\n"
					"-----END CERTIFICATE-----\n";
static char default_cakey[] ="-----BEGIN RSA PRIVATE KEY-----\n"
					"Proc-Type: 4,ENCRYPTED\n"
					"DEK-Info: DES-EDE3-CBC,CB70670238B9A46F\n"
					"\n"
					"4VDw50tA8qVC3wHHhVirFUMIfvYsF5seRDZCi/YDhg7FykteEa/ksQxImcc83xD+\n"
					"M64Xg2H9PssLVh/cnOcB4IiZumV7vqmFD/L/DO2HdlGPmp/mCtpFeT4z0arqhiel\n"
					"fevG4xzgI1Ns8THTtHK+3oLejliFXruViaLw+Zg9mYHCPPlHD/4kORcPoptyI3hS\n"
					"CJrvsWlxTxua1VyOXpaZlYvxrYKV9Wsd0XT9BeRXbiMrW4qL1cZ2KienwukvBCKx\n"
					"E/uesEk1j4h5gc0Y2IQ69hss8cMcM1BVE9coMoWBRPWESgO1pd0EXKkqfl4wpIJJ\n"
					"C0kImYvnRHwStJ+zlDpmwWPZtGUZRkj+2pQGtWJlwkJKmSIksqqF91AOIIoMN4ql\n"
					"iWViV3ys4dH/stJGjU+Be8EnvcIyoPEZ1rrTK6QPjcjn7xiyg5PxT6zm3F2E04jj\n"
					"K+qiwj+KBbjMoUQom0IirwSPSfNVswQm3/BQ/2R/U/Ugps2Ze/AAUZ0ogVkpRZAM\n"
					"sIvPxWDayVjQ5xHuEzfe4AEYq7i+G51T+jJcDXJ+7mJPNTcuG3tMdYK2TWZeYsuO\n"
					"EfctWaw6AS7CtzsozaY3VGykOhtHewRYCQGcz0Sqn/33u+ALfaaaQ41pzs4JnBgv\n"
					"U5DI0zmatjKb5gNNG95FVF1l1hyCBx19j4npozsbvh97/uQjiI3G2+6rH7maNCil\n"
					"yBdzhUkMuVT21OtmwynHkGXzd5YhTTZ6sUaqfCCie1GfmJ5ImI8Vcqmlb6sn8Q29\n"
					"O0noKSLb9spUVIW9pqQ/kEPPodt4fpPeiFsamtwH9DEqfbNco/IVVg==\n"
					"-----END RSA PRIVATE KEY-----\n";
static char default_cakeypass[]="123456";

socketSSL :: socketSSL():
	m_ctx(NULL),m_ssl(NULL),m_ssltype(SSL_INIT_NONE)
{
	 m_bNotfile=true;
	 m_bSSLverify=false;
	 m_carootfile=""; //Root CA PEM file for SSL server to verify client certificates
	 m_crlfile="";
}

socketSSL :: socketSSL(socketSSL &sockSSL) :socketTcp(sockSSL)
{
	m_ssltype=sockSSL.m_ssltype;
	m_ctx=sockSSL.m_ctx;
	m_ssl=sockSSL.m_ssl;
	m_cacert=sockSSL.m_cacert;
	m_cakey=sockSSL.m_cakey;
	m_cakeypass=sockSSL.m_cakeypass;
	m_bNotfile=sockSSL.m_bNotfile;
	m_bSSLverify=sockSSL.m_bSSLverify;
	m_carootfile=sockSSL.m_carootfile;
	m_crlfile=sockSSL.m_crlfile;

	sockSSL.m_ssltype=SSL_INIT_NONE;
	sockSSL.m_ctx=NULL;
	sockSSL.m_ssl=NULL;
}

socketSSL & socketSSL :: operator = (socketSSL &sockSSL)
{
	socketTcp::operator = (sockSSL);
	m_ssltype=sockSSL.m_ssltype;
	m_ctx=sockSSL.m_ctx;
	m_ssl=sockSSL.m_ssl;
	m_cacert=sockSSL.m_cacert;
	m_cakey=sockSSL.m_cakey;
	m_cakeypass=sockSSL.m_cakeypass;
	m_bNotfile=sockSSL.m_bNotfile;
	m_bSSLverify=sockSSL.m_bSSLverify;
	m_carootfile=sockSSL.m_carootfile;
	m_crlfile=sockSSL.m_crlfile;

	sockSSL.m_ssltype=SSL_INIT_NONE;
	sockSSL.m_ctx=NULL;
	sockSSL.m_ssl=NULL;
	return *this;
}

socketSSL :: ~socketSSL()
{
	Close();
	freeSSL();
}


//Set the SSL certificate private key password
//bNotfile -- indicates whether strCaCert&strCaKey point to certificate file names or certificate content
//If bNotfile=true and strCaCert or strCaKey is empty, default certificate and private key are used
void socketSSL :: setCacert(const char *strCaCert,const char *strCaKey,const char *strCaKeypwd,
							bool bNotfile,const char *strCaRootFile,const char *strCRLfile)
{
	if( (m_bNotfile=bNotfile) && 
		(strCaCert==NULL || strCaCert[0]==0 || 
			strCaKey==NULL || strCaKey[0]==0) )
	{
		m_cacert.assign(default_cacert);
		m_cakey.assign(default_cakey);
		m_cakeypass.assign(default_cakeypass);
		m_carootfile="";
		m_crlfile="";
	}
	else
	{
		if(strCaCert) m_cacert.assign(strCaCert);
		if(strCaKey) m_cakey.assign(strCaKey);
		if(strCaKeypwd) m_cakeypass.assign(strCaKeypwd);
		if(strCaRootFile) m_carootfile.assign(strCaRootFile);
		else m_carootfile="";
		if(strCRLfile)	m_crlfile.assign(strCRLfile);
		else m_crlfile="";
	}
	//If carootfile!="" then SSL client verification is required
	if(m_carootfile!="") m_bSSLverify=true; else m_bSSLverify=false;
	return;
}
void socketSSL :: setCacert(socketSSL *psock,bool bOnlyCopyCert)
{
	if(psock==NULL) return;
	m_cacert=psock->m_cacert;
	m_cakey=psock->m_cakey;
	m_cakeypass=psock->m_cakeypass;
	m_bNotfile=psock->m_bNotfile;
	if(!bOnlyCopyCert){
		m_bSSLverify=psock->m_bSSLverify;
		m_carootfile=psock->m_carootfile;
		m_crlfile=psock->m_crlfile;
	}else{
		m_bSSLverify=false;
		m_carootfile="";
		m_crlfile="";
	}
	return;
}

void socketSSL :: Close()
{
	if(m_ssl) SSL_shutdown (m_ssl);  // send SSL/TLS close_notify 
	socketTcp::Close();
	if(m_ssl) SSL_free(m_ssl);  
	m_ssl=NULL; return;
}

//Perform SSL handshake/negotiation
bool socketSSL :: SSL_Associate()
{
	if(m_sockstatus!=SOCKS_CONNECTED) return false;
	if(m_ctx==NULL){
		RW_LOG_DEBUG(0,"[SSL] Must be init SSL\r\n");
		return false; 
	}
	if(m_ssl==NULL){
		if( (m_ssl = SSL_new (m_ctx))==NULL )
		{
			RW_LOG_DEBUG(0,"[SSL] failed to ssl_new()\r\n");
			return false;
		}
	}//?if(m_ssl==NULL)
	SSL_set_fd (m_ssl, m_sockfd);
	//SSL_CTX_set_timeout(m_ctx,1000); //default is 300ms
	if(m_ssltype==SSL_INIT_CLNT) //Client-side SSL
	{
		if(SSL_connect(m_ssl)!=-1)
		{
			/* --------------------------------------------------------------
			// Following two steps are optional and not required for
			// data exchange to be successful. 
			RW_LOG_DEBUG("SSL connection using %s\n", SSL_get_cipher (m_ssl));
			// Get server's certificate (note: beware of dynamic allocation) - opt 
			X509 *server_cert=SSL_get_peer_certificate (m_ssl);
			if(server_cert==NULL){	 
				RW_LOG_PRINT(LOGLEVEL_ERROR,0,"failed to Server certificate!\r\n");
				Close(); return SOCKSERR_SSL_ERROR; }
			char *str = X509_NAME_oneline (X509_get_subject_name (server_cert),0,0);
			if(str==NULL){ X509_free (server_cert); Close(); return SOCKSERR_SSL_ERROR; }
			OPENSSL_free (str);
			str = X509_NAME_oneline (X509_get_issuer_name  (server_cert),0,0);
			if(str==NULL){ X509_free (server_cert); Close(); return SOCKSERR_SSL_ERROR; }
			OPENSSL_free (str);
			// We could do all sorts of certificate verification stuff here before
			// deallocating the certificate.
			X509_free (m_server_cert); m_server_cert=NULL;
			// --------------------------------------------------- 
			// DATA EXCHANGE - Send a message and receive a reply. 
			// -------------------------------------------------------------*/
			return true;
		}//?if(SSL_connect(m_ssl)!=-1)
		else
			RW_LOG_DEBUG(0,"[SSL] failed to ssl_connect(),error=%d!\r\n",SSL_get_error(m_ssl,-1));
	}//?if(m_ssltype==SSL_INIT_CLNT)
	else
	{
		if(SSL_accept(m_ssl)!=-1)
		{
			RW_LOG_DEBUG("[SSL] SSL connection using %s\r\n",SSL_get_cipher (m_ssl));
			X509*    client_cert=SSL_get_peer_certificate (m_ssl);
			if (client_cert != NULL) 
			{
				RW_LOG_DEBUG(0,"[SSL] Client certificate:\r\n");
				char *str = X509_NAME_oneline (X509_get_subject_name (client_cert), 0, 0);
				if(str){
					RW_LOG_DEBUG("\t subject: %s\r\n",str);
					OPENSSL_free (str);
				}
				str = X509_NAME_oneline (X509_get_issuer_name  (client_cert), 0, 0);
				if(str){
					RW_LOG_DEBUG("\t issuer: %s\r\n",str);
					OPENSSL_free (str);
				}
				// We could do all sorts of certificate verification stuff here before
				//   deallocating the certificate. 
				X509_free (client_cert);
			} 
			else RW_LOG_DEBUG(0,"[SSL] Client does not have certificate!\r\n");
			return true;
		}//?if(SSL_accept(m_ssl)!=-1)
		else
		{
			RW_LOG_PRINT(LOGLEVEL_ERROR,"[SSL] failed to SSL_accept(),error=%d!\r\n",SSL_get_error(m_ssl,-1));
			long verify_error=SSL_get_verify_result(m_ssl);
			if (verify_error == X509_V_OK) return true;
			RW_LOG_PRINT(LOGLEVEL_ERROR,"[SSL] verify error:%s\n",X509_verify_cert_error_string(verify_error));
		}
	}//?if(...) else ...
	SSL_free(m_ssl); m_ssl=NULL;
	return false;
}

inline size_t socketSSL :: v_write(const char *buf,size_t buflen)
{
	size_t len=0;
	if(m_ssl)
		len = SSL_write (m_ssl, buf, buflen); //returns -1 on error
	else
		len=::send(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	return len;
}
inline size_t socketSSL :: v_read(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_ssl)
		len=SSL_read (m_ssl, buf, buflen); //returns -1 on error	
	else
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	return len;
}
//!!! SSL_peek modifies the socket readable flag after peeking; if checked via
//select to inspect the socket handle, it will always appear unreadable
inline size_t socketSSL :: v_peek(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_ssl)
		len=SSL_peek (m_ssl, buf, buflen); //returns -1 on error
	else
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_PEEK);
	return len;
}

//------------------------------------------------------------------
void socketSSL::freeSSL()
{
	if(m_ssltype!=SSL_INIT_NONE && m_ctx)
		SSL_CTX_free(m_ctx);
	m_ctx=NULL;
}
//Initialize SSL socket. bInitServer=true initializes server-side SSL; otherwise initializes client-side SSL
static int passwdcb( char * buf, int size, int rwflag, void * userdata )
{
	strcpy( buf , (const char *)userdata );
	return strlen( (const char *)userdata ); 
}
//Certificate verification callback; additional processing can be done here...
static int verify_callback(int ok, X509_STORE_CTX *ctx)
{
	if(ok) //Perform CRL verification for successfully verified certificates
	{
		X509 *ok_cert=X509_STORE_CTX_get_current_cert(ctx);
	}
	return ok;
}
//If psock!=NULL, use psock's certificate to initialize the SSL server
bool socketSSL::initSSL(bool bInitServer,socketSSL *psock)
{
	if(m_ctx!=NULL) return true;
	m_ssltype=SSL_INIT_NONE;
	SSL_load_error_strings();//Prepare for printing debug information
						//After calling SSL_load_error_strings(), ERR_print_errors_fp() can be used at any time to print error messages
	SSLeay_add_ssl_algorithms();//Initialize
	//Specify the protocol to use (SSLv2/SSLv3/TLSv1) here
	SSL_METHOD *meth=(bInitServer)?SSLv23_server_method(): //TLSv1_server_method();
								   SSLv23_client_method(); //SSLv2_client_method();
	if( (m_ctx = SSL_CTX_new (meth))==NULL ) return false;

	const char *strCacert=(psock!=NULL)?psock->m_cacert.c_str():this->m_cacert.c_str();
	const char *strCakey=(psock!=NULL)?psock->m_cakey.c_str():this->m_cakey.c_str();
	const char *strCakeypass=(psock!=NULL)?psock->m_cakeypass.c_str():this->m_cakeypass.c_str();
	bool bNotfile =(psock!=NULL)?psock->m_bNotfile:this->m_bNotfile;
	if(bInitServer && (strCacert==NULL || strCacert[0]==0) ){ //Safety check: initialize server with default certificate
		strCacert=default_cacert; strCakey=default_cakey;
		strCakeypass=default_cakeypass; bNotfile=true;
	}
	
	//Initialize SSL: load certificate (public key) and private key
	if(strCakeypass && strCakeypass[0]!=0){//strCakeypass!="" if no private key password is specified, the user will be prompted to enter one
		SSL_CTX_set_default_passwd_cb(m_ctx,passwdcb);
		SSL_CTX_set_default_passwd_cb_userdata(m_ctx,(void *)strCakeypass); 
	}
	
	if(strCacert[0]!=0 && strCakey[0]!=0) //Certificate and private key are specified
	{
		if(strCakeypass[0]!=0){//strCakeypass!="" if no private key password is specified, the user will be prompted to enter one
			SSL_CTX_set_default_passwd_cb(m_ctx,passwdcb);
			SSL_CTX_set_default_passwd_cb_userdata(m_ctx,(void *)strCakeypass);
		}
		int ret=(bNotfile)? //Load certificate
			SSL_CTX_use_certificate_buf(m_ctx, strCacert, SSL_FILETYPE_PEM):
			SSL_CTX_use_certificate_file(m_ctx, strCacert, SSL_FILETYPE_PEM);
		if(!(ret>0)){
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] Failed to load certificate.\r\n");
			SSL_CTX_free (m_ctx);  m_ctx=NULL; return false;
		}
		ret=(bNotfile)? //Load private key
			SSL_CTX_use_PrivateKey_buf(m_ctx, strCakey, SSL_FILETYPE_PEM):
			SSL_CTX_use_PrivateKey_file(m_ctx, strCakey, SSL_FILETYPE_PEM);
		if(!(ret>0)){
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] Failed to load private key.\r\n");
			SSL_CTX_free (m_ctx);  m_ctx=NULL; return false;
		}
		if(!SSL_CTX_check_private_key(m_ctx)){
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] Private key and certificate do not match.\r\n");
			SSL_CTX_free (m_ctx);  m_ctx=NULL; return false;
		}
	}//Otherwise if initializing server-side, server cert and key must be specified; already guarded above
//	else if(bInitServer){ SSL_CTX_free (m_ctx); m_ctx=NULL; return false; }
	
	if(!bInitServer){ m_ssltype=SSL_INIT_CLNT; return true;}
	if(!m_bSSLverify){
		SSL_CTX_set_verify(m_ctx,SSL_VERIFY_NONE,NULL);
	}else{
		SSL_CTX_load_verify_locations(m_ctx, m_carootfile.c_str(), NULL);
		SSL_CTX_set_verify_depth(m_ctx,1);
		int mode=SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE|SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
		SSL_CTX_set_verify(m_ctx,mode,NULL); //verify_callback);
		//When client verification is required, the server sends trusted CA certificates from CAfile to the client.
		//If SSL_CTX_set_client_CA_list is not called, the client (IE) lists all installed certificates for the user
		//Otherwise only certificates verified by this CA are listed
		if(m_carootfile !="" )
			SSL_CTX_set_client_CA_list(m_ctx,SSL_load_client_CA_file(m_carootfile.c_str()));
		//To generate a session_id within the program itself, a session_id_context must be configured,
		//otherwise the program obtains session_id_context externally which is error-prone
		//Length must not exceed SSL_MAX_SSL_SESSION_ID_LENGTH
		//If SSL_CTX_set_session_id_context is not called, session resumption is disabled by default, causing every connection to perform
		//full certificate verification and handshake (slow for e.g. IE). Enabling sessions means after the first verification
		//the client retains a session and subsequent connections skip repeated verification/handshake
		static const unsigned char s_server_session_id_context[]="yyc1234";
		SSL_CTX_set_session_id_context(m_ctx,s_server_session_id_context,sizeof(s_server_session_id_context));

		if(m_crlfile!=""){ //Load CRL list
			X509_STORE *store=SSL_CTX_get_cert_store(m_ctx);
			X509_LOOKUP *lookup= X509_STORE_add_lookup(store, X509_LOOKUP_file());
			int iret=X509_load_crl_file(lookup, m_crlfile.c_str(), X509_FILETYPE_PEM);
			if(iret==1)
				X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK |X509_V_FLAG_CRL_CHECK_ALL);
			else RW_LOG_DEBUG("[SSL] Failed to load CRL %s\r\n",m_crlfile.c_str());
		}
	}
	m_ssltype=SSL_INIT_SERV;
	return true;
}

#endif

/*
SOCKSRESULT socketSSL::Accept(time_t lWaitout,socketSSL *psock)
{
	SOCKSRESULT sr=socketTcp::Accept(lWaitout,psock);
	if(sr>0 && psock)
	{
		psock->m_ssltype=SSL_INIT_NONE;
		if((psock->m_ctx=this->m_ctx)==NULL)
		{//psock may need to initialize SSL server-side, so copy the parent socketSSL certificate info to
		 //the accepted psock; if the parent socket already initialized SSL server-side, pass the ctx object directly
			psock->m_cacert=this->m_cacert;
			psock->m_cakey=this->m_cakey;
			psock->m_cakeypass=this->m_cakeypass;
			psock->m_bNotfile=this->m_bNotfile;
		}
	}//?if(sr>0 && psock)
	return sr;
}

  bool socketSSL::initSSL(bool bInitServer,socketSSL *psock)
{
	if(m_ctx!=NULL) return true;
	m_ssltype=SSL_INIT_NONE;
	SSL_load_error_strings();//Prepare for printing debug information
						//After calling SSL_load_error_strings(), ERR_print_errors_fp() can be used at any time to print error messages
	SSLeay_add_ssl_algorithms();//Initialize
	//Specify the protocol to use (SSLv2/SSLv3/TLSv1) here
	SSL_METHOD *meth=(bInitServer)?SSLv23_server_method(): //TLSv1_server_method();
								   SSLv23_client_method(); //SSLv2_client_method();
	if( (m_ctx = SSL_CTX_new (meth))==NULL ) return false;
	if(!bInitServer){ m_ssltype=SSL_INIT_CLNT; return true;}

	const char *strCacert=(psock!=NULL)?psock->m_cacert.c_str():this->m_cacert.c_str();
	const char *strCakey=(psock!=NULL)?psock->m_cakey.c_str():this->m_cakey.c_str();
	const char *strCakeypass=(psock!=NULL)?psock->m_cakeypass.c_str():this->m_cakeypass.c_str();
	bool bNotfile =(psock!=NULL)?psock->m_bNotfile:this->m_bNotfile;

	if(strCacert==NULL || strCacert[0]==0){ //Safety check: initialize with default certificate
		strCacert=default_cacert;
		strCakey=default_cakey;
		strCakeypass=default_cakeypass;
		bNotfile=true;
	} //yyc add 2006-11-23

	//Initialize SSL server: load certificate and private key
	if(strCakeypass && strCakeypass[0]!=0){//strCakeypass!="" if no private key password is specified, the user will be prompted to enter one
		SSL_CTX_set_default_passwd_cb(m_ctx,passwdcb);
		SSL_CTX_set_default_passwd_cb_userdata(m_ctx,(void *)strCakeypass); 
	}
	
	int ret=(bNotfile)?
			SSL_CTX_use_certificate_buf(m_ctx, strCacert, SSL_FILETYPE_PEM):
			SSL_CTX_use_certificate_file(m_ctx, strCacert, SSL_FILETYPE_PEM);

	if(ret>0)
	{//Certificate loaded successfully
		ret=(bNotfile)?
			SSL_CTX_use_PrivateKey_buf(m_ctx, strCakey, SSL_FILETYPE_PEM):
			SSL_CTX_use_PrivateKey_file(m_ctx, strCakey, SSL_FILETYPE_PEM);
		if(ret>0)
		{//Private key loaded successfully
			if(SSL_CTX_check_private_key(m_ctx))
			{
//				SSL_CTX_load_verify_locations(m_ctx, "cacert.pem", NULL);
//				SSL_CTX_set_verify_depth(m_ctx,1);
//				if(m_sslverifymode==SSL_VERIFY_PEER)
//					SSL_CTX_set_verify(m_ctx,SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE);
//				else if(m_sslverifymode==SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
//					SSL_CTX_set_verify(m_ctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_CLIENT_ONCE,NULL); //
//				else 
					SSL_CTX_set_verify(m_ctx,SSL_VERIFY_NONE,NULL);

				m_ssltype=SSL_INIT_SERV;
				return true;
			}
			else
				RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] Private key and certificate do not match.\r\n");
		}//?//Private key loaded successfully
		else
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] Failed to load private key.\r\n");
	}//Certificate loaded successfully
	else
		RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] Failed to load certificate.\r\n");
	SSL_CTX_free (m_ctx); 
	m_ctx=NULL; return false;
}

*/
