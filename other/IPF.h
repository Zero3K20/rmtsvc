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
// DIBSCANLINE_WIDTHBYTES 执行DIB扫描行的DWORDalignment操作。宏parameter“bits”yes
// the product of biWidth and biBitCount in the DIB info structure. The macro result is the aligned
// bytes occupied by a scan line.
#define DIBSCANLINE_WIDTHBYTES(bits)    (((bits)+31)/32*4)
// DDBSCANLINE_WIDTHBYTES 执行DDB扫描行的WORDalignment操作。宏parameter“bits”yes
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
	//open bitmap file -- 
	//[in] filename ---- bitmap filename
	//[out] lpbi ---- return bitmap info,packet括可能的调色板info，user必须保证足够大。
	//					一般来说defineBMPINFOSIZEbyte足够了
	//[out] lppBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
	//				iflpBits==NULL则仅仅return bitmap info
	//return: 0 on failure, non-zero (image data size) on success
	static IPFRESULT IPF_LoadBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//openJPEGfile -- 
	//[in] filename ---- JPEGfilename
	//[out] lpbi ---- return bitmap info
	//[out] lpBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
	//				iflpBits==NULL则仅仅return bitmap info
	//return: 0 on failure, non-zero (image data size) on success
//	static IPFRESULT IPF_LoadJPEGFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//save BMP file -- 
	//[in] filename ---- 目的bitmap filename
	//[in] lpbi ---- bitmap info
	//[in] lpBits ---- bitmap data pointer
	//return: 0 on failure, file size on success
//	static IPFRESULT IPF_SaveBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//save JPEG file -- 
	//currently only supports 8-bit grayscale or 24-bit true color
	//[in] filename ---- 目的bitmap filename
	//[in] lpbi ---- bitmap info头
	//[in] lpBits ---- bitmap data pointer
	//[in] quality --- JPEG compression quality (0~100)
	//return: 0 on failure, file size on success
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality);
	//lpBuf --- jpegdata流 
	//dwSize --- jpegdata流size
	//return: 0 on failure, file size on success
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBYTE lpBuf,DWORD dwSize);
	//将bitmapdata压缩为JPEGdata流 -- 
	//currently only supports 8-bit grayscale or 24-bit true color
	//[in] lpbih ---- bitmap info头
	//[in] lpBits ---- bitmap data pointer
	//[out] dstBuf ---- 存储convert后的JPEGdata的space,user必须保证space足够大
	//					一般来说分配and原bitmap一样大的space即可
	//[in] quality --- JPEG compression quality (0~100)
	//return：iffailurereturn0，otherwisereturn压缩成jpeg后data的size
	static IPFRESULT IPF_EncodeJPEG(LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,LPBYTE dstBuf,int quality);
	//将jpegdata解压缩为bitmapdata流 -- 
	//[in] srcBuf ---- jpegdatapointer
	//[in] dwSize ---- srcBufpointer to的space的size
	//[out] lpbi ---- return bitmap info
	//[out] lpBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
	//				iflpBits==NULL则仅仅return bitmap info
	//return: 0 on failure, non-zero (image data size) on success
//	static IPFRESULT IPF_DecodeJPEG(LPBYTE srcBuf,DWORD dwSize,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//捕捉windowimage --- 24bittrue彩色image
	//ifhWnd==NULL则捕捉整个screen
	//lpbih --- biWidth ，biHeightspecified捕捉window的宽高,==0则捕捉window的宽or高
	//			biCompressionspecifiedimagedata的压缩方式，目前只支持BI_RGB(not压缩)，BI_JPEG(jpeg压缩)
	//			returnimage的info
	//lpBits --- saveimagedataor压缩后的imagedata
	//			if==NULL,则仅仅returnimagedata需要的spacesize
	//quality --- ifspecified了BI_JPEG压缩则此parameterspecifiesjpegcompression quality
	//lprc --- specified捕捉window的区域==NULL则为整个window区域
	//failurereturn0，otherwisereturnimagedatasize
	//ifCapCursorwhethercapture mouse cursor
	static IPFRESULT capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor);

	//getspecifiedDIB的调色板尺寸（以byte为单bit）
//	static WORD PaletteSize(LPBITMAPINFOHEADER lpbih);

};

#endif

