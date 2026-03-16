/*
**	FILENAME			IPF.h
**
**	PURPOSE				open, save and convert image file formats
**						currently supports only BMP and JPEG image formats
**
**	CREATION DATE		2003-12-24
**	LAST MODIFICATION	2005-09-22 removed tiff and Pix image format support
**
**	AUTHOR				yyc
**
**	http://hi.baidu.com/yycblog/home
*/

#ifndef __IPF_20031224_H__
#define __IPF_20031224_H__

//----------------const define------------------
#define BMPINFOSIZE 2048
// DIBSCANLINE_WIDTHBYTES 执行DIB扫描行的DWORD对齐操作。宏parameter“bits”yes
// the product of biWidth and biBitCount in the DIB info structure. The macro result is the aligned
// bytes occupied by a scan line.
#define DIBSCANLINE_WIDTHBYTES(bits)    (((bits)+31)/32*4)
// DDBSCANLINE_WIDTHBYTES 执行DDB扫描行的WORD对齐操作。宏parameter“bits”yes
// the product of bmWidth and bmBitCount in the DDB info structure. The macro result is the aligned
// bytes occupied by a scan line.
#define DDBSCANLINE_WIDTHBYTES(bits)    (((bits)+15)/16*2)

#ifndef BI_JPEG
	#define BI_JPEG        4L
#endif

#define SWAP_INT(x,y) \
{ \
	int l=(x); \
	(x)=(y); \
	(y)=l; \
}
//少量byte复制
//#define BITSCOPY(dst,src,c) \
//	memcpy((dst),(src),(c))

#define BITSCOPY(dst,src,c) \
	{ \
		for(int n=0;n<(c);n++) \
			*((dst)+n)=*((src)+n); \
	}
//----------------------------------------------

#define IPFRESULT DWORD

class cImageF
{
public:
	//openbit图file -- 
	//[in] filename ---- bit图filename
	//[out] lpbi ---- returnbit图info,packet括可能的调色板info，user必须保证足够大。
	//					一般来说defineBMPINFOSIZEbyte足够了
	//[out] lppBits ---- returnbit图datapointer(DWORD 对齐)，user必须保证null间足够大
	//				iflpBits==NULL则仅仅returnbit图info
	//return：iffailurereturn0，otherwisereturn非0(图像datasize)
	static IPFRESULT IPF_LoadBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//openJPEGfile -- 
	//[in] filename ---- JPEGfilename
	//[out] lpbi ---- returnbit图info
	//[out] lpBits ---- returnbit图datapointer(DWORD 对齐)，user必须保证null间足够大
	//				iflpBits==NULL则仅仅returnbit图info
	//return：iffailurereturn0，otherwisereturn非0(图像datasize)
//	static IPFRESULT IPF_LoadJPEGFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//存储BMPfile -- 
	//[in] filename ---- 目的bit图filename
	//[in] lpbi ---- bit图info
	//[in] lpBits ---- bit图datapointer
	//return：iffailurereturn0，otherwisereturnfile size
//	static IPFRESULT IPF_SaveBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//存储JPEGfile -- 
	//目前只支持将8bit灰度图or24bittrue彩色
	//[in] filename ---- 目的bit图filename
	//[in] lpbi ---- bit图info头
	//[in] lpBits ---- bit图datapointer
	//[in] quality --- jpeg压缩质量 (0~100)
	//return：iffailurereturn0，otherwisereturnfile size
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality);
	//lpBuf --- jpegdata流 
	//dwSize --- jpegdata流size
	//return：iffailurereturn0，otherwisereturnfile size
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBYTE lpBuf,DWORD dwSize);
	//将bit图data压缩为JPEGdata流 -- 
	//目前只支持将8bit灰度图or24bittrue彩色
	//[in] lpbih ---- bit图info头
	//[in] lpBits ---- bit图datapointer
	//[out] dstBuf ---- 存储convert后的JPEGdata的null间,user必须保证null间足够大
	//					一般来说分配and原bit图一样大的null间即可
	//[in] quality --- jpeg压缩质量 (0~100)
	//return：iffailurereturn0，otherwisereturn压缩成jpeg后data的size
	static IPFRESULT IPF_EncodeJPEG(LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,LPBYTE dstBuf,int quality);
	//将jpegdata解压缩为bit图data流 -- 
	//[in] srcBuf ---- jpegdatapointer
	//[in] dwSize ---- srcBufpointer to的null间的size
	//[out] lpbi ---- returnbit图info
	//[out] lpBits ---- returnbit图datapointer(DWORD 对齐)，user必须保证null间足够大
	//				iflpBits==NULL则仅仅returnbit图info
	//return：iffailurereturn0，otherwisereturn非0(图像datasize)
//	static IPFRESULT IPF_DecodeJPEG(LPBYTE srcBuf,DWORD dwSize,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//捕捉窗口图像 --- 24bittrue彩色图像
	//ifhWnd==NULL则捕捉整个屏幕
	//lpbih --- biWidth ，biHeightspecified捕捉窗口的宽高,==0则捕捉窗口的宽or高
	//			biCompressionspecified图像data的压缩方式，目前只支持BI_RGB(not压缩)，BI_JPEG(jpeg压缩)
	//			return图像的info
	//lpBits --- save图像dataor压缩后的图像data
	//			if==NULL,则仅仅return图像data需要的null间size
	//quality --- ifspecified了BI_JPEG压缩则此parameterspecifiesjpeg压缩质量
	//lprc --- specified捕捉窗口的区域==NULL则为整个窗口区域
	//failurereturn0，otherwisereturn图像datasize
	//ifCapCursoryesno捕获鼠标光标
	static IPFRESULT capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor);

	//getspecifiedDIB的调色板尺寸（以byte为单bit）
//	static WORD PaletteSize(LPBITMAPINFOHEADER lpbih);

};

#endif

