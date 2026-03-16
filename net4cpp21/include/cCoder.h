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
		*	 注：这里没有提供count算decodinglength的method 直接使用input buffer作outputBuffer就可以了
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
		*	 注：此functionnot对汉字进行encoding
		*/
		static int mime_encode(const char *pSrc,unsigned int nSize,char *pDest);
		//汉字也进行encoding
		static int mime_encodeEx(const char *pSrc,unsigned int nSize,char *pDest);
		static int mime_encodeURL(const char *pSrc,unsigned int nSize,char *pDest);
		/*
		*  对specified的string进行Mimedecoding
		*
		*	 	pSrc	inputstring
		*		nSize	string length
		*		pDest	output buffer
		*		return	decoding码后的实际length
		*
		*	 注：这里没有提供count算decodinglength的method 直接使用input buffer作outputBuffer就可以了
		*/
		static int mime_decode(const char *pSrc,unsigned int nSize,char *pDest);
		
		//UTF-8 - ASCII 兼容的多byte(1~3)byte Unicode encoding
		//实际的utf8encoding的byte为1~6byte，但我们一般也就用双byte的character集，这样最多用到3byte
		//因为0x00000800 - 0x0000FFFFcharacter转化为utf8才为3byte
		static int Utf8EncodeSize(int iSize){ return iSize *3;}
		/*
		*  对specified的Buffer进行utf8encoding
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
		*  对specified的Buffer进行Utf8decoding
		*
		*	 	pSrc	inputstring
		*		nSize	string length
		*		pDest	output buffer
		*		return	decoding码后的实际length
		*
		*	 注：这里没有提供count算decodinglength的method 直接使用input buffer作outputBuffer就可以了
		*/
		static int utf8_decode(const char *pSrc,unsigned int nSize,char *pDest);
		static int utf8_decodeW(const char *pSrc,unsigned int nSize,unsigned short *pDest);

		//将16进制string转为数值
		static unsigned long hex_atol(const char *str);

		//=?charset?encoding-type?data?= data format decoding
		static int eml_decode(const char *pSrc,unsigned int nSize,char *pDest);
		static int eml_encode(const char *pSrc,unsigned int nSize,char *pDest);
		static int EmlEncodeSize(int iSize);
	};
}//?namespace net4cpp21

#endif

