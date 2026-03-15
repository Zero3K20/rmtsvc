/*
**	FILENAME			IPF.h
**
**	PURPOSE				图像文件的打开，存储,format转换
**						目前只支持BMP,JPEG两种图像format
**
**	CREATION DATE		2003-12-24
**	LAST MODIFICATION	2003-12-24
**
**	AUTHOR				yyc
**
**	http://hi.baidu.com/yycblog/home
*/

#include <windows.h>
#include <stdio.h>
#include "IPF.h"

#ifdef __cplusplus
	extern "C" {
#endif // __cplusplus

#include "../libs/jpeg/jpeglib.h"

#ifdef __cplusplus
	}
#endif // __cplusplus

#pragma comment( lib, "libs/bin/jpeg-r-dll" )
#pragma comment( lib, "legacy_stdio_definitions.lib" )


//打开位图文件 -- 
//[in] filename ---- 位图filename
//[out] lpbi ---- 返回位图info
//[out] lppBits ---- 返回位图data指针(DWORD 对齐),用户必须保证空间足够大
//				如果lppBits==NULL则仅仅返回位图info
//返回：如果failure返回0，否则返回非0(图像datasize)
IPFRESULT cImageF :: IPF_LoadBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits)
{
	if(filename==NULL || lpbi==NULL) return 0;
	FILE * file=NULL;
	if((file=fopen(filename,"rb"))==NULL) return 0;
	DWORD dataLen=0;
	BITMAPFILEHEADER bmfHeader;
	fseek(file,0,SEEK_SET);
	if( fread((void *)&bmfHeader,1,sizeof(BITMAPFILEHEADER),file)==sizeof(BITMAPFILEHEADER)
		&& bmfHeader.bfType ==0x4D42 )//是位图文件
	{
		fseek(file,sizeof(BITMAPFILEHEADER),SEEK_SET);
		fread((void *)lpbi,1,bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER),file);
		fseek(file,0,SEEK_END);
		dataLen=ftell(file)-bmfHeader.bfOffBits;
		if(lpBits)
		{ 
			// 读取位data
			fseek(file,bmfHeader.bfOffBits,SEEK_SET);
			fread((void *)lpBits,1,dataLen,file);
		}//?if(lppBits)
	}//?if( fread((void *)&bmfHeader,1
	fclose(file);
	return dataLen;
}

/*
//打开JPEG文件 -- 
//[in] filename ---- JPEGfilename
//[out] lpbi ---- 返回位图info
//[out] lpBits ---- 返回位图data指针(DWORD 对齐)，用户必须保证空间足够大
//				如果lpBits==NULL则仅仅返回位图info
//返回：如果failure返回0，否则返回非0(图像datasize)
IPFRESULT cImageF :: IPF_LoadJPEGFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits)
{
	if(filename==NULL || lpbi==NULL) return 0;
	FILE * file=NULL;
	if((file=fopen(filename,"rb"))==NULL) return 0;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	// Now we can initialize the JPEG compression object. 
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, TRUE);//得到图像的头info
	
//	if(cinfo.out_color_components==3)  
//		cinfo.out_color_space=JCS_RGB;
//	else
//		cinfo.out_color_space=JCS_GRAYSCALE;
	jpeg_start_decompress(&cinfo);

	lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbi->bmiHeader.biPlanes = 1;
	lpbi->bmiHeader.biXPelsPerMeter=0;
	lpbi->bmiHeader.biYPelsPerMeter=0;
	lpbi->bmiHeader.biClrUsed = 0;
	lpbi->bmiHeader.biClrImportant = 0;
	lpbi->bmiHeader.biCompression =BI_RGB;
	lpbi->bmiHeader.biBitCount =cinfo.out_color_components*8;
	lpbi->bmiHeader.biHeight =cinfo.output_height;
	lpbi->bmiHeader.biWidth =cinfo.output_width;
	long lEffWidth=DIBSCANLINE_WIDTHBYTES(lpbi->bmiHeader.biWidth *lpbi->bmiHeader.biBitCount );
	lpbi->bmiHeader.biSizeImage =lEffWidth * lpbi->bmiHeader.biHeight ;
	
	if( lpbi->bmiHeader.biBitCount==8 ){
		lpbi->bmiHeader.biClrUsed=256;//(1<<8);
		RGBQUAD	*lpRGB=(RGBQUAD *)((LPSTR)lpbi+lpbi->bmiHeader.biSize);
		int ratio=lpbi->bmiHeader.biClrUsed/256;
		for(int i=0; i <(int)lpbi->bmiHeader.biClrUsed;i++) {
			lpRGB[i].rgbBlue=i/ratio;
			lpRGB[i].rgbGreen=i/ratio;
			lpRGB[i].rgbRed=i/ratio;
			lpRGB[i].rgbReserved=0;
		}
	}//?if( (lpbi->bmiHeader.biBitCount==8...

	if(lpBits)
	{ 
		JSAMPROW ptr=lpBits+lEffWidth*(lpbi->bmiHeader.biHeight-1);
		//注意jpegdata总是从上到下读，而windows位图的data存储是从最后一行还是存即
		//从下到上
		//如果颜色不正确则可能RGB的颜色反了，可在"jpeg/jmorecfg.h"文件调整
		// Process data 
		 while (cinfo.output_scanline < cinfo.output_height) 
		 {
			 jpeg_read_scanlines(&cinfo,&ptr,1);
			 ptr-=lEffWidth;
		  }//?while(
		 jpeg_finish_decompress(&cinfo);
	}//?if(lppBits)

	jpeg_destroy_decompress(&cinfo);
	fclose(file);
	return lpbi->bmiHeader.biSizeImage;
}
*/
/*
//存储BMP文件 -- 
//[in] filename ---- 目的位图filename
//[in] lpbi ---- 位图info
//[in] lpBits ---- 位图data指针
//返回：如果failure返回0，否则返回file size
IPFRESULT cImageF :: IPF_SaveBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits)
{
	if (lpbi==NULL || lpBits==NULL ) return 0;
	if (filename==NULL || filename[0]==0) return 0;
	
	FILE *fp=::fopen(filename, "w+b");// 用create方式打开文件(二进制)
	if(!fp) return 0;

	::fseek(fp, 0, SEEK_SET);
	//写入文件头info
	BITMAPFILEHEADER	bmf;	
	bmf.bfType = 0x4D42;		//('BM')
	// 文件头尺寸＋info头尺寸＋颜色表尺寸＋位data尺寸
	DWORD dataSize=DIBSCANLINE_WIDTHBYTES(lpbi->bmiHeader.biWidth *lpbi->bmiHeader.biBitCount) 
		*lpbi->bmiHeader.biHeight;
	DWORD palSize=PaletteSize((LPBITMAPINFOHEADER)lpbi);
    bmf.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+palSize+dataSize;
    bmf.bfReserved1 = 0; 
    bmf.bfReserved2 = 0;
	// 文件头尺寸＋info头尺寸＋颜色表尺寸
    bmf.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +palSize;
	
	// 写入文件头info
	::fwrite((const void *)&bmf, sizeof(BITMAPFILEHEADER), 1, fp);

	// 写入info块内容和data
	LPBYTE lp = lpBits;
	
	::fwrite((const void *)lpbi, sizeof(BITMAPINFOHEADER) +palSize, 1, fp);
	DWORD dwB, dwC;
	// 以分段方式写入位data，每个段length为32KB。
	dwB = dataSize/32768;			// 段数（32768）
	dwC = dataSize - (dwB*32768);	// 余数
	for (;dwB!=0;dwB--)
	{
		::fwrite((const void *)lp, 32768, 1, fp);
		lp = (LPBYTE)((DWORD)lp+32768UL);
	}
	// 写入剩余的位data
	::fwrite((const void *)lp, dwC, 1, fp);

	::fclose(fp);
	return bmf.bfSize;
}
*/

//存储JPEG文件 -- 
//目前只支持将8位灰度图或24位真彩色
//[in] filename ---- 目的位图filename
//[in] lpbi ---- 位图info
//[in] lpBits ---- 位图data指针
//[in] quality --- jpeg压缩质量 (0~100)
//返回：如果failure返回0，否则返回file size
IPFRESULT cImageF :: IPF_SaveJPEGFile(const char *filename,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality)
{
	if (lpbih==NULL || lpBits==NULL ) return 0;
	if (filename==NULL || filename[0]==0) return 0;
	//目前只支持将8位灰度图或24位真彩色
	if(lpbih->biBitCount!=8 && lpbih->biBitCount!=24) return 0;
	// not supported压缩位图
//	ASSERT(lpbih->biCompression == BI_RGB);
	//计算原图像的行宽
	if(lpbih->biSizeImage==0)
		lpbih->biSizeImage=lpbih->biHeight * 
			DIBSCANLINE_WIDTHBYTES(lpbih->biWidth *lpbih->biBitCount);
	long lEffWidth =lpbih->biSizeImage/lpbih->biHeight;

	FILE *fp=::fopen(filename, "w+b");// 用create方式打开文件(二进制)
	if(!fp) return 0;
	::fseek(fp, 0, SEEK_SET);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fp);
	cinfo.image_width =lpbih->biWidth; 	// image width and height, in pixels
	cinfo.image_height =lpbih->biHeight;
	if (lpbih->biBitCount==24) {
		cinfo.input_components = 3;		// # of color components per pixel 
		cinfo.in_color_space = JCS_RGB; 	// colorspace of input image 
	 } else {
		cinfo.input_components = 1;		// # of color components per pixel 
		cinfo.in_color_space = JCS_GRAYSCALE; 	// colorspace of input image 
	 }
	jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    cinfo.dct_method = JDCT_FASTEST;
    cinfo.optimize_coding = TRUE;
	jpeg_start_compress(&cinfo, TRUE);
	//注意jpegdata总是从上到下读，而windows位图的data存储是从最后一行还是存即
	//从下到上
	//如果颜色不正确则可能RGB的颜色反了，可在"jpeg/jmorecfg.h"文件调整
	BYTE * IterImage=lpBits+lEffWidth*(cinfo.image_height-1);
	while (cinfo.next_scanline < cinfo.image_height) 
	{	
		 jpeg_write_scanlines(&cinfo, &IterImage, 1);
		 IterImage -= lEffWidth;
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	fseek(fp,0,SEEK_END);
	lEffWidth=ftell(fp);
	fclose(fp);
	return lEffWidth;
}

//lpBuf --- jpegdata流 
//dwSize --- jpegdata流size
//返回：如果failure返回0，否则返回file size
IPFRESULT cImageF::IPF_SaveJPEGFile(const char *filename,LPBYTE lpBuf,DWORD dwSize)
{
	if (lpBuf==NULL || dwSize==0 ) return 0;
	if (filename==NULL || filename[0]==0) return 0;
	FILE *fp=::fopen(filename, "w+b");// 用create方式打开文件(二进制)
	if(!fp) return 0;
	::fseek(fp, 0, SEEK_SET);

	LPBYTE lp = lpBuf;
	DWORD dwB, dwC;
	// 以分段方式写入位data，每个段length为32KB。
	dwB = dwSize/32768;			// 段数（32768）
	dwC = dwSize - (dwB*32768);	// 余数
	for (;dwB!=0;dwB--)
	{
		::fwrite((const void *)lp, 32768, 1, fp);
		lp = (LPBYTE)((DWORD)lp+32768UL);
	}
	// 写入剩余的位data
	::fwrite((const void *)lp, dwC, 1, fp);
	::fclose(fp);
	return dwSize;
}

//将位图data压缩为JPEGdata流 -- 
//目前只支持将8位灰度图或24位真彩色
//[in] lpbih ---- 位图info头
//[in] lpBits ---- 位图data指针
//[out] dstBuf ---- 存储转换后的JPEGdata的空间,用户必须保证空间足够大
//					一般来说分配和原位图一样大的空间即可
//[in] quality --- jpeg压缩质量 (0~100)
//返回：如果failure返回0，否则返回压缩成jpeg后data的size
METHODDEF void init_destination (j_compress_ptr cinfo)
{
	return;
}
METHODDEF boolean empty_output_buffer (j_compress_ptr cinfo)
{
	return true;
}
METHODDEF void term_destination (j_compress_ptr cinfo)
{
	return;
}
IPFRESULT cImageF :: IPF_EncodeJPEG(LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,LPBYTE dstBuf,int quality)
{
	if (lpbih==NULL || lpBits==NULL || dstBuf==NULL ) return 0;
	//目前只支持将8位灰度图或24位真彩色
	if(lpbih->biBitCount!=8 && lpbih->biBitCount!=24) return 0;
	//计算原图像的行宽
	if(lpbih->biSizeImage==0)
		lpbih->biSizeImage=lpbih->biHeight * 
			DIBSCANLINE_WIDTHBYTES(lpbih->biWidth *lpbih->biBitCount);
	long lEffWidth =lpbih->biSizeImage/lpbih->biHeight;
	LPBYTE lpDstBits=dstBuf;
	if(dstBuf==lpBits)
	{//源address和目的address相同
		if( (lpDstBits=(LPBYTE)::malloc(lpbih->biSizeImage))==NULL)
			return 0;
	}
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	struct jpeg_destination_mgr dest;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	dest.next_output_byte = lpDstBits;
	dest.free_in_buffer = lpbih->biSizeImage;
	dest.init_destination = init_destination;//NULL;
	dest.empty_output_buffer = empty_output_buffer;//NULL;
	dest.term_destination = term_destination;//NULL;
	cinfo.dest=&dest;
	cinfo.image_width =lpbih->biWidth; 	// image width and height, in pixels
	cinfo.image_height =lpbih->biHeight;
	if (lpbih->biBitCount==24) {
		cinfo.input_components = 3;		/* # of color components per pixel */
		cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
	 } else {
		cinfo.input_components = 1;		/* # of color components per pixel */
		cinfo.in_color_space = JCS_GRAYSCALE; 	/* colorspace of input image */
	 }
	jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    cinfo.dct_method = JDCT_FASTEST;
    cinfo.optimize_coding = TRUE;
	
	jpeg_start_compress(&cinfo, TRUE);
	
	//注意jpegdata总是从上到下读，而windows位图的data存储是从最后一行还是存即
	//从下到上
	//如果颜色不正确则可能RGB的颜色反了，可在"jpeg/jmorecfg.h"文件调整
	BYTE * IterImage=lpBits+lEffWidth*(cinfo.image_height-1);
	while (cinfo.next_scanline < cinfo.image_height) 
	{	
		 jpeg_write_scanlines(&cinfo, &IterImage, 1);
		 IterImage -= lEffWidth;
	}
	
	jpeg_finish_compress(&cinfo);
	lEffWidth=lpbih->biSizeImage-dest.free_in_buffer;
	jpeg_destroy_compress(&cinfo);

	if(dstBuf==lpBits)
	{//源address和目的address相同
		memcpy((void *)lpBits,(const void *)lpDstBits,lEffWidth);
		::free(lpDstBits);
	}
	return lEffWidth;
}
/*
//将jpegdata解压缩为位图data流 -- 
//[in] srcBuf ---- jpegdata指针
//[in] dwSize ---- srcBuf指向的空间的size
//[out] lpbi ---- 返回位图info
//[out] lpBits ---- 返回位图data指针(DWORD 对齐)，用户必须保证空间足够大
//				如果lpBits==NULL则仅仅返回位图info
//返回：如果failure返回0，否则返回非0(图像datasize)
IPFRESULT cImageF :: IPF_DecodeJPEG(LPBYTE srcBuf,DWORD dwSize,LPBITMAPINFO lpbi,LPBYTE lpBits)
{
	if(srcBuf==NULL || dwSize<=0 || lpbi==NULL) return 0;
	
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	struct jpeg_source_mgr src;
	cinfo.err = jpeg_std_error(&jerr);
	// Now we can initialize the JPEG compression object. 
	jpeg_create_decompress(&cinfo);
	src.next_input_byte = srcBuf;
	src.bytes_in_buffer = dwSize;
	src.init_source = NULL;
	src.fill_input_buffer=NULL;
	src.skip_input_data=NULL;
	src.resync_to_restart=NULL;
	src.term_source=NULL;
	cinfo.src=&src;

	jpeg_read_header(&cinfo, TRUE);//得到图像的头info
	jpeg_start_decompress(&cinfo);
	
	lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbi->bmiHeader.biPlanes = 1;
	lpbi->bmiHeader.biXPelsPerMeter=0;
	lpbi->bmiHeader.biYPelsPerMeter=0;
	lpbi->bmiHeader.biClrUsed = 0;
	lpbi->bmiHeader.biClrImportant = 0;
	lpbi->bmiHeader.biCompression =BI_RGB;
	lpbi->bmiHeader.biBitCount =cinfo.out_color_components*8;
	lpbi->bmiHeader.biHeight =cinfo.output_height;
	lpbi->bmiHeader.biWidth =cinfo.output_width;
	long lEffWidth=DIBSCANLINE_WIDTHBYTES(lpbi->bmiHeader.biWidth *lpbi->bmiHeader.biBitCount );
	lpbi->bmiHeader.biSizeImage =lEffWidth * lpbi->bmiHeader.biHeight ;

	if( lpbi->bmiHeader.biBitCount==8 ){
		lpbi->bmiHeader.biClrUsed=256;//(1<<8);
		RGBQUAD	*lpRGB=(RGBQUAD *)((LPSTR)lpbi+lpbi->bmiHeader.biSize);
		int ratio=lpbi->bmiHeader.biClrUsed/256;
		for(int i=0; i <(int)lpbi->bmiHeader.biClrUsed;i++) {
			lpRGB[i].rgbBlue=i/ratio;
			lpRGB[i].rgbGreen=i/ratio;
			lpRGB[i].rgbRed=i/ratio;
			lpRGB[i].rgbReserved=0;
		}
	}//?if( (lpbi->bmiHeader.biBitCount==8...

	if(lpBits)
	{ 
		JSAMPROW ptr=lpBits+lEffWidth*(lpbi->bmiHeader.biHeight-1);
		//注意jpegdata总是从上到下读，而windows位图的data存储是从最后一行还是存即
		//从下到上
		//如果颜色不正确则可能RGB的颜色反了，可在"jpeg/jmorecfg.h"文件调整
		// Process data 
		 while (cinfo.output_scanline < cinfo.output_height) 
		 {
			 jpeg_read_scanlines(&cinfo,&ptr,1);
			 ptr-=lEffWidth;
		  }//?while(
		 jpeg_finish_decompress(&cinfo);
	}//?if(lppBits)

	jpeg_destroy_decompress(&cinfo);

	return lpbi->bmiHeader.biSizeImage;
}
*/
//------------------------------------------------------------------------------------------
//捕捉窗口图像 --- 24位真彩色图像
//如果hWnd==NULL则捕捉整个屏幕
//lpbih --- 
//			biCompressionspecified图像data的压缩方式，目前只支持BI_RGB(不压缩)，BI_JPEG(jpeg压缩)
//			返回图像的info
//lpBits --- 保存图像data或压缩后的图像data
//			如果==NULL,则仅仅返回图像data需要的空间size
//quality --- 如果specified了BI_JPEG压缩则此参数指明jpeg压缩质量
//failure返回0，否则返回图像datasize
//ifCapCursor是否捕获鼠标光标
IPFRESULT cImageF::capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor)
{
	if(lpbih==NULL) return 0;
	if(hWnd == NULL)
		if((hWnd = ::GetDesktopWindow())==NULL) return 0;
	
	RECT rect;
	::GetClientRect(hWnd, &rect);

	//要保存的原图像区域的起始点坐标和宽高
	long lX=0,tY=0;
	long lWidth=rect.right - rect.left;
	long lHeight=rect.bottom - rect.top ;

	lpbih->biSize = sizeof(BITMAPINFOHEADER);
	lpbih->biPlanes = 1;
	lpbih->biXPelsPerMeter=0;
	lpbih->biYPelsPerMeter=0;
	lpbih->biClrUsed = 0;
	lpbih->biClrImportant = 0;
	lpbih->biBitCount =24;
	lpbih->biHeight =lHeight;
	lpbih->biWidth =lWidth;
	long lEffWidth=DIBSCANLINE_WIDTHBYTES(lpbih->biWidth *lpbih->biBitCount );
	DWORD dwRet=lpbih->biSizeImage =lEffWidth * lpbih->biHeight ;
	if(lpBits==NULL) return dwRet;
	//是否压缩
	BOOL ifComp=(lpbih->biCompression==BI_JPEG);
	lpbih->biCompression =BI_RGB;
	
	HDC hWndDC = NULL;
	HDC hMemDC = NULL;
	HBITMAP hMemBmp = NULL;
	HBITMAP hOldBmp = NULL;
	hWndDC = ::GetDC(hWnd);
	hMemDC = ::CreateCompatibleDC(hWndDC);
	hMemBmp = ::CreateCompatibleBitmap(hWndDC, lWidth, lHeight);
	hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);
	::BitBlt(hMemDC, 0, 0, lWidth, lHeight, hWndDC, lX, tY, SRCCOPY);
	
	if(ifCapCursor) //捕获鼠标光标
	{
		POINT ptCursor;
		::GetCursorPos(&ptCursor);
		//先获得鼠标光标下的窗口句柄，得到该窗口的thread ID
		//Attatchcurrent thread到specified的窗口线程
		//获得该窗口current鼠标光标句柄
		//Deattach
		//!!!如果不这样做，直接调用GetCursor()则总是获得current thread的光标句柄
		//如果没有设置则获得的总是漏斗光标句柄
		HWND hw=::WindowFromPoint(ptCursor);
		if(hw==NULL) hw=::GetDesktopWindow();
		DWORD hdl=::GetWindowThreadProcessId(hw,NULL);
		::AttachThreadInput(::GetCurrentThreadId(),hdl,TRUE);
		HCURSOR hCursor=::GetCursor();
		::AttachThreadInput(::GetCurrentThreadId(),hdl,FALSE);
		//获取光标的图标data 
		ICONINFO IconInfo;
		if (::GetIconInfo(hCursor, &IconInfo))
		{
			ptCursor.x -= ((int) IconInfo.xHotspot);
			ptCursor.y -= ((int) IconInfo.yHotspot);
			
			//if (IconInfo.hbmMask != NULL)
			//	::DeleteObject(IconInfo.hbmMask);
			//if (IconInfo.hbmColor != NULL)
			//	::DeleteObject(IconInfo.hbmColor);
		}
		//在兼容设备description表上画出该光标
		::DrawIconEx(
		hMemDC, // handle to device context 
		ptCursor.x, ptCursor.y,
		hCursor, // handle to icon to draw 
		0,0, // width of the icon 
		0, // index of frame in animated cursor 
		NULL, // handle to background brush 
		DI_NORMAL | DI_COMPAT // icon-drawing flags 
		); 
	}//?if(ifCapCursor) //捕获鼠标光标

	if(!::GetDIBits(hWndDC,hMemBmp,0,lHeight,lpBits,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
		dwRet=0;
	else if(ifComp)
	{//是否进行jpeg压缩
		dwRet=cImageF::IPF_EncodeJPEG(lpbih,lpBits,lpBits,quality);
		lpbih->biCompression =BI_JPEG;
	}
	::SelectObject(hMemDC, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWnd, hWndDC);
	return dwRet;
}

//***********************************************************************************************
//**********************************************private function ********************************
/*
//获取specifiedDIB的调色板尺寸（以字节为单位）
WORD cImageF::PaletteSize(LPBITMAPINFOHEADER lpbih)
{
	WORD size, wBitCount;

	// 获得每个像素所占的位数
	wBitCount =lpbih->biBitCount;
				
	// 16位和32位位图在颜色表中占用三个DWORD数值来表示
	// 红、绿、蓝在位data中的掩码
	if ((wBitCount == 16)||(wBitCount == 32))
	{
		return sizeof(DWORD)*3;
	}
	else
	{
		// not supported压缩位图
		//ASSERT(lpbi->biCompression == BI_RGB);
		WORD wc;
		if (lpbih->biClrUsed) 
			wc=(WORD)(lpbih->biClrUsed);
		else
		{
			switch (lpbih->biBitCount)
			{
				case 1:
					wc=2;
					break;          // 单色位图，只有黑白两种颜色
				case 4:
					wc=16;		
					break;			// 标准VGA位图，有16种颜色
				case 8:
					wc=256;		
					break;			// SVGA位图，有256种颜色
				case	16:			// 64K色位图
				case	24:			// 16M色位图（真彩色）
				case	32:			// 16M+色位图（真彩色）
					wc=0;		
					break;			// 颜色表中没有颜色data返回0
				default:
					return 0; //error
			}
		}
		size = (WORD)(wc * sizeof(RGBQUAD));
	}
	return size;	
}
*/

