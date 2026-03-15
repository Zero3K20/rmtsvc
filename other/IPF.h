/*
**	FILENAME			IPF.h
**
**	PURPOSE				图像文件的打开，存储,format转换
**						目前只支持BMP,JPEG两种图像format
**
**	CREATION DATE		2003-12-24
**	LAST MODIFICATION	2005-09-22 去掉了tiff和Pix图像format部分
**
**	AUTHOR				yyc
**
**	http://hi.baidu.com/yycblog/home
*/

#ifndef __IPF_20031224_H__
#define __IPF_20031224_H__

//----------------const define------------------
#define BMPINFOSIZE 2048
// DIBSCANLINE_WIDTHBYTES 执行DIB扫描行的DWORD对齐操作。宏参数“bits”是
// DIBinfo结构中成员biWidth和biBitCount的乘积。宏的结果是经过对齐后一个
// 扫描行所占的字节数。
#define DIBSCANLINE_WIDTHBYTES(bits)    (((bits)+31)/32*4)
// DDBSCANLINE_WIDTHBYTES 执行DDB扫描行的WORD对齐操作。宏参数“bits”是
// DDBinfo结构中成员bmWidth和bmBitCount的乘积。宏的结果是经过对齐后一个
// 扫描行所占的字节数。
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
//少量字节复制
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
	//打开位图文件 -- 
	//[in] filename ---- 位图filename
	//[out] lpbi ---- 返回位图info,包括可能的调色板info，用户必须保证足够大。
	//					一般来说defineBMPINFOSIZE字节足够了
	//[out] lppBits ---- 返回位图data指针(DWORD 对齐)，用户必须保证空间足够大
	//				如果lpBits==NULL则仅仅返回位图info
	//返回：如果failure返回0，否则返回非0(图像datasize)
	static IPFRESULT IPF_LoadBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//打开JPEG文件 -- 
	//[in] filename ---- JPEGfilename
	//[out] lpbi ---- 返回位图info
	//[out] lpBits ---- 返回位图data指针(DWORD 对齐)，用户必须保证空间足够大
	//				如果lpBits==NULL则仅仅返回位图info
	//返回：如果failure返回0，否则返回非0(图像datasize)
//	static IPFRESULT IPF_LoadJPEGFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//存储BMP文件 -- 
	//[in] filename ---- 目的位图filename
	//[in] lpbi ---- 位图info
	//[in] lpBits ---- 位图data指针
	//返回：如果failure返回0，否则返回file size
//	static IPFRESULT IPF_SaveBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//存储JPEG文件 -- 
	//目前只支持将8位灰度图或24位真彩色
	//[in] filename ---- 目的位图filename
	//[in] lpbi ---- 位图info头
	//[in] lpBits ---- 位图data指针
	//[in] quality --- jpeg压缩质量 (0~100)
	//返回：如果failure返回0，否则返回file size
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality);
	//lpBuf --- jpegdata流 
	//dwSize --- jpegdata流size
	//返回：如果failure返回0，否则返回file size
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBYTE lpBuf,DWORD dwSize);
	//将位图data压缩为JPEGdata流 -- 
	//目前只支持将8位灰度图或24位真彩色
	//[in] lpbih ---- 位图info头
	//[in] lpBits ---- 位图data指针
	//[out] dstBuf ---- 存储转换后的JPEGdata的空间,用户必须保证空间足够大
	//					一般来说分配和原位图一样大的空间即可
	//[in] quality --- jpeg压缩质量 (0~100)
	//返回：如果failure返回0，否则返回压缩成jpeg后data的size
	static IPFRESULT IPF_EncodeJPEG(LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,LPBYTE dstBuf,int quality);
	//将jpegdata解压缩为位图data流 -- 
	//[in] srcBuf ---- jpegdata指针
	//[in] dwSize ---- srcBuf指向的空间的size
	//[out] lpbi ---- 返回位图info
	//[out] lpBits ---- 返回位图data指针(DWORD 对齐)，用户必须保证空间足够大
	//				如果lpBits==NULL则仅仅返回位图info
	//返回：如果failure返回0，否则返回非0(图像datasize)
//	static IPFRESULT IPF_DecodeJPEG(LPBYTE srcBuf,DWORD dwSize,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//捕捉窗口图像 --- 24位真彩色图像
	//如果hWnd==NULL则捕捉整个屏幕
	//lpbih --- biWidth ，biHeightspecified捕捉窗口的宽高,==0则捕捉窗口的宽或高
	//			biCompressionspecified图像data的压缩方式，目前只支持BI_RGB(不压缩)，BI_JPEG(jpeg压缩)
	//			返回图像的info
	//lpBits --- 保存图像data或压缩后的图像data
	//			如果==NULL,则仅仅返回图像data需要的空间size
	//quality --- 如果specified了BI_JPEG压缩则此参数指明jpeg压缩质量
	//lprc --- specified捕捉窗口的区域==NULL则为整个窗口区域
	//failure返回0，否则返回图像datasize
	//ifCapCursor是否捕获鼠标光标
	static IPFRESULT capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor);

	//获取specifiedDIB的调色板尺寸（以字节为单位）
//	static WORD PaletteSize(LPBITMAPINFOHEADER lpbih);

};

#endif

