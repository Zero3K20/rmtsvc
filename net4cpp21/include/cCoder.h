/*******************************************************************
   *	cCoder.h
   *    DESCRIPTION:character encoding/decoding utilities
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.0
   *******************************************************************/
   
#ifndef __YY_CCODER_H__
#define __YY_CCODER_H__

namespace net4cpp21
{
	class cCoder
	{
		static unsigned char DecToHex(unsigned char B);		//character conversion for Quoted encoding
		static unsigned char HexToDec(unsigned char C);		//character conversion for Quoted decoding
		
		static const char BASE64_ENCODE_TABLE[64];		//Base64 encoding table
		static const unsigned int BASE64_DECODE_TABLE[256];	//Base64 decoding table
		static const unsigned char QUOTED_ENCODE_TABLE[256];	//Quoted encoding table
		
	public:
		static unsigned int m_LineWidth;			//specifies the line length after encoding; default is 76
		//get Base64 encoded length from actual file length; -1 means no line length limit; 0 means line length is m_LineWidth
		static int Base64EncodeSize(int iSize,unsigned int nLineWidth=0);  
		static int Base64DecodeSize(int iSize);		//get Base64 decode length from the already-encoded file length
		static int UUEncodeSize(int iSize);			//get UUCode encoded length from actual file length
		static int UUDecodeSize(int iSize);			//get UUCode decode length from already-encoded file length
		static int QuotedEncodeSize(int iSize);		//get Quoted-Printable encoded length from actual file length
		
		
		/*
		*  perform Base64 encoding on a buffer
		*
		*	 	pSrc	input buffer
		*		nSize	Bufferlength
		*		pDest	output buffer
		*
		*	 note: output buffer length can be obtained using the Base64EncodeSize(int) method
		*/
		static int base64_encode(char *pSrc, unsigned int nSize, char *pDest,
			unsigned int nLineWidth=0); //add CRLF every specified LineWidth characters
		//==-1 means no line length limit after encoding; ==0 means line length is m_LineWidth
		/*
		*  perform Base64 decoding on a buffer
		*	
		*	 	pSrc	input buffer
		*		nSize	Bufferlength
		*		pDest	output buffer
		*		return	actual length after decoding
		*
		*	 note: output buffer length can be obtained using the Base64DecodeSize(int) method
		*/
		static int  base64_decode(char *pSrc, unsigned int nSize, char *pDest);
		/*
		*  perform UUCODE encoding on a buffer
		*
		*	 	pSrc	input buffer
		*		nSize	Bufferlength
		*		pDest	output buffer
		*
		*	 note: output buffer length can be obtained using the UUEncodeSize(int) method
		*/
		static int UU_encode(char *pSrc, unsigned int nSize, char *pDest);
		
		/*
		*  perform UUCODE decoding on a buffer
		*
		*	 	pSrc	input buffer
		*		nSize	Bufferlength
		*		pDest	output buffer
		*
		*	 note: output buffer length can be obtained using the UUDecodeSize(int) method
		*/
		static int UU_decode(char *pSrc, unsigned int nSize, char *pDest);
		/*
		*  perform Quoted-Printable encoding on a buffer
		*
		*	 	pSrc	input buffer
		*		nSize	Bufferlength
		*		pDest	output buffer
		*		return	actual length after encoding
		*
		*	 note: output buffer length can be obtained using the QuotedDecodeSize(int) method
		*/
		static int quoted_encode(char *pSrc, unsigned int nSize, char *pDest);
		
		/*
		*  perform Quoted-Printable decoding on a buffer
		*
		*	 	pSrc	input buffer
		*		nSize	Bufferlength
		*		pDest	output buffer
		*		return	actual length after decoding
		*
		*	 note: no method is provided to calculate the decode length here; use the input buffer directly as the output buffer
		*/
		static int quoted_decode(char *pSrc, unsigned int nSize, char *pDest);
		
		//URL encode/decode: convert &amp; &lt; &gt; to & < >
		static int url_decode(const char *pSrc,int nSize,char *pDest);
		static int url_encode(const char *pSrc,int nSize,char *pDest);
		static int MimeEncodeSize(int iSize){ return iSize *3;}
		/*
		*  perform MIME encoding on the specified string
		*
		*	 	pSrc	inputstring
		*		nSize	string length
		*		pDest	output buffer
		*		return	actual length after encoding
		*
		*	 note: this function does not encode Chinese characters
		*/
		static int mime_encode(const char *pSrc,unsigned int nSize,char *pDest);
		//also encodes Chinese characters
		static int mime_encodeEx(const char *pSrc,unsigned int nSize,char *pDest);
		static int mime_encodeURL(const char *pSrc,unsigned int nSize,char *pDest);
		/*
		*  perform MIME decoding on the specified string
		*
		*	 	pSrc	inputstring
		*		nSize	string length
		*		pDest	output buffer
		*		return	actual length after decoding
		*
		*	 note: no method is provided to calculate the decode length here; use the input buffer directly as the output buffer
		*/
		static int mime_decode(const char *pSrc,unsigned int nSize,char *pDest);
		
		//UTF-8 - ASCII-compatible multi-byte (1~3 bytes) Unicode encoding
		//the actual UTF-8 encoding uses 1~6 bytes, but we generally use double-byte character sets, so at most 3 bytes are used
		//because characters in range 0x00000800-0x0000FFFF are converted to 3 bytes in UTF-8
		static int Utf8EncodeSize(int iSize){ return iSize *3;}
		/*
		*  perform UTF-8 encoding on the specified buffer
		*
		*	 	pSrc	input buffer
		*		nSize	Bufferlength
		*		pDest	output buffer
		*		return	actual length after encoding
		*
		*/
		static int utf8_encode(const char *pSrc,unsigned int nSize,char *pDest);
		static int utf8_encodeW(const unsigned short *pSrc,unsigned int nSize,char *pDest);
		/*
		*  perform UTF-8 decoding on the specified buffer
		*
		*	 	pSrc	inputstring
		*		nSize	string length
		*		pDest	output buffer
		*		return	actual length after decoding
		*
		*	 note: no method is provided to calculate the decode length here; use the input buffer directly as the output buffer
		*/
		static int utf8_decode(const char *pSrc,unsigned int nSize,char *pDest);
		static int utf8_decodeW(const char *pSrc,unsigned int nSize,unsigned short *pDest);

		//convert a hexadecimal string to a numeric value
		static unsigned long hex_atol(const char *str);

		//=?charset?encoding-type?data?= data format decoding
		static int eml_decode(const char *pSrc,unsigned int nSize,char *pDest);
		static int eml_encode(const char *pSrc,unsigned int nSize,char *pDest);
		static int EmlEncodeSize(int iSize);
	};
}//?namespace net4cpp21

#endif

