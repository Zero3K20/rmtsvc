/*
**	FILENAME			IPF.h
**
**	PURPOSE				open, save and convert image file formats
**						currently supports only BMP image format
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
// DIBSCANLINE_WIDTHBYTES performs DIB scanline DWORD alignment. Macro parameter "bits" is
// the product of biWidth and biBitCount in the DIB info structure. The macro result is the aligned
// bytes occupied by a scan line.
#define DIBSCANLINE_WIDTHBYTES(bits)    (((bits)+31)/32*4)
// DDBSCANLINE_WIDTHBYTES performs DDB scanline WORD alignment. Macro parameter "bits" is
// the product of bmWidth and bmBitCount in the DDB info structure. The macro result is the aligned
// bytes occupied by a scan line.
#define DDBSCANLINE_WIDTHBYTES(bits)    (((bits)+15)/16*2)

#define SWAP_INT(x,y) \
{ \
	int l=(x); \
	(x)=(y); \
	(y)=l; \
}
//small byte copy
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
	//[out] lpbi ---- return bitmap info,including possible palette info, user must ensure sufficient space.
	//					generally BMPINFOSIZE bytes is sufficient
	//[out] lppBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
	//				if lpBits==NULL, only return bitmap info
	//return: 0 on failure, non-zero (image data size) on success
	static IPFRESULT IPF_LoadBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//save BMP file -- 
	//[in] filename ---- destination bitmap filename
	//[in] lpbi ---- bitmap info
	//[in] lpBits ---- bitmap data pointer
	//return: 0 on failure, file size on success
//	static IPFRESULT IPF_SaveBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//capture window image --- 24-bit true color image
	//if hWnd==NULL, capture the entire screen
	//lpbih --- biWidth, biHeight specify the width/height of the window to capture; ==0 means capture window's full width or height
	//			biCompression should be set to BI_RGB (uncompressed)
	//			returns image info
	//lpBits --- save image data
	//			if==NULL, only return the space size needed for image data
	//quality --- reserved, not used
	//lprc --- specifies the capture area of the window; ==NULL means the entire window area
	//failure return 0, otherwise return image data size
	//ifCapCursor: whether to capture mouse cursor
	static IPFRESULT capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor);

	//get palette size of the specified DIB (in bytes)
//	static WORD PaletteSize(LPBITMAPINFOHEADER lpbih);

};

#endif

