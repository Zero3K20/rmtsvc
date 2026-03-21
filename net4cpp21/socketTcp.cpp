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

#include "include/cCoder.h"
#include <vector>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

// ---------------------------------------------------------------------------
// DER / ASN.1 builder helpers for runtime self-signed cert generation
// ---------------------------------------------------------------------------

static void cb_der_len(std::vector<unsigned char> &b, size_t n)
{
    if (n < 128) {
        b.push_back((unsigned char)n);
    } else if (n < 256) {
        b.push_back(0x81); b.push_back((unsigned char)n);
    } else {
        b.push_back(0x82);
        b.push_back((unsigned char)(n >> 8));
        b.push_back((unsigned char)(n & 0xFF));
    }
}
static void cb_der_tlv(std::vector<unsigned char> &b, unsigned char tag,
                        const unsigned char *data, size_t n)
{
    b.push_back(tag); cb_der_len(b, n);
    if (data && n) b.insert(b.end(), data, data + n);
}
static void cb_der_tlv(std::vector<unsigned char> &b, unsigned char tag,
                        const std::vector<unsigned char> &v)
{ cb_der_tlv(b, tag, v.data(), v.size()); }
static std::vector<unsigned char> cb_der_seq(const std::vector<unsigned char> &v)
{ std::vector<unsigned char> o; cb_der_tlv(o, 0x30, v); return o; }
static std::vector<unsigned char> cb_der_oid(const unsigned char *p, size_t n)
{ std::vector<unsigned char> o; cb_der_tlv(o, 0x06, p, n); return o; }
static std::vector<unsigned char> cb_der_int(const unsigned char *p, size_t n)
{
    while (n > 1 && p[0] == 0) { ++p; --n; }
    std::vector<unsigned char> v;
    if (p[0] & 0x80) v.push_back(0x00);
    v.insert(v.end(), p, p + n);
    std::vector<unsigned char> o; cb_der_tlv(o, 0x02, v); return o;
}
static std::vector<unsigned char> cb_ecdsa_sig_to_der(const unsigned char s[64])
{
    auto r = cb_der_int(s, 32); auto ss = cb_der_int(s+32, 32);
    std::vector<unsigned char> v; v.insert(v.end(),r.begin(),r.end());
    v.insert(v.end(),ss.begin(),ss.end()); return cb_der_seq(v);
}
static void cb_utctime(std::vector<unsigned char> &b, time_t t)
{
    struct tm *u = gmtime(&t); char s[14];
    snprintf(s, sizeof(s), "%02d%02d%02d%02d%02d%02dZ",
             u->tm_year%100, u->tm_mon+1, u->tm_mday,
             u->tm_hour, u->tm_min, u->tm_sec);
    cb_der_tlv(b, 0x17, (const unsigned char *)s, 13);
}

static const unsigned char CB_OID_EC[]        = {0x2a,0x86,0x48,0xce,0x3d,0x02,0x01};
static const unsigned char CB_OID_P256[]      = {0x2a,0x86,0x48,0xce,0x3d,0x03,0x01,0x07};
static const unsigned char CB_OID_ECSHA256[]  = {0x2a,0x86,0x48,0xce,0x3d,0x04,0x03,0x02};
static const unsigned char CB_OID_CN[]        = {0x55,0x04,0x03};
static const unsigned char CB_OID_SAN[]       = {0x55,0x1d,0x11}; // 2.5.29.17
static const unsigned char CB_OID_BC[]        = {0x55,0x1d,0x13}; // 2.5.29.19

// Generate a self-signed P-256 TLS certificate at runtime containing the
// machine's DNS hostname (and 'localhost' / 127.0.0.1) in the Subject
// Alternative Name extension so that modern browsers accept it.
// On success fills cert_der and privkey[32] and returns true.
// Self-signed certificate validity period (10 years in seconds).
static const long SELF_SIGNED_CERT_VALIDITY_SEC = 10L * 365 * 24 * 3600;

static bool gen_self_signed_cert(const char *hostname,
                                  std::vector<unsigned char> &cert_der,
                                  unsigned char privkey[32])
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    bool ok = false;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg,
            BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0)))
        return false;
    do {
        if (!BCRYPT_SUCCESS(BCryptGenerateKeyPair(hAlg, &hKey, 256, 0))) break;
        if (!BCRYPT_SUCCESS(BCryptFinalizeKeyPair(hKey, 0)))             break;

        ULONG pubSz = 0;
        if (!BCRYPT_SUCCESS(BCryptExportKey(hKey, NULL, BCRYPT_ECCPUBLIC_BLOB,
                NULL, 0, &pubSz, 0))) break;
        std::vector<unsigned char> pub((size_t)pubSz);
        if (!BCRYPT_SUCCESS(BCryptExportKey(hKey, NULL, BCRYPT_ECCPUBLIC_BLOB,
                pub.data(), pubSz, &pubSz, 0))) break;
        if (pubSz < 72) break;

        ULONG prvSz = 0;
        if (!BCRYPT_SUCCESS(BCryptExportKey(hKey, NULL, BCRYPT_ECCPRIVATE_BLOB,
                NULL, 0, &prvSz, 0))) break;
        std::vector<unsigned char> prv((size_t)prvSz);
        if (!BCRYPT_SUCCESS(BCryptExportKey(hKey, NULL, BCRYPT_ECCPRIVATE_BLOB,
                prv.data(), prvSz, &prvSz, 0))) break;
        if (prvSz < 104) break;
        memcpy(privkey, prv.data() + 72, 32);

        const unsigned char *pubX = pub.data() + 8;
        const unsigned char *pubY = pub.data() + 40;

        // version [0] { INTEGER 2 }
        std::vector<unsigned char> verIn = {0x02,0x01,0x02};
        std::vector<unsigned char> version; cb_der_tlv(version, 0xA0, verIn);

        time_t now = time(NULL);
        unsigned char ser[4] = {
            (unsigned char)((now>>24)&0xFF),(unsigned char)((now>>16)&0xFF),
            (unsigned char)((now>> 8)&0xFF),(unsigned char)( now     &0xFF) };
        auto serial = cb_der_int(ser, 4);
        auto sigAlg = cb_der_seq(cb_der_oid(CB_OID_ECSHA256, sizeof(CB_OID_ECSHA256)));

        // name = SEQUENCE { SET { SEQUENCE { OID CN, UTF8String(hostname) } } }
        auto cnOid = cb_der_oid(CB_OID_CN, sizeof(CB_OID_CN));
        std::vector<unsigned char> cnV;
        cb_der_tlv(cnV, 0x0C, (const unsigned char *)hostname, strlen(hostname));
        std::vector<unsigned char> attrI; attrI.insert(attrI.end(),cnOid.begin(),cnOid.end());
        attrI.insert(attrI.end(),cnV.begin(),cnV.end());
        std::vector<unsigned char> rdnS; cb_der_tlv(rdnS, 0x31, cb_der_seq(attrI));
        auto name = cb_der_seq(rdnS);

        std::vector<unsigned char> val;
        cb_utctime(val, now);
        cb_utctime(val, now + SELF_SIGNED_CERT_VALIDITY_SEC);
        auto validity = cb_der_seq(val);

        // SubjectPublicKeyInfo
        std::vector<unsigned char> algoI;
        auto ecOid = cb_der_oid(CB_OID_EC, sizeof(CB_OID_EC));
        auto p256   = cb_der_oid(CB_OID_P256, sizeof(CB_OID_P256));
        algoI.insert(algoI.end(),ecOid.begin(),ecOid.end());
        algoI.insert(algoI.end(),p256.begin(),p256.end());
        std::vector<unsigned char> pkB = {0x00,0x04};
        pkB.insert(pkB.end(),pubX,pubX+32); pkB.insert(pkB.end(),pubY,pubY+32);
        std::vector<unsigned char> pkBs; cb_der_tlv(pkBs,0x03,pkB);
        std::vector<unsigned char> spkiI;
        auto algoSeq = cb_der_seq(algoI);
        spkiI.insert(spkiI.end(),algoSeq.begin(),algoSeq.end());
        spkiI.insert(spkiI.end(),pkBs.begin(),pkBs.end());
        auto spki = cb_der_seq(spkiI);

        // subjectAltName: dNSName(hostname), dNSName(localhost), iPAddress(127.0.0.1)
        std::vector<unsigned char> sanSeq;
        cb_der_tlv(sanSeq, 0x82, (const unsigned char *)hostname, strlen(hostname));
        if (strcmp(hostname,"localhost") != 0)
            cb_der_tlv(sanSeq, 0x82, (const unsigned char *)"localhost", 9);
        unsigned char lo[4]={127,0,0,1}; cb_der_tlv(sanSeq,0x87,lo,4);
        auto sanVal = cb_der_seq(sanSeq);
        std::vector<unsigned char> sanOct; cb_der_tlv(sanOct,0x04,sanVal);
        std::vector<unsigned char> sanExtI;
        auto sanOid = cb_der_oid(CB_OID_SAN,sizeof(CB_OID_SAN));
        sanExtI.insert(sanExtI.end(),sanOid.begin(),sanOid.end());
        sanExtI.insert(sanExtI.end(),sanOct.begin(),sanOct.end());
        auto sanExt = cb_der_seq(sanExtI);

        // basicConstraints critical CA:TRUE
        std::vector<unsigned char> bcSeq = {0x01,0x01,0xff};
        auto bcVal = cb_der_seq(bcSeq);
        std::vector<unsigned char> bcOct; cb_der_tlv(bcOct,0x04,bcVal);
        std::vector<unsigned char> bcCrit={0x01,0x01,0xff};
        std::vector<unsigned char> bcExtI;
        auto bcOid = cb_der_oid(CB_OID_BC,sizeof(CB_OID_BC));
        bcExtI.insert(bcExtI.end(),bcOid.begin(),bcOid.end());
        bcExtI.insert(bcExtI.end(),bcCrit.begin(),bcCrit.end());
        bcExtI.insert(bcExtI.end(),bcOct.begin(),bcOct.end());
        auto bcExt = cb_der_seq(bcExtI);

        std::vector<unsigned char> extL;
        extL.insert(extL.end(),sanExt.begin(),sanExt.end());
        extL.insert(extL.end(),bcExt.begin(),bcExt.end());
        std::vector<unsigned char> extTag; cb_der_tlv(extTag,0xA3,cb_der_seq(extL));

        std::vector<unsigned char> tbsI;
        tbsI.insert(tbsI.end(),version.begin(),version.end());
        tbsI.insert(tbsI.end(),serial.begin(),serial.end());
        tbsI.insert(tbsI.end(),sigAlg.begin(),sigAlg.end());
        tbsI.insert(tbsI.end(),name.begin(),name.end());
        tbsI.insert(tbsI.end(),validity.begin(),validity.end());
        tbsI.insert(tbsI.end(),name.begin(),name.end());
        tbsI.insert(tbsI.end(),spki.begin(),spki.end());
        tbsI.insert(tbsI.end(),extTag.begin(),extTag.end());
        auto tbs = cb_der_seq(tbsI);

        unsigned char hash[32]={};
        bool hok=false;
        { BCRYPT_ALG_HANDLE hS=NULL; BCRYPT_HASH_HANDLE hH=NULL;
          if(BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hS,BCRYPT_SHA256_ALGORITHM,NULL,0))){
            if(BCRYPT_SUCCESS(BCryptCreateHash(hS,&hH,NULL,0,NULL,0,0))){
              if(BCRYPT_SUCCESS(BCryptHashData(hH,(PUCHAR)tbs.data(),(ULONG)tbs.size(),0))&&
                 BCRYPT_SUCCESS(BCryptFinishHash(hH,hash,32,0))) hok=true;
              BCryptDestroyHash(hH); }
            BCryptCloseAlgorithmProvider(hS,0); } }
        if(!hok) break;

        ULONG sigSz=0;
        if(!BCRYPT_SUCCESS(BCryptSignHash(hKey,NULL,hash,32,NULL,0,&sigSz,0))) break;
        std::vector<unsigned char> sig((size_t)sigSz);
        if(!BCRYPT_SUCCESS(BCryptSignHash(hKey,NULL,hash,32,sig.data(),sigSz,&sigSz,0))) break;
        if(sigSz!=64) break;

        auto sigDer = cb_ecdsa_sig_to_der(sig.data());
        std::vector<unsigned char> sigBs={0x00};
        sigBs.insert(sigBs.end(),sigDer.begin(),sigDer.end());
        std::vector<unsigned char> sigBsW; cb_der_tlv(sigBsW,0x03,sigBs);

        std::vector<unsigned char> certI;
        certI.insert(certI.end(),tbs.begin(),tbs.end());
        certI.insert(certI.end(),sigAlg.begin(),sigAlg.end());
        certI.insert(certI.end(),sigBsW.begin(),sigBsW.end());
        cert_der = cb_der_seq(certI);
        ok = true;
    } while(0);
    if(hKey) BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg,0);
    return ok;
}

// ---------------------------------------------------------------------------
// Helper: parse ALL certificates from a PEM chain file and return the
// intermediate certs (certs 2..N) concatenated with 3-byte BE length prefixes,
// ready to be appended verbatim to the TLS Certificate message.
// ---------------------------------------------------------------------------
static std::vector<unsigned char> pem_chain_extra(const char *pem)
{
    std::vector<unsigned char> result;
    const char *p = pem;
    int idx = 0;
    while ((p = strstr(p, "-----BEGIN CERTIFICATE-----")) != nullptr)
    {
        p += 27;
        const char *end = strstr(p, "-----END CERTIFICATE-----");
        if (!end) break;
        if (idx > 0) // skip the leaf cert (first one)
        {
            std::string b64;
            for (const char *q = p; q < end; q++)
            {
                char c = *q;
                if (c != '\n' && c != '\r' && c != ' ' && c != '\t') b64 += c;
            }
            int dlen = net4cpp21::cCoder::Base64DecodeSize((int)b64.size());
            if (dlen > 0)
            {
                std::vector<unsigned char> der((size_t)dlen);
                int actual = net4cpp21::cCoder::base64_decode(
                    (char *)b64.data(), (unsigned int)b64.size(),
                    (char *)der.data());
                if (actual > 0)
                {
                    der.resize((size_t)actual);
                    // 3-byte big-endian length prefix
                    result.push_back((unsigned char)(actual >> 16));
                    result.push_back((unsigned char)((actual >> 8) & 0xFF));
                    result.push_back((unsigned char)(actual & 0xFF));
                    result.insert(result.end(), der.begin(), der.end());
                }
            }
        }
        idx++;
        p = end + 25; // advance past "-----END CERTIFICATE-----"
    }
    return result;
}

// Default self-signed P-256 ECDSA certificate (DER-encoded X.509).
// Kept as a last-resort fallback in case BCrypt keygen fails at runtime.
// Generated with: openssl ecparam -name prime256v1 -genkey -noout -out k.key &&
//   openssl req -new -x509 -key k.key -days 3650 -subj "/CN=rmtsvc" -outform DER -out c.der
static const unsigned char default_cert_der[] = {
  0x30, 0x82, 0x01, 0x78, 0x30, 0x82, 0x01, 0x1d, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x14, 0x06, 0xa9, 0x5e, 0x1f, 0xd7, 0xa0, 0xeb, 0x8a, 0x7e,
  0x40, 0x02, 0x1e, 0xc6, 0x90, 0x13, 0xb5, 0x72, 0xa4, 0xf6, 0xde, 0x30,
  0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30,
  0x11, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x06,
  0x72, 0x6d, 0x74, 0x73, 0x76, 0x63, 0x30, 0x1e, 0x17, 0x0d, 0x32, 0x36,
  0x30, 0x33, 0x31, 0x39, 0x31, 0x39, 0x34, 0x32, 0x35, 0x33, 0x5a, 0x17,
  0x0d, 0x33, 0x36, 0x30, 0x33, 0x31, 0x36, 0x31, 0x39, 0x34, 0x32, 0x35,
  0x33, 0x5a, 0x30, 0x11, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04,
  0x03, 0x0c, 0x06, 0x72, 0x6d, 0x74, 0x73, 0x76, 0x63, 0x30, 0x59, 0x30,
  0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08,
  0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04,
  0x16, 0xf6, 0x72, 0x57, 0x28, 0xe7, 0x9d, 0x75, 0x9b, 0xe0, 0x4f, 0x61,
  0x0f, 0x1f, 0x1f, 0x09, 0x92, 0xff, 0xc8, 0xb6, 0x53, 0xbd, 0x5e, 0xc2,
  0x67, 0xf3, 0x93, 0x41, 0xdc, 0x5d, 0x50, 0x7a, 0x34, 0xc4, 0x1f, 0xd5,
  0x50, 0x46, 0x78, 0xbd, 0x88, 0x90, 0x89, 0xcd, 0xe0, 0xfc, 0x74, 0x80,
  0x9c, 0x36, 0xc7, 0xef, 0x06, 0xa8, 0x8d, 0x3e, 0x98, 0x32, 0x4f, 0xc1,
  0x71, 0x07, 0x2b, 0x48, 0xa3, 0x53, 0x30, 0x51, 0x30, 0x1d, 0x06, 0x03,
  0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x57, 0x7f, 0x30, 0x0e, 0x40,
  0xbd, 0x29, 0xc3, 0x06, 0x82, 0x53, 0x60, 0x5c, 0xec, 0x6d, 0xd0, 0x1f,
  0x3d, 0x5d, 0x0e, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18,
  0x30, 0x16, 0x80, 0x14, 0x57, 0x7f, 0x30, 0x0e, 0x40, 0xbd, 0x29, 0xc3,
  0x06, 0x82, 0x53, 0x60, 0x5c, 0xec, 0x6d, 0xd0, 0x1f, 0x3d, 0x5d, 0x0e,
  0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05,
  0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48,
  0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21,
  0x00, 0xc2, 0x42, 0x51, 0xa9, 0xf5, 0x2c, 0xcc, 0xc8, 0xfe, 0x3e, 0xcc,
  0x2f, 0x15, 0x79, 0x08, 0xce, 0x70, 0xad, 0xf7, 0x95, 0xe0, 0xb9, 0x6a,
  0x11, 0xcd, 0x32, 0x85, 0x70, 0x37, 0xfb, 0x0a, 0x19, 0x02, 0x21, 0x00,
  0xc6, 0xf2, 0xf9, 0xec, 0x60, 0xd3, 0x61, 0xe1, 0xbd, 0x63, 0xc0, 0x00,
  0x33, 0xb6, 0xaa, 0xd5, 0x0f, 0x52, 0xa9, 0xe2, 0x17, 0x33, 0x13, 0x44,
  0x93, 0x03, 0xf7, 0x19, 0xbb, 0x9f, 0x18, 0x65
};
static const int default_cert_der_len = (int)sizeof(default_cert_der);

// Default raw 32-byte P-256 private key (big-endian scalar).
static const unsigned char default_privkey[32] = {
  0x74, 0xef, 0x56, 0x6a, 0x05, 0xee, 0xf1, 0x99, 0x5b, 0x93, 0x13, 0xfc,
  0xc0, 0x00, 0x0e, 0xdd, 0xde, 0x94, 0x17, 0xd3, 0xbf, 0xf9, 0xe4, 0xc1,
  0x36, 0x06, 0x58, 0x98, 0x2a, 0x3e, 0x62, 0xd2
};

// ---------------------------------------------------------------------------
// Helper: convert a PEM certificate block to DER.
// Expects "-----BEGIN CERTIFICATE-----" ... "-----END CERTIFICATE-----".
// ---------------------------------------------------------------------------
static bool pem_cert_to_der(const char *pem, std::vector<unsigned char> &der)
{
const char *begin = strstr(pem, "-----BEGIN CERTIFICATE-----");
if(!begin) return false;
begin += 27; // length of "-----BEGIN CERTIFICATE-----"
const char *end = strstr(begin, "-----END CERTIFICATE-----");
if(!end) return false;

std::string b64;
for(const char *p = begin; p < end; p++){
char c = *p;
if(c != '\n' && c != '\r' && c != ' ' && c != '\t') b64 += c;
}
if(b64.empty()) return false;

int der_len = net4cpp21::cCoder::Base64DecodeSize((int)b64.length());
if(der_len <= 0) return false;
der.resize((size_t)der_len);
int actual = net4cpp21::cCoder::base64_decode((char*)b64.c_str(),
                                               (unsigned int)b64.length(),
                                               (char*)der.data());
if(actual <= 0) return false;
der.resize((size_t)actual);
return true;
}

// ---------------------------------------------------------------------------
// Helper: scan DER bytes for the SEC1 EC private key raw scalar.
// Looks for the ASN.1 pattern INTEGER(1) OCTET STRING(32) which marks the
// private key field in both SEC1 and PKCS#8 encoded P-256 keys.
// ---------------------------------------------------------------------------
static bool extract_ec_raw_key(const unsigned char *der, int len, unsigned char raw[32])
{
// Pattern: 02 01 01  04 20  [32 bytes]
//          INTEGER(1)  OCTET STRING(32)
for(int i = 0; i + 37 <= len; i++){
if(der[i]   == 0x02 && der[i+1] == 0x01 && der[i+2] == 0x01 &&
   der[i+3] == 0x04 && der[i+4] == 0x20){
memcpy(raw, der + i + 5, 32);
return true;
}
}
return false;
}

// ---------------------------------------------------------------------------
// Helper: convert a PEM EC private key block to a raw 32-byte scalar.
// Accepts "-----BEGIN EC PRIVATE KEY-----" (SEC1) and
//         "-----BEGIN PRIVATE KEY-----" (PKCS#8).
// ---------------------------------------------------------------------------
static bool pem_ec_key_to_raw(const char *pem, unsigned char raw[32])
{
const char *begin = strstr(pem, "-----BEGIN");
if(!begin) return false;
// Advance past the header line
while(*begin && *begin != '\n') begin++;
if(*begin) begin++;

const char *end = strstr(pem, "-----END");
if(!end) return false;

std::string b64;
for(const char *p = begin; p < end; p++){
char c = *p;
if(c != '\n' && c != '\r' && c != ' ' && c != '\t') b64 += c;
}
if(b64.empty()) return false;

int der_len = net4cpp21::cCoder::Base64DecodeSize((int)b64.length());
if(der_len <= 0) return false;
std::vector<unsigned char> der((size_t)der_len);
int actual = net4cpp21::cCoder::base64_decode((char*)b64.c_str(),
                                               (unsigned int)b64.length(),
                                               (char*)der.data());
if(actual <= 0) return false;
return extract_ec_raw_key(der.data(), actual, raw);
}

// ---------------------------------------------------------------------------
// Helper: load a certificate from a file (PEM or DER auto-detected).
// ---------------------------------------------------------------------------
static bool load_cert_file(const char *path, std::vector<unsigned char> &der)
{
FILE *f = fopen(path, "rb");
if(!f) return false;
fseek(f, 0, SEEK_END);
long sz = ftell(f);
fseek(f, 0, SEEK_SET);
if(sz <= 0){ fclose(f); return false; }

std::vector<char> buf((size_t)(sz + 1));
fread(buf.data(), 1, (size_t)sz, f);
fclose(f);
buf[(size_t)sz] = 0;

if(strstr(buf.data(), "-----BEGIN"))
return pem_cert_to_der(buf.data(), der);

// Assume DER
der.assign((unsigned char*)buf.data(), (unsigned char*)buf.data() + sz);
return !der.empty();
}

// ---------------------------------------------------------------------------
// Helper: load a P-256 private key from a file (PEM or raw 32 bytes).
// ---------------------------------------------------------------------------
static bool load_key_file(const char *path, unsigned char raw[32])
{
FILE *f = fopen(path, "rb");
if(!f) return false;
fseek(f, 0, SEEK_END);
long sz = ftell(f);
fseek(f, 0, SEEK_SET);
if(sz <= 0){ fclose(f); return false; }

std::vector<char> buf((size_t)(sz + 1));
fread(buf.data(), 1, (size_t)sz, f);
fclose(f);
buf[(size_t)sz] = 0;

if(sz == 32){
memcpy(raw, buf.data(), 32);
return true;
}
if(strstr(buf.data(), "-----BEGIN"))
return pem_ec_key_to_raw(buf.data(), raw);

// Try DER
return extract_ec_raw_key((unsigned char*)buf.data(), (int)sz, raw);
}

// ---------------------------------------------------------------------------
// socketSSL implementation
// ---------------------------------------------------------------------------

socketSSL :: socketSSL():
m_ssltype(SSL_INIT_NONE), m_tls_client(NULL), m_tls_server(NULL),
m_has_cert(false), m_has_fallback(false), m_peeklen(0)
{
memset(m_privkey, 0, 32);
memset(m_fallback_privkey, 0, 32);
}

socketSSL :: socketSSL(socketSSL &sockSSL) : socketTcp(sockSSL)
{
m_ssltype   = sockSSL.m_ssltype;
m_tls_client= sockSSL.m_tls_client;
m_tls_server= sockSSL.m_tls_server;
m_cert_der  = sockSSL.m_cert_der;
memcpy(m_privkey, sockSSL.m_privkey, 32);
m_has_cert  = sockSSL.m_has_cert;
m_chain_der = sockSSL.m_chain_der;
m_acme_domain = sockSSL.m_acme_domain;
m_fallback_cert_der = sockSSL.m_fallback_cert_der;
memcpy(m_fallback_privkey, sockSSL.m_fallback_privkey, 32);
m_has_fallback = sockSSL.m_has_fallback;
m_sni_host  = sockSSL.m_sni_host;
m_peeklen   = 0;

sockSSL.m_ssltype    = SSL_INIT_NONE;
sockSSL.m_tls_client = NULL;
sockSSL.m_tls_server = NULL;
}

socketSSL & socketSSL :: operator = (socketSSL &sockSSL)
{
socketTcp::operator = (sockSSL);
m_ssltype   = sockSSL.m_ssltype;
m_tls_client= sockSSL.m_tls_client;
m_tls_server= sockSSL.m_tls_server;
m_cert_der  = sockSSL.m_cert_der;
memcpy(m_privkey, sockSSL.m_privkey, 32);
m_has_cert  = sockSSL.m_has_cert;
m_chain_der = sockSSL.m_chain_der;
m_acme_domain = sockSSL.m_acme_domain;
m_fallback_cert_der = sockSSL.m_fallback_cert_der;
memcpy(m_fallback_privkey, sockSSL.m_fallback_privkey, 32);
m_has_fallback = sockSSL.m_has_fallback;
m_sni_host  = sockSSL.m_sni_host;
m_peeklen   = 0;

sockSSL.m_ssltype    = SSL_INIT_NONE;
sockSSL.m_tls_client = NULL;
sockSSL.m_tls_server = NULL;
return *this;
}

socketSSL :: ~socketSSL()
{
Close();
freeSSL();
}

//Set the TLS certificate and private key.
void socketSSL :: setCacert(const char *strCaCert, const char *strCaKey,
                             const char * /*strCaKeypwd*/, bool bNotfile,
                             const char * /*strCaRootFile*/, const char * /*strCRLfile*/)
{
if(bNotfile &&
   (strCaCert == NULL || strCaCert[0] == 0 ||
    strCaKey  == NULL || strCaKey[0]  == 0))
{
// Use built-in default P-256 certificate — generated at runtime so the
// machine's hostname is included in the Subject Alternative Name.
char hostname[256] = "localhost";
DWORD hsz = (DWORD)sizeof(hostname);
GetComputerNameExA(ComputerNameDnsHostname, hostname, &hsz);
std::vector<unsigned char> rtcert;
unsigned char rtkey[32] = {};
if(gen_self_signed_cert(hostname, rtcert, rtkey)){
    m_cert_der = rtcert;
    memcpy(m_privkey, rtkey, 32);
} else {
    m_cert_der.assign(default_cert_der, default_cert_der + default_cert_der_len);
    memcpy(m_privkey, default_privkey, 32);
}
m_chain_der.clear();
m_acme_domain.clear();
m_fallback_cert_der.clear();
m_has_fallback = false;
m_has_cert = true;
return;
}

if(!bNotfile){
// Load from files (DER cert + raw/PEM key)
std::vector<unsigned char> cert;
unsigned char key[32];
bool cert_ok = (strCaCert && strCaCert[0]) ? load_cert_file(strCaCert, cert) : false;
bool key_ok  = (strCaKey  && strCaKey[0])  ? load_key_file(strCaKey, key)   : false;
if(cert_ok && key_ok){
    m_cert_der = cert;
    memcpy(m_privkey, key, 32);
    m_has_cert = true;
    m_acme_domain.clear(); // caller (e.g. websvr) will set via setAcmeDomain() if needed
    // Extract intermediate chain certs from PEM file (for LE full-chain serving)
    m_chain_der.clear();
    if(strCaCert && strCaCert[0])
    {
        FILE *fc = fopen(strCaCert, "rb");
        if(fc){
            fseek(fc,0,SEEK_END); long fsz=ftell(fc); fseek(fc,0,SEEK_SET);
            if(fsz>0){
                std::vector<char> pbuf((size_t)(fsz+1),0);
                fread(pbuf.data(),1,(size_t)fsz,fc);
                if(strstr(pbuf.data(),"-----BEGIN"))
                    m_chain_der = pem_chain_extra(pbuf.data());
            }
            fclose(fc);
        }
    }
    // Also generate a self-signed fallback cert with the local machine hostname
    // so that LAN clients connecting via a hostname different from the ACME domain
    // get a cert that matches their SNI (avoiding ERR_CERT_COMMON_NAME_INVALID).
    char hostname[256] = "localhost";
    DWORD hsz = (DWORD)sizeof(hostname);
    GetComputerNameExA(ComputerNameDnsHostname, hostname, &hsz);
    std::vector<unsigned char> fbcert;
    unsigned char fbkey[32] = {};
    if(gen_self_signed_cert(hostname, fbcert, fbkey)){
        m_fallback_cert_der = fbcert;
        memcpy(m_fallback_privkey, fbkey, 32);
        m_has_fallback = true;
    } else {
        m_fallback_cert_der.clear();
        m_has_fallback = false;
    }
} else {
    RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[setCacert] Failed to load cert/key files.\r\n");
    // Fall back to runtime-generated cert with local hostname
    char hostname[256] = "localhost";
    DWORD hsz = (DWORD)sizeof(hostname);
    GetComputerNameExA(ComputerNameDnsHostname, hostname, &hsz);
    std::vector<unsigned char> rtcert;
    unsigned char rtkey[32] = {};
    if(gen_self_signed_cert(hostname, rtcert, rtkey)){
        m_cert_der = rtcert;
        memcpy(m_privkey, rtkey, 32);
    } else {
        m_cert_der.assign(default_cert_der, default_cert_der + default_cert_der_len);
        memcpy(m_privkey, default_privkey, 32);
    }
    m_chain_der.clear();
    m_acme_domain.clear();
    m_fallback_cert_der.clear();
    m_has_fallback = false;
    m_has_cert = true;
}
return;
}

// bNotfile=true with non-empty content: detect PEM vs binary
std::vector<unsigned char> cert;
unsigned char key[32];
bool cert_ok = false, key_ok = false;

if(strCaCert && strCaCert[0]){
if(strstr(strCaCert, "-----BEGIN"))
cert_ok = pem_cert_to_der(strCaCert, cert);
else {
// Treat as binary DER: scan until '\0' or rely on size
// We can't know the length from a char* alone; try PEM first
RW_LOG_PRINT(LOGLEVEL_ERROR, 0, "[setCacert] Non-PEM in-memory cert not supported; using default.\r\n");
}
}
if(strCaKey && strCaKey[0]){
if(strstr(strCaKey, "-----BEGIN"))
key_ok = pem_ec_key_to_raw(strCaKey, key);
else if(strlen(strCaKey) == 32){
memcpy(key, strCaKey, 32);
key_ok = true;
}
}
if(cert_ok && key_ok){
m_cert_der = cert;
memcpy(m_privkey, key, 32);
m_has_cert = true;
} else {
// Fall back to runtime-generated cert with local hostname
char hostname[256] = "localhost";
DWORD hsz = (DWORD)sizeof(hostname);
GetComputerNameExA(ComputerNameDnsHostname, hostname, &hsz);
std::vector<unsigned char> rtcert;
unsigned char rtkey[32] = {};
if(gen_self_signed_cert(hostname, rtcert, rtkey)){
    m_cert_der = rtcert; memcpy(m_privkey, rtkey, 32);
} else {
    m_cert_der.assign(default_cert_der, default_cert_der + default_cert_der_len);
    memcpy(m_privkey, default_privkey, 32);
}
m_chain_der.clear();
m_acme_domain.clear();
m_fallback_cert_der.clear();
m_has_fallback = false;
m_has_cert = true;
}
}

void socketSSL :: setCacert(socketSSL *psock, bool /*bOnlyCopyCert*/)
{
if(psock == NULL) return;
m_cert_der  = psock->m_cert_der;
memcpy(m_privkey, psock->m_privkey, 32);
m_has_cert  = psock->m_has_cert;
m_chain_der = psock->m_chain_der;
m_acme_domain = psock->m_acme_domain;
m_fallback_cert_der = psock->m_fallback_cert_der;
memcpy(m_fallback_privkey, psock->m_fallback_privkey, 32);
m_has_fallback = psock->m_has_fallback;
}

void socketSSL :: Close()
{
freeSSL();
m_sockfd = INVALID_SOCKET; // TLSSC already closed the socket; prevent double-close
socketTcp::Close();
}

//Perform TLS handshake/negotiation after TCP connect/accept
bool socketSSL :: SSL_Associate()
{
if(m_sockstatus != SOCKS_CONNECTED) return false;

tls_client::init_global();

if(m_ssltype == SSL_INIT_CLNT)
{
// Client-side: wrap the already-connected TCP socket
tls_client *cli = new tls_client;
const char *host = m_sni_host.empty() ? NULL : m_sni_host.c_str();
if(cli->wrap(m_sockfd, host) != 0)
{
RW_LOG_DEBUG(0, "[SSL] TLS client handshake failed: %s\r\n",
             cli->errmsg() ? cli->errmsg() : "");
cli->detach_socket(); // don't close m_sockfd
delete cli;
return false;
}
m_tls_client = cli;
return true;
}
else if(m_ssltype == SSL_INIT_SERV)
{
// Server-side: perform TLS handshake on the accepted socket
if(!m_has_cert){
// Fall back to runtime-generated cert with local hostname
char hostname[256] = "localhost";
DWORD hsz = (DWORD)sizeof(hostname);
GetComputerNameExA(ComputerNameDnsHostname, hostname, &hsz);
std::vector<unsigned char> rtcert;
unsigned char rtkey[32] = {};
if(gen_self_signed_cert(hostname, rtcert, rtkey)){
    m_cert_der = rtcert; memcpy(m_privkey, rtkey, 32);
} else {
    m_cert_der.assign(default_cert_der, default_cert_der + default_cert_der_len);
    memcpy(m_privkey, default_privkey, 32);
}
m_chain_der.clear();
m_fallback_cert_der.clear();
m_has_fallback = false;
m_has_cert = true;
}
tls_server_conn *srv = new tls_server_conn;
srv->init(m_sockfd,
          m_cert_der.data(), (int)m_cert_der.size(),
          m_privkey);
// Pass full certificate chain (leaf + intermediates) if available
if(!m_chain_der.empty())
    srv->set_chain(m_chain_der.data(), (int)m_chain_der.size());
// Pass the fallback self-signed cert for SNI-based cert selection
if(m_has_fallback && !m_acme_domain.empty())
    srv->set_fallback_cert(m_fallback_cert_der.data(), (int)m_fallback_cert_der.size(),
                           m_fallback_privkey, m_acme_domain.c_str());
const char *err = srv->handshake();
if(err != NULL)
{
// These errors are expected for port-scanners, probes, plain-HTTP clients
// hitting the HTTPS port, or clients that close mid-handshake.  Log them at
// WARN level so they appear without flooding the error log.
// All other errors (crypto failures, protocol bugs, etc.) are logged at ERROR.
// Use prefix matching (strncmp) because error strings now carry step and
// error-code context, e.g. "send failed [ServerHello] (WSAError=10054)".
bool expected = (strncmp(err, "timeout",              sizeof("timeout")              - 1) == 0 ||
                 strncmp(err, "send failed",           sizeof("send failed")          - 1) == 0 ||
                 strncmp(err, "connection closed",     sizeof("connection closed")    - 1) == 0 ||
                 strncmp(err, "not a TLS record",      sizeof("not a TLS record")     - 1) == 0 ||
                 strncmp(err, "fatal alert from client",
                                                       sizeof("fatal alert from client") - 1) == 0 ||
                 strncmp(err, "TLS 1.2 not offered",
                                                       sizeof("TLS 1.2 not offered")    - 1) == 0);
if(expected)
    RW_LOG_PRINT(LOGLEVEL_WARN,  "[SSL] TLS server handshake failed: %s\r\n", err);
else
    RW_LOG_PRINT(LOGLEVEL_ERROR, "[SSL] TLS server handshake failed: %s\r\n", err);
srv->detach_socket(); // don't close m_sockfd
delete srv;
return false;
}
RW_LOG_DEBUG("[SSL] TLS server handshake complete\r\n");
m_tls_server = srv;
return true;
}
RW_LOG_DEBUG(0, "[SSL] SSL_Associate called but TLS not initialized\r\n");
return false;
}

inline size_t socketSSL :: v_write(const char *buf, size_t buflen)
{
if(m_tls_client)
return (size_t)m_tls_client->send((char*)buf, (int)buflen);
if(m_tls_server)
return (size_t)m_tls_server->send(buf, (int)buflen);
return ::send(m_sockfd, buf, buflen, MSG_NOSIGNAL);
}

inline size_t socketSSL :: v_read(char *buf, size_t buflen)
{
// Drain peek buffer first
if(m_peeklen > 0){
size_t n = (buflen < (size_t)m_peeklen) ? buflen : (size_t)m_peeklen;
memcpy(buf, m_peekbuf, n);
m_peeklen -= (int)n;
if(m_peeklen > 0)
memmove(m_peekbuf, m_peekbuf + n, (size_t)m_peeklen);
return n;
}
if(m_tls_client)
return (size_t)m_tls_client->recv(buf, (int)buflen);
if(m_tls_server)
return (size_t)m_tls_server->recv(buf, (int)buflen);
return ::recv(m_sockfd, buf, buflen, MSG_NOSIGNAL);
}

inline size_t socketSSL :: v_peek(char *buf, size_t buflen)
{
if(m_peeklen == 0 && (m_tls_client || m_tls_server)){
// Fill peek buffer from TLS layer
int n = 0;
if(m_tls_client)
n = m_tls_client->recv(m_peekbuf, (int)sizeof(m_peekbuf));
else if(m_tls_server)
n = m_tls_server->recv(m_peekbuf, (int)sizeof(m_peekbuf));
if(n > 0) m_peeklen = n;
}
if(m_peeklen > 0){
size_t n = (buflen < (size_t)m_peeklen) ? buflen : (size_t)m_peeklen;
memcpy(buf, m_peekbuf, n);
return n;
}
return ::recv(m_sockfd, buf, buflen, MSG_NOSIGNAL | MSG_PEEK);
}

void socketSSL :: freeSSL()
{
if(m_tls_client){ delete m_tls_client; m_tls_client = NULL; }
if(m_tls_server){ delete m_tls_server; m_tls_server = NULL; }
m_ssltype = SSL_INIT_NONE;
m_peeklen = 0;
}

bool socketSSL :: initSSL(bool bInitServer, socketSSL *psock)
{
if(m_ssltype != SSL_INIT_NONE) return true;

if(psock != NULL)
setCacert(psock, false); // copy certificate from parent socket

if(bInitServer){
if(!m_has_cert){
// Generate runtime cert with machine hostname in SAN as fallback
char hostname[256] = "localhost";
DWORD hsz = (DWORD)sizeof(hostname);
GetComputerNameExA(ComputerNameDnsHostname, hostname, &hsz);
std::vector<unsigned char> rtcert;
unsigned char rtkey[32] = {};
if(gen_self_signed_cert(hostname, rtcert, rtkey)){
    m_cert_der = rtcert; memcpy(m_privkey, rtkey, 32);
} else {
    m_cert_der.assign(default_cert_der, default_cert_der + default_cert_der_len);
    memcpy(m_privkey, default_privkey, 32);
}
m_has_cert = true;
}
m_ssltype = SSL_INIT_SERV;
} else {
m_ssltype = SSL_INIT_CLNT;
}
return true;
}

#endif
