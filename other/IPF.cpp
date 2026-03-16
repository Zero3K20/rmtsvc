/*
**	FILENAME			IPF.h
**
**	PURPOSE				open, save and convert image file formats
**						currently supports only BMP and JPEG image formats
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


//open bitmap file -- 
//[in] filename ---- bitmap filename
//[out] lpbi ---- return bitmap info
//[out] lppBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
//				iflppBits==NULL则仅仅return bitmap info
//return: 0 on failure, non-zero (image data size) on success
IPFRESULT cImageF :: IPF_LoadBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits)
{
	if(filename==NULL || lpbi==NULL) return 0;
	FILE * file=NULL;
	if((file=fopen(filename,"rb"))==NULL) return 0;
	DWORD dataLen=0;
	BITMAPFILEHEADER bmfHeader;
	fseek(file,0,SEEK_SET);
	if( fread((void *)&bmfHeader,1,sizeof(BITMAPFILEHEADER),file)==sizeof(BITMAPFILEHEADER)
		&& bmfHeader.bfType ==0x4D42 )//is a bitmap file
	{
		fseek(file,sizeof(BITMAPFILEHEADER),SEEK_SET);
		fread((void *)lpbi,1,bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER),file);
		fseek(file,0,SEEK_END);
		dataLen=ftell(file)-bmfHeader.bfOffBits;
		if(lpBits)
		{ 
			// readbitdata
			fseek(file,bmfHeader.bfOffBits,SEEK_SET);
			fread((void *)lpBits,1,dataLen,file);
		}//?if(lppBits)
	}//?if( fread((void *)&bmfHeader,1
	fclose(file);
	return dataLen;
}

/*
//openJPEGfile -- 
//[in] filename ---- JPEGfilename
//[out] lpbi ---- return bitmap info
//[out] lpBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
//				iflpBits==NULL则仅仅return bitmap info
//return: 0 on failure, non-zero (image data size) on success
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
	jpeg_read_header(&cinfo, TRUE);//get the image header info
	
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
		//note: JPEG data is read top-to-bottom, but Windows bitmap data is stored starting from the last line, i.e.
		//bottom-to-top
		//if colors are incorrect, RGB colors may be reversed; adjust in "jpeg/jmorecfg.h" file
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
//save BMP file -- 
//[in] filename ---- 目的bitmap filename
//[in] lpbi ---- bitmap info
//[in] lpBits ---- bitmap data pointer
//return: 0 on failure, file size on success
IPFRESULT cImageF :: IPF_SaveBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits)
{
	if (lpbi==NULL || lpBits==NULL ) return 0;
	if (filename==NULL || filename[0]==0) return 0;
	
	FILE *fp=::fopen(filename, "w+b");// open file in create mode (binary)
	if(!fp) return 0;

	::fseek(fp, 0, SEEK_SET);
	//write file header info
	BITMAPFILEHEADER	bmf;	
	bmf.bfType = 0x4D42;		//('BM')
	// file header size + info header size + color table size + bit data size
	DWORD dataSize=DIBSCANLINE_WIDTHBYTES(lpbi->bmiHeader.biWidth *lpbi->bmiHeader.biBitCount) 
		*lpbi->bmiHeader.biHeight;
	DWORD palSize=PaletteSize((LPBITMAPINFOHEADER)lpbi);
    bmf.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+palSize+dataSize;
    bmf.bfReserved1 = 0; 
    bmf.bfReserved2 = 0;
	// file header size + info header size + color table size
    bmf.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +palSize;
	
	// write file header info
	::fwrite((const void *)&bmf, sizeof(BITMAPFILEHEADER), 1, fp);

	// write info block content and data
	LPBYTE lp = lpBits;
	
	::fwrite((const void *)lpbi, sizeof(BITMAPINFOHEADER) +palSize, 1, fp);
	DWORD dwB, dwC;
	// write bitmap data in fragments, each segment 32KB.
	dwB = dataSize/32768;			// number of segments (32768)
	dwC = dataSize - (dwB*32768);	// remainder
	for (;dwB!=0;dwB--)
	{
		::fwrite((const void *)lp, 32768, 1, fp);
		lp = (LPBYTE)((DWORD)lp+32768UL);
	}
	// write remaining bitmap data
	::fwrite((const void *)lp, dwC, 1, fp);

	::fclose(fp);
	return bmf.bfSize;
}
*/

//save JPEG file -- 
//currently only supports 8-bit grayscale or 24-bit true color
//[in] filename ---- 目的bitmap filename
//[in] lpbi ---- bitmap info
//[in] lpBits ---- bitmap data pointer
//[in] quality --- JPEG compression quality (0~100)
//return: 0 on failure, file size on success
IPFRESULT cImageF :: IPF_SaveJPEGFile(const char *filename,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality)
{
	if (lpbih==NULL || lpBits==NULL ) return 0;
	if (filename==NULL || filename[0]==0) return 0;
	//currently only supports 8-bit grayscale or 24-bit true color
	if(lpbih->biBitCount!=8 && lpbih->biBitCount!=24) return 0;
	// bitmap compression not supported
//	ASSERT(lpbih->biCompression == BI_RGB);
	//calculate the row width of the original image
	if(lpbih->biSizeImage==0)
		lpbih->biSizeImage=lpbih->biHeight * 
			DIBSCANLINE_WIDTHBYTES(lpbih->biWidth *lpbih->biBitCount);
	long lEffWidth =lpbih->biSizeImage/lpbih->biHeight;

	FILE *fp=::fopen(filename, "w+b");// open file in create mode (binary)
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
	//note: JPEG data is read top-to-bottom, but Windows bitmap data is stored starting from the last line, i.e.
	//bottom-to-top
	//if colors are incorrect, RGB colors may be reversed; adjust in "jpeg/jmorecfg.h" file
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
//return: 0 on failure, file size on success
IPFRESULT cImageF::IPF_SaveJPEGFile(const char *filename,LPBYTE lpBuf,DWORD dwSize)
{
	if (lpBuf==NULL || dwSize==0 ) return 0;
	if (filename==NULL || filename[0]==0) return 0;
	FILE *fp=::fopen(filename, "w+b");// open file in create mode (binary)
	if(!fp) return 0;
	::fseek(fp, 0, SEEK_SET);

	LPBYTE lp = lpBuf;
	DWORD dwB, dwC;
	// write bitmap data in fragments, each segment 32KB.
	dwB = dwSize/32768;			// number of segments (32768)
	dwC = dwSize - (dwB*32768);	// remainder
	for (;dwB!=0;dwB--)
	{
		::fwrite((const void *)lp, 32768, 1, fp);
		lp = (LPBYTE)((DWORD)lp+32768UL);
	}
	// write remaining bitmap data
	::fwrite((const void *)lp, dwC, 1, fp);
	::fclose(fp);
	return dwSize;
}

//将bitmapdata压缩为JPEGdata流 -- 
//currently only supports 8-bit grayscale or 24-bit true color
//[in] lpbih ---- bitmap info头
//[in] lpBits ---- bitmap data pointer
//[out] dstBuf ---- 存储convert后的JPEGdata的space,user必须保证space足够大
//					一般来说分配and原bitmap一样大的space即可
//[in] quality --- JPEG compression quality (0~100)
//return：iffailurereturn0，otherwisereturn压缩成jpeg后data的size
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
	//currently only supports 8-bit grayscale or 24-bit true color
	if(lpbih->biBitCount!=8 && lpbih->biBitCount!=24) return 0;
	//calculate the row width of the original image
	if(lpbih->biSizeImage==0)
		lpbih->biSizeImage=lpbih->biHeight * 
			DIBSCANLINE_WIDTHBYTES(lpbih->biWidth *lpbih->biBitCount);
	long lEffWidth =lpbih->biSizeImage/lpbih->biHeight;
	LPBYTE lpDstBits=dstBuf;
	if(dstBuf==lpBits)
	{//源addressand目的address相同
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
	
	//note: JPEG data is read top-to-bottom, but Windows bitmap data is stored starting from the last line, i.e.
	//bottom-to-top
	//if colors are incorrect, RGB colors may be reversed; adjust in "jpeg/jmorecfg.h" file
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
	{//源addressand目的address相同
		memcpy((void *)lpBits,(const void *)lpDstBits,lEffWidth);
		::free(lpDstBits);
	}
	return lEffWidth;
}
/*
//将jpegdata解压缩为bitmapdata流 -- 
//[in] srcBuf ---- jpegdatapointer
//[in] dwSize ---- srcBufpointer to的space的size
//[out] lpbi ---- return bitmap info
//[out] lpBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
//				iflpBits==NULL则仅仅return bitmap info
//return: 0 on failure, non-zero (image data size) on success
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

	jpeg_read_header(&cinfo, TRUE);//get the image header info
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
		//note: JPEG data is read top-to-bottom, but Windows bitmap data is stored starting from the last line, i.e.
		//bottom-to-top
		//if colors are incorrect, RGB colors may be reversed; adjust in "jpeg/jmorecfg.h" file
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
//捕捉windowimage --- 24bittrue彩色image
//ifhWnd==NULL则捕捉整个screen
//lpbih --- 
//			biCompressionspecifiedimagedata的压缩方式，目前只支持BI_RGB(not压缩)，BI_JPEG(jpeg压缩)
//			returnimage的info
//lpBits --- saveimagedataor压缩后的imagedata
//			if==NULL,则仅仅returnimagedata需要的spacesize
//quality --- ifspecified了BI_JPEG压缩则此parameterspecifiesjpegcompression quality
//failurereturn0，otherwisereturnimagedatasize
//ifCapCursorwhethercapture mouse cursor
IPFRESULT cImageF::capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor)
{
	if(lpbih==NULL) return 0;
	if(hWnd == NULL)
		if((hWnd = ::GetDesktopWindow())==NULL) return 0;
	
	RECT rect;
	::GetClientRect(hWnd, &rect);

	//要save的原image区域的起始点坐标and宽高
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
	//whether压缩
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
	
	if(ifCapCursor) //capture mouse cursor
	{
		POINT ptCursor;
		::GetCursorPos(&ptCursor);
		//first get the window handle under the mouse cursor, then get that window's thread ID
		//Attatchcurrent thread到specified的windowthread
		//get该windowcurrentmousecursorhandle
		//Deattach
		//!!!ifnot这样做，直接调用GetCursor()则totalyesgetcurrent thread的cursorhandle
		//if没有set则get的totalyes漏斗cursorhandle
		HWND hw=::WindowFromPoint(ptCursor);
		if(hw==NULL) hw=::GetDesktopWindow();
		DWORD hdl=::GetWindowThreadProcessId(hw,NULL);
		::AttachThreadInput(::GetCurrentThreadId(),hdl,TRUE);
		HCURSOR hCursor=::GetCursor();
		::AttachThreadInput(::GetCurrentThreadId(),hdl,FALSE);
		//getcursor的图标data 
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
		//at兼容设备description表上画出该cursor
		::DrawIconEx(
		hMemDC, // handle to device context 
		ptCursor.x, ptCursor.y,
		hCursor, // handle to icon to draw 
		0,0, // width of the icon 
		0, // index of frame in animated cursor 
		NULL, // handle to background brush 
		DI_NORMAL | DI_COMPAT // icon-drawing flags 
		); 
	}//?if(ifCapCursor) //capture mouse cursor

	if(!::GetDIBits(hWndDC,hMemBmp,0,lHeight,lpBits,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
		dwRet=0;
	else if(ifComp)
	{//whetherperform JPEG compression
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
//getspecifiedDIB的调色板尺寸（以byte为单bit）
WORD cImageF::PaletteSize(LPBITMAPINFOHEADER lpbih)
{
	WORD size, wBitCount;

	// geteach像素所占的bit数
	wBitCount =lpbih->biBitCount;
				
	// 16bitand32bitbitmapatcolor表中占用三个DWORD数值来表示
	// 红、绿、蓝atbitdata中的掩码
	if ((wBitCount == 16)||(wBitCount == 32))
	{
		return sizeof(DWORD)*3;
	}
	else
	{
		// bitmap compression not supported
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
					break;          // 单色bitmap，只有黑白两种color
				case 4:
					wc=16;		
					break;			// 标准VGAbitmap，有16种color
				case 8:
					wc=256;		
					break;			// SVGAbitmap，有256种color
				case	16:			// 64K色bitmap
				case	24:			// 16M色bitmap（true彩色）
				case	32:			// 16M+色bitmap（true彩色）
					wc=0;		
					break;			// color表中没有colordatareturn0
				default:
					return 0; //error
			}
		}
		size = (WORD)(wc * sizeof(RGBQUAD));
	}
	return size;	
}
*/

