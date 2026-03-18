/*
**	FILENAME			IPF.h
**
**	PURPOSE				open, save and convert image file formats
**						currently supports only BMP image format
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




//open bitmap file -- 
//[in] filename ---- bitmap filename
//[out] lpbi ---- return bitmap info
//[out] lppBits ---- return bitmap data pointer (DWORD-aligned); user must ensure sufficient space
//				if lppBits==NULL, only return bitmap info
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
//save BMP file -- 
//[in] filename ---- destination bitmap filename
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

//capture window image --- 24-bit true color image
//if hWnd==NULL, capture the entire screen
//lpbih --- returns image info
//lpBits --- save image data
//			if==NULL, only return the space size needed for image data
//failurereturn0，otherwisereturnimagedatasize
//ifCapCursorwhethercapture mouse cursor
IPFRESULT cImageF::capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor)
{
	if(lpbih==NULL) return 0;
	if(hWnd == NULL)
		if((hWnd = ::GetDesktopWindow())==NULL) return 0;
	
	RECT rect;
	::GetClientRect(hWnd, &rect);

	//starting point coordinates and width/height of the original image area to save
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
		//Attach current thread to the specified window thread
		//get the current mouse cursor handle of that window
		//Deattach
		//!!!if not done this way, calling GetCursor() directly will only get the current thread's cursor handle
		//if not set, the cursor handle retrieved will be the hourglass cursor handle
		HWND hw=::WindowFromPoint(ptCursor);
		if(hw==NULL) hw=::GetDesktopWindow();
		DWORD hdl=::GetWindowThreadProcessId(hw,NULL);
		::AttachThreadInput(::GetCurrentThreadId(),hdl,TRUE);
		HCURSOR hCursor=::GetCursor();
		::AttachThreadInput(::GetCurrentThreadId(),hdl,FALSE);
		//get cursor icon data 
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
		//draw the cursor on the compatible device context
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
	::SelectObject(hMemDC, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWnd, hWndDC);
	return dwRet;
}

//***********************************************************************************************
//**********************************************private function ********************************
/*
//get palette size of the specified DIB (in bytes)
WORD cImageF::PaletteSize(LPBITMAPINFOHEADER lpbih)
{
	WORD size, wBitCount;

	// get number of bits per pixel
	wBitCount =lpbih->biBitCount;
				
	// 16-bit and 32-bit bitmaps use three DWORD values in the color table to represent
	// red, green, blue masks in the bit data
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
					break;          // monochrome bitmap, only black and white
				case 4:
					wc=16;		
					break;			// standard VGA bitmap, 16 colors
				case 8:
					wc=256;		
					break;			// SVGA bitmap, 256 colors
				case	16:			// 64K-color bitmap
				case	24:			// 16M-color bitmap (true color)
				case	32:			// 16M+ color bitmap (true color)
					wc=0;		
					break;			// no color data in color table, return 0
				default:
					return 0; //error
			}
		}
		size = (WORD)(wc * sizeof(RGBQUAD));
	}
	return size;	
}
*/

