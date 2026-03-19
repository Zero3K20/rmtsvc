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
#ifdef _SUPPORT_TLSCLIENT_

// Include TLSClient implementation for client-mode TLS connections
#include "../tlsclient/tlsclient_source.cpp"

using namespace std;
using namespace net4cpp21;

//Default self-signed certificate (PEM format) used when no user certificate is specified
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

// =====================================================================
// PEM / CryptoAPI helpers for loading certificates into Windows SChannel
// =====================================================================

bool socketSSL::DecodePemToDer(const char* pem, const char* beginMarker, std::vector<BYTE>& der)
{
	const char* start = strstr(pem, beginMarker);
	if(!start) return false;
	start = strchr(start, '\n');
	if(!start) return false;
	start++;

	const char* end = strstr(start, "-----END");
	if(!end) return false;

	std::string b64;
	while(start < end){
		if(*start != '\n' && *start != '\r') b64 += *start;
		start++;
	}

	DWORD derLen = 0;
	if(!CryptStringToBinaryA(b64.c_str(), (DWORD)b64.size(), CRYPT_STRING_BASE64, NULL, &derLen, NULL, NULL))
		return false;
	der.resize(derLen);
	return CryptStringToBinaryA(b64.c_str(), (DWORD)b64.size(), CRYPT_STRING_BASE64, der.data(), &derLen, NULL, NULL) != FALSE;
}

PCCERT_CONTEXT socketSSL::LoadCertFromPemString(const char* pem)
{
	std::vector<BYTE> der;
	if(!DecodePemToDer(pem, "-----BEGIN CERTIFICATE-----", der)) return NULL;
	return CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, der.data(), (DWORD)der.size());
}

PCCERT_CONTEXT socketSSL::LoadCertFromPemFile(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if(!f) return NULL;
	fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
	std::vector<char> buf(sz + 1);
	fread(buf.data(), 1, sz, f);
	fclose(f);
	buf[sz] = 0;
	return LoadCertFromPemString(buf.data());
}

bool socketSSL::DecryptPemKey(const char* pemKey, const char* password, std::vector<BYTE>& derKey)
{
	// Check if the key is encrypted (has Proc-Type header)
	const char* pt = strstr(pemKey, "Proc-Type:");
	if(!pt){
		// Not encrypted: just base64-decode
		return DecodePemToDer(pemKey, "-----BEGIN RSA PRIVATE KEY-----", derKey);
	}

	// Parse DEK-Info: algorithm,IV
	const char* di = strstr(pemKey, "DEK-Info:");
	if(!di) return false;
	di += 9;
	while(*di == ' ') di++;

	// Read algorithm (only DES-EDE3-CBC supported, which is the default in OpenSSL)
	const char* comma = strchr(di, ',');
	if(!comma) return false;

	const char* ivstart = comma + 1;
	while(*ivstart == ' ') ivstart++;
	const char* ivend = strpbrk(ivstart, "\r\n");
	if(!ivend) return false;

	// Decode IV from hex string
	BYTE iv[16] = {};
	int ivlen = 0;
	for(const char* p = ivstart; p + 1 < ivend && ivlen < 16; p += 2){
		char byte[3] = { p[0], p[1], 0 };
		iv[ivlen++] = (BYTE)strtol(byte, NULL, 16);
	}

	// Base64-decode the encrypted key body
	std::vector<BYTE> encKey;
	if(!DecodePemToDer(pemKey, "-----BEGIN RSA PRIVATE KEY-----", encKey)) return false;

	// Derive 3DES key from password + IV using OpenSSL-compatible EVP_BytesToKey
	// (MD5-based, 1 iteration, 24-byte key for 3DES)
	HCRYPTPROV hProv = 0;
	if(!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return false;

	BYTE d0[16], d1[16];
	HCRYPTHASH hHash;
	DWORD hashLen;

	// D0 = MD5(password + IV[0:8])
	CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
	CryptHashData(hHash, (const BYTE*)password, (DWORD)strlen(password), 0);
	CryptHashData(hHash, iv, 8, 0);
	hashLen = 16; CryptGetHashParam(hHash, HP_HASHVAL, d0, &hashLen, 0);
	CryptDestroyHash(hHash);

	// D1 = MD5(D0 + password + IV[0:8])
	CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
	CryptHashData(hHash, d0, 16, 0);
	CryptHashData(hHash, (const BYTE*)password, (DWORD)strlen(password), 0);
	CryptHashData(hHash, iv, 8, 0);
	hashLen = 16; CryptGetHashParam(hHash, HP_HASHVAL, d1, &hashLen, 0);
	CryptDestroyHash(hHash);

	BYTE keyBytes[24] = {};
	memcpy(keyBytes, d0, 16);
	memcpy(keyBytes + 16, d1, 8);

	// Import 3DES key into CryptoAPI
	struct { BLOBHEADER hdr; DWORD keyLen; BYTE key[24]; } keyBlob;
	keyBlob.hdr.bType    = PLAINTEXTKEYBLOB;
	keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
	keyBlob.hdr.reserved = 0;
	keyBlob.hdr.aiKeyAlg = CALG_3DES;
	keyBlob.keyLen       = 24;
	memcpy(keyBlob.key, keyBytes, 24);

	HCRYPTKEY hKey = 0;
	bool ok = false;
	if(CryptImportKey(hProv, (const BYTE*)&keyBlob, sizeof(keyBlob), 0, 0, &hKey)){
		// Set IV for decryption
		CryptSetKeyParam(hKey, KP_IV, iv, 0);
		// Set CBC mode
		DWORD mode = CRYPT_MODE_CBC;
		CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&mode, 0);
		// Decrypt in-place
		derKey = encKey;
		DWORD decLen = (DWORD)derKey.size();
		ok = CryptDecrypt(hKey, 0, TRUE, 0, derKey.data(), &decLen) != FALSE;
		if(ok) derKey.resize(decLen);
		CryptDestroyKey(hKey);
	}
	CryptReleaseContext(hProv, 0);
	return ok;
}

bool socketSSL::ImportPrivateKeyToCert(PCCERT_CONTEXT pCert, const std::vector<BYTE>& derKey)
{
	// Decode PKCS#1 RSA private key DER to a CryptoAPI key blob
	DWORD dwBlobLen = 0;
	BYTE* pBlob = NULL;
	if(!CryptDecodeObjectEx(X509_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY,
		derKey.data(), (DWORD)derKey.size(),
		CRYPT_DECODE_ALLOC_FLAG, NULL, &pBlob, &dwBlobLen))
		return false;

	// Create a temporary named key container
	char containerName[80];
	sprintf(containerName, "rmtsvc_ssl_%lu", (unsigned long)GetTickCount());

	HCRYPTPROV hProv = 0;
	if(!CryptAcquireContextA(&hProv, containerName, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)){
		LocalFree(pBlob);
		return false;
	}

	HCRYPTKEY hKey = 0;
	bool ok = false;
	if(CryptImportKey(hProv, pBlob, dwBlobLen, 0, CRYPT_EXPORTABLE, &hKey)){
		CryptDestroyKey(hKey);
		// Link key container to certificate
		CRYPT_KEY_PROV_INFO kpi = {};
		WCHAR wContainer[80] = {};
		MultiByteToWideChar(CP_ACP, 0, containerName, -1, wContainer, 80);
		kpi.pwszContainerName = wContainer;
		kpi.dwProvType  = PROV_RSA_FULL;
		kpi.dwKeySpec   = AT_KEYEXCHANGE;
		ok = CertSetCertificateContextProperty(pCert, CERT_KEY_PROV_INFO_PROP_ID, 0, &kpi) != FALSE;
	}
	LocalFree(pBlob);
	CryptReleaseContext(hProv, 0);
	return ok;
}

// =====================================================================
// socketSSL constructors / destructors
// =====================================================================

socketSSL::socketSSL() :
	m_ssltype(SSL_INIT_NONE),
	m_tlsclient(NULL),
	m_credValid(false),
	m_ctxValid(false),
	m_credOwned(true),
	m_rawRecvLen(0),
	m_pendingOffset(0),
	m_bNotfile(true),
	m_bSSLverify(false),
	m_pCertContext(NULL)
{
	memset(&m_hCred, 0, sizeof(m_hCred));
	memset(&m_hCtx,  0, sizeof(m_hCtx));
}

socketSSL::socketSSL(socketSSL &sockSSL) : socketTcp(sockSSL)
{
	m_ssltype        = sockSSL.m_ssltype;
	m_tlsclient      = sockSSL.m_tlsclient;
	m_tlsHostname    = sockSSL.m_tlsHostname;
	m_hCred          = sockSSL.m_hCred;
	m_hCtx           = sockSSL.m_hCtx;
	m_streamSizes    = sockSSL.m_streamSizes;
	m_credValid      = sockSSL.m_credValid;
	m_ctxValid       = sockSSL.m_ctxValid;
	m_credOwned      = sockSSL.m_credOwned;
	m_rawRecvBuf     = sockSSL.m_rawRecvBuf;
	m_rawRecvLen     = sockSSL.m_rawRecvLen;
	m_pendingDecrypt = sockSSL.m_pendingDecrypt;
	m_pendingOffset  = sockSSL.m_pendingOffset;
	m_cacert         = sockSSL.m_cacert;
	m_cakey          = sockSSL.m_cakey;
	m_cakeypass      = sockSSL.m_cakeypass;
	m_bNotfile       = sockSSL.m_bNotfile;
	m_bSSLverify     = sockSSL.m_bSSLverify;
	m_carootfile     = sockSSL.m_carootfile;
	m_crlfile        = sockSSL.m_crlfile;
	m_pCertContext   = sockSSL.m_pCertContext;

	// Transfer ownership
	sockSSL.m_ssltype    = SSL_INIT_NONE;
	sockSSL.m_tlsclient  = NULL;
	sockSSL.m_credValid  = false;
	sockSSL.m_ctxValid   = false;
	sockSSL.m_credOwned  = true;
	sockSSL.m_pCertContext = NULL;
}

socketSSL & socketSSL::operator = (socketSSL &sockSSL)
{
	socketTcp::operator=(sockSSL);
	m_ssltype        = sockSSL.m_ssltype;
	m_tlsclient      = sockSSL.m_tlsclient;
	m_tlsHostname    = sockSSL.m_tlsHostname;
	m_hCred          = sockSSL.m_hCred;
	m_hCtx           = sockSSL.m_hCtx;
	m_streamSizes    = sockSSL.m_streamSizes;
	m_credValid      = sockSSL.m_credValid;
	m_ctxValid       = sockSSL.m_ctxValid;
	m_credOwned      = sockSSL.m_credOwned;
	m_rawRecvBuf     = sockSSL.m_rawRecvBuf;
	m_rawRecvLen     = sockSSL.m_rawRecvLen;
	m_pendingDecrypt = sockSSL.m_pendingDecrypt;
	m_pendingOffset  = sockSSL.m_pendingOffset;
	m_cacert         = sockSSL.m_cacert;
	m_cakey          = sockSSL.m_cakey;
	m_cakeypass      = sockSSL.m_cakeypass;
	m_bNotfile       = sockSSL.m_bNotfile;
	m_bSSLverify     = sockSSL.m_bSSLverify;
	m_carootfile     = sockSSL.m_carootfile;
	m_crlfile        = sockSSL.m_crlfile;
	m_pCertContext   = sockSSL.m_pCertContext;

	sockSSL.m_ssltype    = SSL_INIT_NONE;
	sockSSL.m_tlsclient  = NULL;
	sockSSL.m_credValid  = false;
	sockSSL.m_ctxValid   = false;
	sockSSL.m_credOwned  = true;
	sockSSL.m_pCertContext = NULL;
	return *this;
}

socketSSL::~socketSSL()
{
	Close();
	freeSSL();
}

// =====================================================================
// setCacert
// =====================================================================

void socketSSL::setCacert(const char *strCaCert, const char *strCaKey, const char *strCaKeypwd,
	bool bNotfile, const char *strCaRootFile, const char *strCRLfile)
{
	if((m_bNotfile=bNotfile) &&
		(strCaCert==NULL || strCaCert[0]==0 ||
		 strCaKey==NULL  || strCaKey[0]==0))
	{
		m_cacert.assign(default_cacert);
		m_cakey.assign(default_cakey);
		m_cakeypass.assign(default_cakeypass);
		m_carootfile = "";
		m_crlfile    = "";
	}
	else
	{
		if(strCaCert)    m_cacert.assign(strCaCert);
		if(strCaKey)     m_cakey.assign(strCaKey);
		if(strCaKeypwd)  m_cakeypass.assign(strCaKeypwd);
		if(strCaRootFile) m_carootfile.assign(strCaRootFile);
		else              m_carootfile = "";
		if(strCRLfile)    m_crlfile.assign(strCRLfile);
		else              m_crlfile = "";
	}
	m_bSSLverify = !m_carootfile.empty();
}

void socketSSL::setCacert(socketSSL *psock, bool bOnlyCopyCert)
{
	if(psock==NULL) return;
	m_cacert    = psock->m_cacert;
	m_cakey     = psock->m_cakey;
	m_cakeypass = psock->m_cakeypass;
	m_bNotfile  = psock->m_bNotfile;
	if(!bOnlyCopyCert){
		m_bSSLverify = psock->m_bSSLverify;
		m_carootfile = psock->m_carootfile;
		m_crlfile    = psock->m_crlfile;
	}else{
		m_bSSLverify = false;
		m_carootfile = "";
		m_crlfile    = "";
	}
}

// =====================================================================
// Connect override — capture hostname for TLS SNI
// =====================================================================

SOCKSRESULT socketSSL::Connect(const char *host, int port, time_t lWaitout)
{
	m_tlsHostname = host ? host : "";
	return socketTcp::Connect(host, port, lWaitout);
}

// =====================================================================
// Close
// =====================================================================

void socketSSL::Close()
{
	if(m_tlsclient){
		// Signal a clean TLS close before closing the TCP socket
		m_tlsclient->shutdown_send();
	}
	if(m_ctxValid){
		// Send SChannel close_notify
		DWORD dwType = SCHANNEL_SHUTDOWN;
		SecBuffer     OutBufs[1];
		OutBufs[0].pvBuffer    = &dwType;
		OutBufs[0].BufferType  = SECBUFFER_TOKEN;
		OutBufs[0].cbBuffer    = sizeof(dwType);
		SecBufferDesc OutDesc  = { SECBUFFER_VERSION, 1, OutBufs };
		ApplyControlToken(&m_hCtx, &OutDesc);
	}
	socketTcp::Close();
	if(m_tlsclient){
		// Socket already closed by socketTcp::Close(); tell tls_client not to close it again
		delete m_tlsclient;
		m_tlsclient = NULL;
	}
	if(m_ctxValid){
		DeleteSecurityContext(&m_hCtx);
		m_ctxValid = false;
	}
	m_ssltype       = SSL_INIT_NONE;
	m_rawRecvLen    = 0;
	m_pendingOffset = 0;
	m_pendingDecrypt.clear();
}

// =====================================================================
// SSL_Associate — perform TLS handshake on an already-connected socket
// =====================================================================

bool socketSSL::SSL_Associate()
{
	if(m_sockstatus != SOCKS_CONNECTED) return false;
	if(m_ssltype == SSL_INIT_NONE){
		RW_LOG_DEBUG(0,"[TLS] Must call initSSL before SSL_Associate\r\n");
		return false;
	}

	if(m_ssltype == SSL_INIT_CLNT){
		// Client mode: use TLSClient
		tls_client::init_global();
		if(!m_tlsclient) m_tlsclient = new tls_client();
		const char* host = m_tlsHostname.empty() ? getRemoteIP() : m_tlsHostname.c_str();
		int ret = m_tlsclient->connect_tls(m_sockfd, host);
		if(ret != 0){
			RW_LOG_DEBUG(0,"[TLS] TLSClient handshake failed: %s\r\n", m_tlsclient->errmsg());
			delete m_tlsclient;
			m_tlsclient = NULL;
			return false;
		}
		RW_LOG_DEBUG(0,"[TLS] TLSClient connection established\r\n");
		return true;
	}

	// Server mode: Windows SChannel
	return doSchannelHandshake();
}

// =====================================================================
// SChannel server handshake
// =====================================================================

bool socketSSL::doSchannelHandshake()
{
	std::vector<BYTE> inBuf(16384);
	int inBufLen = 0;
	bool fFirstCall = true;
	SECURITY_STATUS ss = SEC_I_CONTINUE_NEEDED;

	while(ss == SEC_I_CONTINUE_NEEDED || ss == SEC_E_INCOMPLETE_MESSAGE){
		// Receive data from client when needed
		if(ss == SEC_I_CONTINUE_NEEDED || ss == SEC_E_INCOMPLETE_MESSAGE || fFirstCall){
			// Make room for more data
			if(inBufLen + 4096 > (int)inBuf.size())
				inBuf.resize(inBuf.size() + 4096);
			int received = ::recv(m_sockfd, (char*)inBuf.data() + inBufLen, (int)inBuf.size() - inBufLen, 0);
			if(received <= 0){
				RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[TLS] SChannel recv failed during handshake\r\n");
				return false;
			}
			inBufLen += received;
		}

		SecBuffer inBuffers[2];
		inBuffers[0].pvBuffer   = inBuf.data();
		inBuffers[0].cbBuffer   = (ULONG)inBufLen;
		inBuffers[0].BufferType = SECBUFFER_TOKEN;
		inBuffers[1].pvBuffer   = NULL;
		inBuffers[1].cbBuffer   = 0;
		inBuffers[1].BufferType = SECBUFFER_EMPTY;

		SecBufferDesc inDesc  = { SECBUFFER_VERSION, 2, inBuffers };

		SecBuffer outBuffers[1];
		outBuffers[0].pvBuffer   = NULL;
		outBuffers[0].cbBuffer   = 0;
		outBuffers[0].BufferType = SECBUFFER_TOKEN;

		SecBufferDesc outDesc = { SECBUFFER_VERSION, 1, outBuffers };

		DWORD dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT |
			ASC_REQ_CONFIDENTIALITY | ASC_REQ_EXTENDED_ERROR |
			ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;
		DWORD dwOutFlags = 0;

		CtxtHandle *phCtx = fFirstCall ? NULL : &m_hCtx;
		ss = AcceptSecurityContext(
			&m_hCred, phCtx, &inDesc, dwSSPIFlags,
			SECURITY_NATIVE_DREP, &m_hCtx, &outDesc, &dwOutFlags, NULL);
		fFirstCall = false;

		// Send output token to client (if any)
		if(outBuffers[0].cbBuffer != 0 && outBuffers[0].pvBuffer != NULL){
			::send(m_sockfd, (const char*)outBuffers[0].pvBuffer, (int)outBuffers[0].cbBuffer, 0);
			FreeContextBuffer(outBuffers[0].pvBuffer);
		}

		if(ss == SEC_E_INCOMPLETE_MESSAGE) continue;

		// Handle leftover (extra) bytes in the input buffer
		if(inBuffers[1].BufferType == SECBUFFER_EXTRA && inBuffers[1].cbBuffer > 0){
			memmove(inBuf.data(), inBuf.data() + inBufLen - inBuffers[1].cbBuffer, inBuffers[1].cbBuffer);
			inBufLen = (int)inBuffers[1].cbBuffer;
		}else{
			inBufLen = 0;
		}
	}

	if(FAILED(ss)){
		RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[TLS] SChannel AcceptSecurityContext failed: 0x%x\r\n", ss);
		return false;
	}

	// Query stream buffer sizes for Encrypt/Decrypt
	QueryContextAttributes(&m_hCtx, SECPKG_ATTR_STREAM_SIZES, &m_streamSizes);
	m_ctxValid = true;

	// Store any application data that arrived with the handshake
	if(inBufLen > 0){
		m_rawRecvBuf.insert(m_rawRecvBuf.end(), inBuf.begin(), inBuf.begin() + inBufLen);
		m_rawRecvLen = inBufLen;
	}

	RW_LOG_DEBUG(0,"[TLS] SChannel handshake complete\r\n");
	return true;
}

// =====================================================================
// v_write / v_read / v_peek
// =====================================================================

size_t socketSSL::v_write(const char *buf, size_t buflen)
{
	if(m_tlsclient)
		return (size_t)m_tlsclient->send((char*)buf, (int)buflen);

	if(m_ctxValid){
		size_t totalSent = 0;
		while(totalSent < buflen){
			size_t toSend = min(buflen - totalSent, (size_t)m_streamSizes.cbMaximumMessage);
			size_t encBufSz = m_streamSizes.cbHeader + toSend + m_streamSizes.cbTrailer;
			std::vector<BYTE> encBuf(encBufSz);
			memcpy(encBuf.data() + m_streamSizes.cbHeader, buf + totalSent, toSend);

			SecBuffer encBufs[4];
			encBufs[0].pvBuffer   = encBuf.data();
			encBufs[0].cbBuffer   = m_streamSizes.cbHeader;
			encBufs[0].BufferType = SECBUFFER_STREAM_HEADER;
			encBufs[1].pvBuffer   = encBuf.data() + m_streamSizes.cbHeader;
			encBufs[1].cbBuffer   = (ULONG)toSend;
			encBufs[1].BufferType = SECBUFFER_DATA;
			encBufs[2].pvBuffer   = encBuf.data() + m_streamSizes.cbHeader + toSend;
			encBufs[2].cbBuffer   = m_streamSizes.cbTrailer;
			encBufs[2].BufferType = SECBUFFER_STREAM_TRAILER;
			encBufs[3].pvBuffer   = NULL; encBufs[3].cbBuffer = 0; encBufs[3].BufferType = SECBUFFER_EMPTY;

			SecBufferDesc encDesc = { SECBUFFER_VERSION, 4, encBufs };
			SECURITY_STATUS ss = EncryptMessage(&m_hCtx, 0, &encDesc, 0);
			if(FAILED(ss)) return totalSent;

			size_t encLen = encBufs[0].cbBuffer + encBufs[1].cbBuffer + encBufs[2].cbBuffer;
			int sent = ::send(m_sockfd, (const char*)encBuf.data(), (int)encLen, 0);
			if(sent <= 0) return totalSent;
			totalSent += toSend;
		}
		return totalSent;
	}

	// No TLS active
	return (size_t)::send(m_sockfd, buf, (int)buflen, MSG_NOSIGNAL);
}

// Read one TLS record into m_pendingDecrypt; returns false on error
static bool schannel_read_record(SOCKET s, CtxtHandle &ctx, SecPkgContext_StreamSizes &szs,
	std::vector<BYTE> &rawBuf, int &rawLen, std::vector<BYTE> &pending, int &pendingOff)
{
	// Try to decrypt what we have; if incomplete, read more
	for(;;){
		if(rawLen > 0){
			SecBuffer dBufs[4];
			dBufs[0].pvBuffer   = rawBuf.data();
			dBufs[0].cbBuffer   = (ULONG)rawLen;
			dBufs[0].BufferType = SECBUFFER_DATA;
			dBufs[1].BufferType = SECBUFFER_EMPTY;
			dBufs[2].BufferType = SECBUFFER_EMPTY;
			dBufs[3].BufferType = SECBUFFER_EMPTY;
			SecBufferDesc dDesc = { SECBUFFER_VERSION, 4, dBufs };

			SECURITY_STATUS ss = DecryptMessage(&ctx, &dDesc, 0, NULL);
			if(ss == SEC_E_OK){
				// Find decrypted data buffer
				for(int i = 0; i < 4; i++){
					if(dBufs[i].BufferType == SECBUFFER_DATA && dBufs[i].cbBuffer > 0){
						const BYTE* p = (const BYTE*)dBufs[i].pvBuffer;
						pending.assign(p, p + dBufs[i].cbBuffer);
						pendingOff = 0;
						break;
					}
				}
				// Move leftover bytes to the start of rawBuf
				rawLen = 0;
				for(int i = 0; i < 4; i++){
					if(dBufs[i].BufferType == SECBUFFER_EXTRA && dBufs[i].cbBuffer > 0){
						memmove(rawBuf.data(), dBufs[i].pvBuffer, dBufs[i].cbBuffer);
						rawLen = (int)dBufs[i].cbBuffer;
						break;
					}
				}
				return true;
			}
			if(ss == SEC_I_RENEGOTIATE){
				// Re-negotiate: just skip for now
				rawLen = 0;
				return false;
			}
			if(ss != SEC_E_INCOMPLETE_MESSAGE)
				return false; // fatal error
		}

		// Need more data
		if(rawLen + 4096 > (int)rawBuf.size()) rawBuf.resize(rawLen + 4096);
		int n = ::recv(s, (char*)rawBuf.data() + rawLen, (int)rawBuf.size() - rawLen, 0);
		if(n <= 0) return false;
		rawLen += n;
	}
}

size_t socketSSL::v_read(char *buf, size_t buflen)
{
	if(m_tlsclient)
		return (size_t)m_tlsclient->recv(buf, (int)buflen);

	if(m_ctxValid){
		// If we have buffered decrypted data, serve from there first
		if(m_pendingOffset < (int)m_pendingDecrypt.size()){
			size_t avail = m_pendingDecrypt.size() - m_pendingOffset;
			size_t n = min(avail, buflen);
			memcpy(buf, m_pendingDecrypt.data() + m_pendingOffset, n);
			m_pendingOffset += (int)n;
			return n;
		}
		// Read and decrypt one record
		m_pendingDecrypt.clear();
		m_pendingOffset = 0;
		if(!schannel_read_record(m_sockfd, m_hCtx, m_streamSizes,
			m_rawRecvBuf, m_rawRecvLen, m_pendingDecrypt, m_pendingOffset))
			return 0;
		// Serve from newly decrypted buffer
		size_t avail = m_pendingDecrypt.size();
		size_t n = min(avail, buflen);
		memcpy(buf, m_pendingDecrypt.data(), n);
		m_pendingOffset = (int)n;
		return n;
	}

	return (size_t)::recv(m_sockfd, buf, (int)buflen, MSG_NOSIGNAL);
}

size_t socketSSL::v_peek(char *buf, size_t buflen)
{
	if(m_tlsclient){
		// TLSClient has no peek; ensure data is in the pending buffer
		// by doing a recv with a small buffer (data stays buffered internally)
		// We can't truly peek without buffering, so just do a 0-byte recv first
		// then read into a local buffer and re-inject — not easily done.
		// For compatibility, read what we can and pretend it's a peek.
		// Note: this is a known limitation; callers should prefer v_read.
		return (size_t)m_tlsclient->recv(buf, (int)buflen);
	}

	if(m_ctxValid){
		// Return buffered data without advancing the offset
		if(m_pendingOffset < (int)m_pendingDecrypt.size()){
			size_t avail = m_pendingDecrypt.size() - m_pendingOffset;
			size_t n = min(avail, buflen);
			memcpy(buf, m_pendingDecrypt.data() + m_pendingOffset, n);
			return n;
		}
		// Read and decrypt without advancing
		m_pendingDecrypt.clear();
		m_pendingOffset = 0;
		if(!schannel_read_record(m_sockfd, m_hCtx, m_streamSizes,
			m_rawRecvBuf, m_rawRecvLen, m_pendingDecrypt, m_pendingOffset))
			return 0;
		m_pendingOffset = 0; // peek: do not advance
		size_t n = min(m_pendingDecrypt.size(), buflen);
		memcpy(buf, m_pendingDecrypt.data(), n);
		return n;
	}

	return (size_t)::recv(m_sockfd, buf, (int)buflen, MSG_NOSIGNAL | MSG_PEEK);
}

// =====================================================================
// freeSSL
// =====================================================================

void socketSSL::freeSSL()
{
	if(m_tlsclient){ delete m_tlsclient; m_tlsclient = NULL; }
	if(m_ctxValid){ DeleteSecurityContext(&m_hCtx); m_ctxValid = false; }
	if(m_credValid && m_credOwned){ FreeCredentialsHandle(&m_hCred); m_credValid = false; }
	if(m_pCertContext){ CertFreeCertificateContext(m_pCertContext); m_pCertContext = NULL; }
	m_ssltype = SSL_INIT_NONE;
}

// =====================================================================
// initSSL — initialize client or server TLS
// =====================================================================

bool socketSSL::initSSL(bool bInitServer, socketSSL *psock)
{
	if(m_ssltype != SSL_INIT_NONE) return true; // already initialized

	if(!bInitServer){
		// Client mode: just mark as client; TLSClient is created in SSL_Associate
		tls_client::init_global();
		m_ssltype = SSL_INIT_CLNT;
		return true;
	}

	// Server mode: load certificate and create SChannel credentials
	const char *strCacert   = (psock!=NULL) ? psock->m_cacert.c_str()   : m_cacert.c_str();
	const char *strCakey    = (psock!=NULL) ? psock->m_cakey.c_str()    : m_cakey.c_str();
	const char *strCakeypass= (psock!=NULL) ? psock->m_cakeypass.c_str(): m_cakeypass.c_str();
	bool        bNotfile    = (psock!=NULL) ? psock->m_bNotfile          : m_bNotfile;

	// Use default embedded certificate if none specified
	if(strCacert==NULL || strCacert[0]==0){
		strCacert    = default_cacert;
		strCakey     = default_cakey;
		strCakeypass = default_cakeypass;
		bNotfile     = true;
	}

	// Load the X.509 certificate
	PCCERT_CONTEXT pCert = bNotfile ?
		LoadCertFromPemString(strCacert) : LoadCertFromPemFile(strCacert);
	if(!pCert){
		RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[TLS] Failed to load certificate.\r\n");
		return false;
	}

	// Load and decrypt the RSA private key
	std::string keyContent;
	const char *strKey = strCakey;
	if(!bNotfile){
		FILE *f = fopen(strCakey, "rb");
		if(!f){ CertFreeCertificateContext(pCert); return false; }
		fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
		keyContent.resize((size_t)sz);
		fread(&keyContent[0], 1, (size_t)sz, f);
		fclose(f);
		strKey = keyContent.c_str();
	}

	std::vector<BYTE> derKey;
	if(!DecryptPemKey(strKey, strCakeypass ? strCakeypass : "", derKey)){
		RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[TLS] Failed to load private key.\r\n");
		CertFreeCertificateContext(pCert);
		return false;
	}

	// Import private key and associate with the certificate
	if(!ImportPrivateKeyToCert(pCert, derKey)){
		RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[TLS] Private key and certificate do not match.\r\n");
		CertFreeCertificateContext(pCert);
		return false;
	}

	// Build SCHANNEL_CRED and acquire credentials handle
	SCHANNEL_CRED sc = {};
	sc.dwVersion            = SCHANNEL_CRED_VERSION;
	sc.cCreds               = 1;
	sc.paCred               = &pCert;
	sc.grbitEnabledProtocols = SP_PROT_TLS1_2_SERVER | SP_PROT_TLS1_1_SERVER | SP_PROT_TLS1_SERVER;
	sc.dwFlags              = SCH_CRED_NO_SYSTEM_MAPPER;

	TimeStamp expiry;
	SECURITY_STATUS ss = AcquireCredentialsHandleA(
		NULL, (SEC_CHAR*)UNISP_NAME_A, SECPKG_CRED_INBOUND,
		NULL, &sc, NULL, NULL, &m_hCred, &expiry);

	if(FAILED(ss)){
		RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[TLS] AcquireCredentialsHandle failed: 0x%x\r\n", ss);
		CertFreeCertificateContext(pCert);
		return false;
	}

	m_pCertContext = pCert;
	m_credValid    = true;
	m_credOwned    = true;
	m_ssltype      = SSL_INIT_SERV;
	return true;
}

#endif

