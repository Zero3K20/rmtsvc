/*******************************************************************
   *	Wutils.cpp
   *    DESCRIPTION:windowssystemutility functions collection
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2005-09-19
   *
   *******************************************************************/


#include "Wutils.h"
#include "stringMatch.h"

#include <stdio.h>
#include <tlhelp32.h> //enumerate all processes
#include "../net4cpp21/include/cLogger.h"

using namespace net4cpp21;

char Wutils::m_buffer[MAX_PATH]={0};
DWORD Wutils::mskbEvent_dwExtraInfo=0x3456;

inline UINT Mouse_Event(DWORD dwFlags, // motion and click options
  DWORD dx,              // horizontal position or change (absolute when MOUSEEVENTF_ABSOLUTE is set)
  DWORD dy,              // vertical position or change  (absolute when MOUSEEVENTF_ABSOLUTE is set)
  DWORD dwData)         // wheel movement
{
	INPUT inp = {0};
	inp.type = INPUT_MOUSE;
	inp.mi.dx          = (LONG)dx;
	inp.mi.dy          = (LONG)dy;
	inp.mi.mouseData   = dwData;
	inp.mi.dwFlags     = dwFlags;
	inp.mi.dwExtraInfo = Wutils::mskbEvent_dwExtraInfo;
	UINT ret = SendInput(1, &inp, sizeof(INPUT));
	if(ret == 0)
		RW_LOG_DEBUG("Mouse_Event SendInput failed: flags=0x%lx dx=%lu dy=%lu dwData=%lu err=%lu\r\n",
			dwFlags, dx, dy, dwData, (unsigned long)GetLastError());
	return ret;
}
inline UINT Keybd_Event(BYTE bVk,               // virtual-key code
  BYTE /*bScan*/,         // hardware scan code (derived from bVk via MapVirtualKey)
  DWORD dwFlags )         // function options
{
	INPUT inp = {0};
	inp.type = INPUT_KEYBOARD;
	inp.ki.wVk        = bVk;
	inp.ki.wScan      = (WORD)MapVirtualKey(bVk, MAPVK_VK_TO_VSC);
	inp.ki.dwFlags    = dwFlags;
	inp.ki.dwExtraInfo = Wutils::mskbEvent_dwExtraInfo;
	UINT ret = SendInput(1, &inp, sizeof(INPUT));
	if(ret == 0)
		RW_LOG_DEBUG("Keybd_Event SendInput failed: vk=0x%02x flags=0x%lx err=%lu\r\n",
			(unsigned)bVk, dwFlags, (unsigned long)GetLastError());
	return ret;
}

//return local machine name
const char *Wutils::computeName()
{
	DWORD retLen=MAX_PATH-1;
	if(!::GetComputerName(m_buffer,&retLen)) retLen=0;
	m_buffer[retLen]=0;
	return m_buffer;
}

//returncpuinfo
int Wutils::cpuInfo(MSOSTYPE ostype)
{
	SYSTEM_INFO sysi; ::GetSystemInfo(&sysi);
	int ret=0;
	if(ostype>=MSOS_TYPE_NT) //NT platform
	{
		if(sysi.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL)
			ret=sprintf(m_buffer,"%d cpus (x86 Family %d)",sysi.dwNumberOfProcessors,
					sysi.wProcessorLevel);
		else
			ret=sprintf(m_buffer,"%d cpus (other Family %d)",sysi.dwNumberOfProcessors,
				sysi.wProcessorLevel);
	}
	else
		ret=sprintf(m_buffer,"%d cpus (x86 Family %d)",
			sysi.dwNumberOfProcessors,sysi.dwProcessorType);
	m_buffer[ret]=0; return sysi.dwNumberOfProcessors;
}

MSOSTYPE Wutils::winOsType()
{
	MSOSTYPE ostype=MSOS_TYPE_UNKNOWED;
	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize=sizeof(vi);  // init this.
#pragma warning(suppress: 4996)
	GetVersionEx(&vi);      //lint !e534
	sprintf(m_buffer,"Unknowed OS");
	if(vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if(vi.dwMajorVersion == 5)
			ostype=MSOS_TYPE_2K; //win2000
		else
			ostype=MSOS_TYPE_NT;//NT
		sprintf(m_buffer,"Windows %s, version:%d.%d , %s",(ostype==MSOS_TYPE_NT)?"NT":"2000",
			vi.dwMajorVersion,vi.dwMinorVersion,vi.szCSDVersion);
	}
	else if(vi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
	{
		ostype=MSOS_TYPE_9X;//win9X
		if(vi.dwMinorVersion==0)
			sprintf(m_buffer,"Windows95 %s",vi.szCSDVersion);
		else
			sprintf(m_buffer,"Windows98 %s",vi.szCSDVersion);
	}
	else
	{
		ostype=MSOS_TYPE_31;//windows 3.1
		sprintf(m_buffer,"windows3.1 %s",vi.szCSDVersion);
	}
	return ostype;
}



//detect system status
MSOSSTATUS Wutils::winOsStatus()
{
	MSOSTYPE ost=Wutils::winOsType();
	sprintf(m_buffer,"status: Normal");
	MSOSSTATUS oss=MSOS_STA_NORMAL;
	//win9x does not support OpenDeskTop and openInputDesktop-related functions
	//therefore if the system is win9x, directly return Normal status
	if(ost<=MSOS_TYPE_9X) return oss;

	HDESK hDesktop=::OpenInputDesktop(0, FALSE,MAXIMUM_ALLOWED);
	if(hDesktop!=NULL)
	{
		DWORD dummy; char deskName[256]; deskName[0]=0;
		//retrieve the name of the desktop object
		::GetUserObjectInformation(hDesktop, UOI_NAME, &deskName, 256, &dummy);
		if(_stricmp(deskName,"winlogon")==0){
			oss=MSOS_STA_LOCKED;
			sprintf(m_buffer,"status: Locked");
		}
		::CloseDesktop(hDesktop);
	}
	else if(GetLastError()!=120) 
	{//if open error, the system may be at the winlogon screen
		//if error return is 120, this system does not support this function; default system left-side status to normal (0)
		//yyc comment 2005-09-23: tested that even when not logged in, hDesktop != NULL; test environment: Win2k SP4
		oss=MSOS_STA_UNLOGIN;
		sprintf(m_buffer,"status: unlogin");
	}
	return oss;
}
//lock workstation,lock workstation (only for NT)
BOOL Wutils:: LockWorkstation()
{
	// Load the user32 library
	HMODULE user32 = LoadLibrary("user32.dll");
	if (user32) {
		// Get the LockWorkstation function
		typedef BOOL (*LWProc) ();
		LWProc lockworkstation = (LWProc)GetProcAddress(user32, "LockWorkStation");
		if(lockworkstation) //LockWorkStation API requires Windows 2000 or above
		{
			// Attempt to lock the workstation
			BOOL result = (lockworkstation)();
		}//?if(lockworkstation)
		FreeLibrary(user32);
	}//?if (user32)
	else return FALSE;
	return TRUE;
}
//simulate Ctrl+Alt+Del key press
BOOL Wutils:: SimulateCtrlAltDel()
{
	HWINSTA hwinsta=NULL,hwinstaSave = NULL; 
	HDESK       hdesk = NULL, hdeskSave=NULL;
	BOOL bRet=FALSE;
//	if(Wutils::inputDesktopSelected()) //check if current desktop is the Default desktop
//	{//if yes, open winlogon desktop, because Ctrl+alt+del must be sent to winlogon desktop   
		// Save a handle to the caller's current window station.
		if ( (hwinstaSave = GetProcessWindowStation() ) == NULL)
			goto Cleanup;
		hwinsta = OpenWindowStation("winsta0", FALSE,
				WINSTA_ACCESSCLIPBOARD   | 
				WINSTA_ACCESSGLOBALATOMS |   
				WINSTA_CREATEDESKTOP     |  
				WINSTA_ENUMDESKTOPS      |    
				WINSTA_ENUMERATE         |   
				WINSTA_EXITWINDOWS       |    
				WINSTA_READATTRIBUTES    |    
				WINSTA_READSCREEN        |   
				WINSTA_WRITEATTRIBUTES);
		if(hwinsta==NULL)
		{
			sprintf(m_buffer,"failed to OpenWindowStation,error=%d\r\n",
				GetLastError());
			goto Cleanup;
		}
		if (!SetProcessWindowStation(hwinsta))
		{
			sprintf(m_buffer,"failed to SetProcessWindowStation,error=%d\r\n",
				GetLastError());
			goto Cleanup;
		}
		hdeskSave=GetThreadDesktop(GetCurrentThreadId());
		hdesk = OpenDesktop("Winlogon", 0, FALSE, 
			DESKTOP_CREATEMENU |   
			DESKTOP_CREATEWINDOW |  
			DESKTOP_ENUMERATE    |   
			DESKTOP_HOOKCONTROL  |  
			DESKTOP_JOURNALPLAYBACK |
			DESKTOP_JOURNALRECORD |   
			DESKTOP_READOBJECTS |       
			DESKTOP_SWITCHDESKTOP |   
			DESKTOP_WRITEOBJECTS);
		if(hdesk==NULL)
		{
			sprintf(m_buffer,"failed to OpenDesktop,error=%d\r\n",
				GetLastError());
			goto Cleanup;
		}
		if (!SetThreadDesktop(hdesk))
		{
			sprintf(m_buffer,"failed to SetThreadDesktop,error=%d\r\n",
				GetLastError());
			goto Cleanup; 
		}
//	}//?if(!Wutils::inputDesktopSelected())
//	else //if not Default desktop, then current input desktop may be winlogon
//	{//switch to current input desktop; use this method to confirm current input is winlogon
//		if(!Wutils::selectInputDesktop()){
//			sprintf(m_buffer,"failed to selectInputDesktop\r\n - %s\r\n",
//			Wutils::getLastInfo()); return FALSE; }
//	}
	bRet=PostMessage(HWND_BROADCAST, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));
	if(!bRet) sprintf(m_buffer,"failed to Simulate Ctrl+Alt+Del,error=%d\r\n",
		GetLastError());
Cleanup:
	if (hwinstaSave != NULL) SetProcessWindowStation (hwinstaSave);
	if (hdeskSave !=NULL ) SetThreadDesktop(hdeskSave);
	if (hwinsta) CloseWindowStation(hwinsta);
	if (hdesk) CloseDesktop(hdesk);
	return bRet;
}
//flags - mouse button status
//flag meaning: lowest 4 bits represent mouse buttons
//						0 Default. No button is pressed. 
//						1 Left button is pressed. 
//						2 Right button is pressed. 
//						3 Left and right buttons are both pressed. 
//						4 Middle button is pressed. 
//						5 Left and middle buttons both are pressed. 
//						6 Right and middle buttons are both pressed. 
//						7 All three buttons are pressed. 
//          high 4 bits represent mouse events
//						0 only Move
//						1 click
//						2 double click
//						3 drag
//						4 drop
//						5 wheel
//						6 button up (released after separate button-down, e.g. right-click on taskbar)
//      low 3 bits of low 2 bytes represent whether Ctrl Shift Alt are pressed
//          bit 0 represents whether Ctrl key is pressed: 0=no, 1=yes
//          bit 1 represents whether Shift key is pressed: 0=no, 1=yes
//          bit 2 represents whether Alt key is pressed: 0=no, 1=yes
#define MSEVENT_BUTTON_NONE 0
#define MSEVENT_BUTTON_LEFT 0x01
#define MSEVENT_BUTTON_RIGHT 0x02
#define MSEVENT_BUTTON_MIDDLE 0x04		
#define MSEVENT_EVENT_NONE 0
#define MSEVENT_EVENT_CLICK 0x10
#define MSEVENT_EVENT_DBLCLICK 0x20
#define MSEVENT_EVENT_DRAG 0x30
#define MSEVENT_EVENT_DROP 0x40
#define MSEVENT_EVENT_WHEEL 0x50
#define MSEVENT_EVENT_BUTTONUP 0x60
#define MSEVENT_EVENT_ALL 0xf0
#define MSEVENT_CTRL 0x0100
#define MSEVENT_SHIFT 0x0200
#define MSEVENT_ALT 0x0400
//dwData - wheel movement, only meaningful for MSEVENT_EVENT_WHEEL
BOOL Wutils :: sendMouseEvent(int x,int y,short flags,DWORD dwData)
{
	RW_LOG_DEBUG("sendMouseEvent: x=%d y=%d flags=0x%04x dwData=%lu\r\n",
		x, y, (unsigned)flags, (unsigned long)dwData);
	if(!Wutils::inputDesktopSelected())
	{
		RW_LOG_DEBUG("sendMouseEvent: not on input desktop (%s), switching\r\n", Wutils::getLastInfo());
		if(!Wutils::selectInputDesktop())
			RW_LOG_DEBUG("sendMouseEvent: selectInputDesktop failed: %s\r\n", Wutils::getLastInfo());
		else
			RW_LOG_DEBUG("sendMouseEvent: selectInputDesktop ok: %s\r\n", Wutils::getLastInfo());
	}
	// Move the cursor using SendInput with absolute virtual-screen coordinates.
	// This is more reliable than SetCursorPos (which can silently fail in a service
	// context) and works correctly across multiple monitors.
	{
		int screenLeft   = GetSystemMetrics(SM_XVIRTUALSCREEN);
		int screenTop    = GetSystemMetrics(SM_YVIRTUALSCREEN);
		int screenWidth  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		// Clamp input coordinates to the virtual-screen rectangle before normalising.
		int cx = x < screenLeft ? screenLeft : (x >= screenLeft + screenWidth  ? screenLeft + screenWidth  - 1 : x);
		int cy = y < screenTop  ? screenTop  : (y >= screenTop  + screenHeight ? screenTop  + screenHeight - 1 : y);
		LONG nx = (screenWidth  > 0) ? (LONG)((double)(cx - screenLeft) * 65535.0 / screenWidth  + 0.5) : 0;
		LONG ny = (screenHeight > 0) ? (LONG)((double)(cy - screenTop)  * 65535.0 / screenHeight + 0.5) : 0;
		Mouse_Event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE,
		            (DWORD)nx, (DWORD)ny, 0);
	}
	if((flags&MSEVENT_EVENT_ALL)==MSEVENT_EVENT_NONE) return TRUE;//only move cursor

	if((flags&MSEVENT_CTRL)!=0) //Ctrl pressed
		Keybd_Event((BYTE)VK_CONTROL, (BYTE)VK_CONTROL,0);
	if((flags&MSEVENT_SHIFT)!=0) //SHIFT pressed
		Keybd_Event((BYTE)VK_SHIFT, (BYTE)VK_SHIFT,0);
	if((flags&MSEVENT_ALT)!=0) //Alt pressed
		Keybd_Event((BYTE)VK_MENU, (BYTE)VK_MENU,0);
	
	//check mouse button
	int fDown=0,fUp=0;
	if((flags&MSEVENT_BUTTON_LEFT)==MSEVENT_BUTTON_LEFT) //check mouse buttonstatus
	{
		fDown|=MOUSEEVENTF_LEFTDOWN;
		fUp|=MOUSEEVENTF_LEFTUP;
	}
	if((flags&MSEVENT_BUTTON_RIGHT)==MSEVENT_BUTTON_RIGHT)
	{
		fDown|=MOUSEEVENTF_RIGHTDOWN;
		fUp|=MOUSEEVENTF_RIGHTUP;
	}
	if((flags&MSEVENT_BUTTON_MIDDLE)==MSEVENT_BUTTON_MIDDLE)
	{
		fDown|=MOUSEEVENTF_MIDDLEDOWN;
		fUp|=MOUSEEVENTF_MIDDLEUP;
	}
	
	if((flags&MSEVENT_EVENT_ALL)==MSEVENT_EVENT_DRAG)
	{//mousedrag: press the button at the drag-start position (cursor already moved there)
		Mouse_Event(fDown,0, 0,dwData);//mouse button down
	}
	else if((flags&MSEVENT_EVENT_ALL)==MSEVENT_EVENT_DROP)
	{//mousedrop: cursor already moved to drop position; release the button
		Mouse_Event(fUp, 0, 0,0);
	}
	else if((flags&MSEVENT_EVENT_ALL)==MSEVENT_EVENT_WHEEL)
	{//simulate mouse wheel event
		Mouse_Event(MOUSEEVENTF_WHEEL,0,0,dwData);
	}
	else if((flags&MSEVENT_EVENT_ALL)==MSEVENT_EVENT_BUTTONUP)
	{//button released after a separately-sent button-down (e.g. right/middle click on taskbar)
		Mouse_Event(fUp, 0, 0,0);
	}
	else //not mouse drag-drop
	{
		Mouse_Event(fDown,0, 0,dwData);//mouse button down
		Mouse_Event(fUp, 0, 0,dwData); //mouse button up
		if((flags&MSEVENT_EVENT_ALL)==MSEVENT_EVENT_DBLCLICK) //mouse double click
		{
			Mouse_Event(fDown, 0, 0,dwData);
			Mouse_Event(fUp, 0, 0,dwData);
		}//double click
	} //not mouse drag-drop

	if((flags&MSEVENT_CTRL)!=0) //Ctrl pressed
		Keybd_Event((BYTE)VK_CONTROL, (BYTE)VK_CONTROL, KEYEVENTF_KEYUP);
	if((flags&MSEVENT_SHIFT)!=0) //SHIFT pressed
		Keybd_Event((BYTE)VK_SHIFT, (BYTE)VK_SHIFT, KEYEVENTF_KEYUP);
	if((flags&MSEVENT_ALT)!=0) //Alt pressed
		Keybd_Event((BYTE)VK_MENU, (BYTE)VK_MENU, KEYEVENTF_KEYUP);
	return TRUE;
}
//send virtual key press
//low 1 byte represents the key ASCII code
//high byte represents Ctrl, Shift, Alt key status
//          lowest bit represents whether Ctrl key is pressed
//          second bit represents whether Shift key is pressed
//          third bit represents whether Alt key is pressed
BOOL Wutils :: sendKeyEvent(short vkey)
{
	RW_LOG_DEBUG("sendKeyEvent: vkey=0x%04x (key=0x%02x ctrl=%d shift=%d alt=%d)\r\n",
		(unsigned)vkey, (unsigned)(vkey&0x0ff),
		(vkey&0x0100)!=0, (vkey&0x0200)!=0, (vkey&0x0400)!=0);
	if(!Wutils::inputDesktopSelected())
	{
		RW_LOG_DEBUG("sendKeyEvent: not on input desktop (%s), switching\r\n", Wutils::getLastInfo());
		if(!Wutils::selectInputDesktop())
			RW_LOG_DEBUG("sendKeyEvent: selectInputDesktop failed: %s\r\n", Wutils::getLastInfo());
		else
			RW_LOG_DEBUG("sendKeyEvent: selectInputDesktop ok: %s\r\n", Wutils::getLastInfo());
	}
	if((vkey&0x0100)!=0) //Ctrl press
		Keybd_Event((BYTE)VK_CONTROL, (BYTE)VK_CONTROL,0);
	if((vkey&0x0200)!=0) //SHIFT pressed
		Keybd_Event((BYTE)VK_SHIFT, (BYTE)VK_SHIFT,0);
	if((vkey&0x0400)!=0) //Alt pressed
		Keybd_Event((BYTE)VK_MENU, (BYTE)VK_MENU,0);
	
	if((vkey&0x0ff)!=0)
	{
		Keybd_Event((BYTE)(vkey&0x0ff), (BYTE)(vkey&0x0ff),0);
		Keybd_Event((BYTE)(vkey&0x0ff), (BYTE)(vkey&0x0ff), KEYEVENTF_KEYUP);
	}

	if((vkey&0x0100)!=0) //Ctrl press
		Keybd_Event((BYTE)VK_CONTROL, (BYTE)VK_CONTROL,KEYEVENTF_KEYUP);
	if((vkey&0x0200)!=0) //SHIFT pressed
		Keybd_Event((BYTE)VK_SHIFT, (BYTE)VK_SHIFT,KEYEVENTF_KEYUP);
	if((vkey&0x0400)!=0) //Alt pressed
		Keybd_Event((BYTE)VK_MENU, (BYTE)VK_MENU,KEYEVENTF_KEYUP);
	return true;
}

//simulate key press to input string, can only input ASCII strings
BOOL Wutils :: sendText(const char *strTxt)
{
	if(strTxt==NULL || strTxt[0]==0) return TRUE;
	if(!Wutils::inputDesktopSelected())
		Wutils::selectInputDesktop();
	const char *ptr=strTxt;
	while(*ptr)
	{
		if(*ptr=='\n'){ ptr++; continue; }
		SHORT VkKey=VkKeyScan(*ptr);
		if(HIBYTE(VkKey)&1) Keybd_Event(VK_SHIFT,VK_SHIFT,0);
		Keybd_Event(LOBYTE(VkKey),LOBYTE(VkKey),0);
		Keybd_Event(LOBYTE(VkKey),LOBYTE(VkKey),KEYEVENTF_KEYUP);
		if(HIBYTE(VkKey)&1) Keybd_Event(VK_SHIFT,VK_SHIFT,KEYEVENTF_KEYUP);
        ptr++;
	}//?while(*ptr)
	return TRUE;
}
//input string via clipboard, can input any text
BOOL Wutils :: sendTextbyClipboard(const char *strTxt)
{
	if(strTxt==NULL || strTxt[0]==0) return TRUE;
	if(!Wutils::inputDesktopSelected())
		Wutils::selectInputDesktop();
	int len=strlen(strTxt); //write text to current window via clipboard
	if(!OpenClipboard(NULL))
	{
		sprintf(m_buffer,"failed to OpenClipboard");
		return FALSE;
	}
	// clear clipboard
	//The EmptyClipboard function empties the clipboard and frees handles to data in the clipboard. 
	//The function then assigns ownership of the clipboard to the window that currently has the clipboard open. 
	if (::EmptyClipboard())
	{
		// allocate memory block
		HANDLE hMem= ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, len+1);
		if (hMem)
		{
			LPSTR lpMem = (LPSTR)::GlobalLock(hMem);
			if (lpMem)
			{
				::memcpy((void*)lpMem, (const void*)strTxt, len+1);
				::SetClipboardData(CF_TEXT,hMem);	
			}//?if (lpMem)
			::GlobalUnlock(hMem);
		}//?hMem
	}
	CloseClipboard();
	//support console text input
	HWND hWnd=GetForegroundWindow();
	if(hWnd)
	{//check if current input focus is at console
		char rgBuf[32]; RECT rt;
		if(GetClassName(hWnd, rgBuf, 32) != 0 && \
			strcmp(rgBuf, "ConsoleWindowClass") == 0 && \
			GetWindowRect(hWnd,&rt)!=0)//get console window screen coordinates
		{//if input window is console window, simulate right mouse click at console window
			int x=rt.left+(rt.right-rt.left)/2;
			int y=rt.top+(rt.bottom -rt.top)/2;
			::SetCursorPos(x, y);//move mouse cursor to specified position
			Mouse_Event(MOUSEEVENTF_RIGHTDOWN,0,0,0);
			Mouse_Event(MOUSEEVENTF_RIGHTUP,0,0,0);
			return TRUE;
		}//
	}//?if(hWnd)
	//otherwise simulate Ctrl+V key press
	#define VK_V 0x56
	Keybd_Event((BYTE)VK_CONTROL, (BYTE)VK_CONTROL, 0);
	Keybd_Event((BYTE)VK_V, (BYTE)VK_V, 0);
	Keybd_Event((BYTE)VK_V, (BYTE)VK_V, KEYEVENTF_KEYUP);
	Keybd_Event((BYTE)VK_CONTROL, (BYTE)VK_CONTROL, KEYEVENTF_KEYUP);
	return TRUE;
}

//modify permissions of this process
BOOL  Wutils:: EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if(!::OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES |
        TOKEN_QUERY | TOKEN_READ,&hToken))
        return FALSE;
    if(!::LookupPrivilegeValue(NULL, lpszPrivilegeName, &luid))
        return TRUE;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;

    ::AdjustTokenPrivileges(hToken,FALSE,&tp,NULL,NULL,NULL);

    ::CloseHandle(hToken);

    return (GetLastError() == ERROR_SUCCESS);
}

DWORD procList_NT(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter);
DWORD procList_2K(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter);
//list all processes on local machine
//return count of processes matching filter condition, conditions separated by commas. Supports * ? wildcards
DWORD Wutils::procList(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter)
{
	MSOSTYPE ost=Wutils::winOsType();
	vecList.clear();
	if(ost==MSOS_TYPE_NT)
		return procList_NT(vecList,filter);
	return procList_2K(vecList,filter);
}

//get ID of specified remote process by name.returnPID
DWORD Wutils::GetPIDFromName(LPCTSTR szRemoteProcessName)
{
	std::vector<std::pair<DWORD,std::string> > vecList;
	DWORD ret=0;
	MSOSTYPE ost=Wutils::winOsType();
	if(ost==MSOS_TYPE_NT)
		ret=procList_NT(vecList,szRemoteProcessName);
	else
		ret=procList_2K(vecList,szRemoteProcessName);
	if(ret>0) ret=vecList[0].first; else ret=0;
	return ret;
}

BOOL GetProcName_NT(DWORD pid,char *szProcessName,DWORD buflen);
BOOL GetProcName_2K(DWORD pid,char *szProcessName,DWORD buflen);
//get process name by process ID
const char *Wutils::GetNameFromPID(DWORD pid)
{
	if(pid==0) return NULL;
	BOOL bRet=FALSE;
	MSOSTYPE ost=Wutils::winOsType();
	if(ost==MSOS_TYPE_NT)
		bRet=GetProcName_NT(pid,m_buffer,MAX_PATH);
	else
		bRet=GetProcName_2K(pid,m_buffer,MAX_PATH);
	return ((bRet)?m_buffer:NULL);
}

//****************************************************************************
//**********************private function for this file************************
//****************************** start ***************************************

// Determine whether the thread's current desktop is the input one
bool Wutils::inputDesktopSelected() 
{
  HDESK current = GetThreadDesktop(GetCurrentThreadId());
  HDESK input = OpenInputDesktop(0, FALSE,
      DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
        DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
        DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
        DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
  if (!input) {
    sprintf(m_buffer,"unable to OpenInputDesktop(1):%u", GetLastError());
    return false;
  }

  DWORD size;
  char currentname[256];
  char inputname[256];

  if (!GetUserObjectInformation(current, UOI_NAME, currentname, 256, &size)) {
    sprintf(m_buffer,"unable to GetUserObjectInformation(1):%u", GetLastError());
    CloseDesktop(input);
    return false;
  }
  if (!GetUserObjectInformation(input, UOI_NAME, inputname, 256, &size)) {
    sprintf(m_buffer,"unable to GetUserObjectInformation(2):%u", GetLastError());
    CloseDesktop(input);
    return false;
  }
  if (!CloseDesktop(input))
    sprintf(m_buffer,"unable to close input desktop:%u", GetLastError());

  sprintf(m_buffer,"current=%s, input=%s", currentname, inputname);
  bool result = strcmp(currentname, inputname) == 0;
  return result;
}

// Switch the current thread to the specified desktop
bool Wutils::switchToDesktop(HDESK desktop) 
{
  HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
  if (!SetThreadDesktop(desktop)) {
    sprintf(m_buffer,"switchToDesktop failed:%u", GetLastError());
    return false;
  }
  if (!CloseDesktop(old_desktop))
    sprintf(m_buffer,"unable to close old desktop:%u", GetLastError());
  return true;
}
// Switch the current thread into the input desktop
bool Wutils::selectInputDesktop() 
{
  // - Open the input desktop
  HDESK desktop = OpenInputDesktop(0, FALSE,
        DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
        DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
        DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
        DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
  if (!desktop) {
    sprintf(m_buffer,"unable to OpenInputDesktop:%u", GetLastError());
    return false;
  }

  // - Switch into it
  if (!switchToDesktop(desktop)) {
    CloseDesktop(desktop);
    return false;
  }

  // ***
  DWORD size = 256;
  char currentname[256]; currentname[0]=0;
  GetUserObjectInformation(desktop, UOI_NAME, currentname, 256, &size);

  sprintf(m_buffer,"switched to input desktop (%s)",currentname);
  return true;
} 

inline bool ifMatch(const char *szProcessName,const char *filter)
{
	bool bMatch=false;
	std::string tmpStr(filter);
	char *pstr=(char *)(tmpStr.c_str());
	char *token = strtok(pstr, ",");
	while( token != NULL )
	{
		bMatch|=MatchingString(szProcessName,token,false);
		token = strtok( NULL, ",");
	}//?while
	return bMatch;
}
//enumerate NT system processes
//for NT operating system, use PSAPI.DLL to enumerate processes and module info
DWORD procList_NT(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter)
{
	typedef BOOL (WINAPI *pfnEnumProcesses_D)(
						DWORD * lpidProcess,  // array to receive the process identifiers
						DWORD cb,             // size of the array
						DWORD * cbNeeded      // receives the number of bytes returned
					);
	typedef BOOL (WINAPI *pfnEnumProcessModules_D)(
					HANDLE hProcess,      // handle to the process
					HMODULE * lphModule,  // array to receive the module handles
					DWORD cb,             // size of the array
					LPDWORD lpcbNeeded    // receives the number of bytes returned
				);
	typedef DWORD (WINAPI *pfnGetModuleBaseName_D)(
					HANDLE hProcess,    // handle to the process
					HMODULE hModule,    // handle to the module
					LPTSTR lpBaseName,  // buffer that receives the base name
					DWORD nSize         // size of the buffer
				);

	HINSTANCE hDll=::LoadLibrary("PSAPI.dll");
	if(hDll==NULL) return 0;
	pfnEnumProcesses_D pfnEnumProcesses=(pfnEnumProcesses_D)::GetProcAddress(hDll,"EnumProcesses");
	pfnEnumProcessModules_D pfnEnumProcessModules=(pfnEnumProcessModules_D)::GetProcAddress(hDll,"EnumProcessModules");
	pfnGetModuleBaseName_D pfnGetModuleBaseName=(pfnGetModuleBaseName_D)::GetProcAddress(hDll,"GetModuleBaseNameA");
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	//enumsystemprocess IDlist
	if (pfnEnumProcesses!=NULL && (*pfnEnumProcesses)(aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		cProcesses = cbNeeded / sizeof(DWORD);
		HANDLE hProcess;
		char szProcessName[MAX_PATH];
		int filternums=0;//filter condition count
		if(filter && filter[0]!=0 )
			filternums=(strchr(filter,','))?2:1;//2 means multiple
		for (unsigned int i = 0; i < cProcesses; i++ )
		{
			strcpy(szProcessName,"unknown");
			if((hProcess = ::OpenProcess( PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE, aProcesses[i])) )
			{
				HMODULE hMod;DWORD cbNeeded;
				if ((*pfnEnumProcessModules)(hProcess, &hMod, sizeof(hMod), &cbNeeded) && pfnGetModuleBaseName)
					(*pfnGetModuleBaseName)( hProcess, hMod, szProcessName,sizeof(szProcessName) );
				::CloseHandle( hProcess );
			}
			bool bMatch=true;
			if(filternums==1) //single filter condition
				bMatch=MatchingString(szProcessName,filter,false);
			else if(filternums>1)
				bMatch=ifMatch(szProcessName,filter);
			if(bMatch)
			{
				std::pair<DWORD,std::string> p(aProcesses[i],szProcessName);
				vecList.push_back(p);
			}
		}//?for(...
	}//?if (pfnEnumProcesses!=NULL &&
	::FreeLibrary(hDll);
	return vecList.size();
}

//enumerate win9x/2k system processes
//for win9x/2k, use toolhelp32 functions to enumerate processes and module info
//only 2k&&win9x support CreateToolhelp32Snapshot and similar functions
DWORD procList_2K(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter)
{
	HINSTANCE hDll=::LoadLibrary("KERNEL32.dll");
	if(hDll==NULL) return 0;
	typedef HANDLE (WINAPI *pfnCreateToolhelp32Snapshot_D)(DWORD,DWORD);
	typedef BOOL (WINAPI *pfnProcess32Next_D)(HANDLE,LPPROCESSENTRY32);
	//yyc modify 2003-04-19
	typedef BOOL (WINAPI *pfnProcess32First_D)(HANDLE,LPPROCESSENTRY32);
	pfnProcess32First_D pfnProcess32First=(pfnProcess32First_D)::GetProcAddress(hDll,"Process32First");
	//yyc modify end 2003-04-19
	pfnCreateToolhelp32Snapshot_D pfnCreateToolhelp32Snapshot=(pfnCreateToolhelp32Snapshot_D)::GetProcAddress(hDll,"CreateToolhelp32Snapshot");
	pfnProcess32Next_D pfnProcess32Next=(pfnProcess32Next_D)::GetProcAddress(hDll,"Process32Next");
	
	HANDLE hSnapShot=(*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS,0);
	 PROCESSENTRY32* processInfo=new PROCESSENTRY32;
	 memset((void *)processInfo,0,sizeof(PROCESSENTRY32));
	 processInfo->dwSize=sizeof(PROCESSENTRY32);

	 if ((*pfnProcess32First)(hSnapShot, processInfo))
	 {
		const char *ptrFilename=NULL;
		int filternums=0;//filter condition count
		if(filter && filter[0]!=0 )
			filternums=(strchr(filter,','))?2:1;//2 means multiple
		do
		{
			//in win9x, display is the full file path; remove the path prefix
			//in 2k, only the filename is displayed (so this check can be skipped)
			//yyc modify 2003-04-20
			if((ptrFilename=strrchr(processInfo->szExeFile,'\\'))==NULL) 
				ptrFilename=processInfo->szExeFile;
			else
				ptrFilename+=1;//remove '\'
			
			bool bMatch=true;
			if(filternums==1) //single filter condition
				bMatch=MatchingString(ptrFilename,filter,false);
			else if(filternums>1)
				bMatch=ifMatch(ptrFilename,filter);
			if(bMatch)
			{
				std::pair<DWORD,std::string> p(processInfo->th32ProcessID,ptrFilename);
				vecList.push_back(p);
			}
		}while ((*pfnProcess32Next)(hSnapShot,processInfo));
	}//?if ((*pfnProcess32First)(hSnapShot, processInfo))
	CloseHandle(hSnapShot);
	delete processInfo;
	::FreeLibrary(hDll);
	return vecList.size();
}

//enumerate NT system processes
//for NT operating system, use PSAPI.DLL to enumerate processes and module info
BOOL GetProcName_NT(DWORD processID,char *szProcessName,DWORD buflen)
{
	typedef BOOL (WINAPI *pfnEnumProcessModules_D)(
				HANDLE hProcess,      // handle to the process
				HMODULE * lphModule,  // array to receive the module handles
				DWORD cb,             // size of the array
				LPDWORD lpcbNeeded    // receives the number of bytes returned
				);
	typedef DWORD (WINAPI *pfnGetModuleBaseName_D)(
					HANDLE hProcess,    // handle to the process
					HMODULE hModule,    // handle to the module
					LPTSTR lpBaseName,  // buffer that receives the base name
					DWORD nSize         // size of the buffer
				);
	typedef DWORD (WINAPI *pfnGetModuleFileNameEx_D)(
					HANDLE hProcess,    // handle to the process
					HMODULE hModule,    // handle to the module
					LPTSTR lpFilename,  // buffer that receives the path
					DWORD nSize         // size of the buffer
				);
	HINSTANCE hDll=::LoadLibrary("PSAPI.dll");
	if(hDll==NULL) return FALSE;

	pfnEnumProcessModules_D pfnEnumProcessModules=(pfnEnumProcessModules_D)::GetProcAddress(hDll,"EnumProcessModules");
	pfnGetModuleBaseName_D pfnGetModuleBaseName=(pfnGetModuleBaseName_D)::GetProcAddress(hDll,"GetModuleBaseNameA");
	pfnGetModuleFileNameEx_D pfnGetModuleFileNameEx=(pfnGetModuleFileNameEx_D)::GetProcAddress(hDll,"GetModuleFileNameExA");

	HMODULE aModules[1024];DWORD cbNeeded, cModules;
	HANDLE hProcess = ::OpenProcess( PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE, processID);
	
	BOOL bRet=(hProcess)?( (*pfnEnumProcessModules)(hProcess, aModules,sizeof(aModules), &cbNeeded ) ):FALSE;
	if(bRet)
	{
		cModules = cbNeeded / sizeof(DWORD);
		if(cModules>0 && pfnGetModuleFileNameEx)
			(*pfnGetModuleFileNameEx)( hProcess, aModules[0], szProcessName,buflen );
		else
			strcpy(szProcessName,"unknown path");
	}
	
	::FreeLibrary(hDll);
	return bRet;
}

//enumerate win9x/2k system process modules
//for win9x/2k, use toolhelp32 functions to enumerate processes and module info
//only 2k&&win9x support CreateToolhelp32Snapshot and similar functions
BOOL GetProcName_2K(DWORD processID,char *szProcessName,DWORD buflen)
{
	HINSTANCE hDll=::LoadLibrary("KERNEL32.dll");
	if(hDll==NULL) return FALSE;
	typedef HANDLE (WINAPI *pfnCreateToolhelp32Snapshot_D)(DWORD,DWORD);
	typedef BOOL (WINAPI *pfnModule32Next_D)(HANDLE,LPMODULEENTRY32);
	typedef BOOL (WINAPI *pfnModule32First_D)(HANDLE,LPMODULEENTRY32);
	pfnModule32First_D pfnModule32First=(pfnModule32First_D)::GetProcAddress(hDll,"Module32First");
	pfnCreateToolhelp32Snapshot_D pfnCreateToolhelp32Snapshot=(pfnCreateToolhelp32Snapshot_D)::GetProcAddress(hDll,"CreateToolhelp32Snapshot");
	pfnModule32Next_D pfnModule32Next=(pfnModule32Next_D)::GetProcAddress(hDll,"Module32Next");
	
	HANDLE hSnapShot=(*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPMODULE,processID);

	MODULEENTRY32* moduleInfo=new MODULEENTRY32;
	memset((void *)moduleInfo,0,sizeof(MODULEENTRY32));
	moduleInfo->dwSize=sizeof(MODULEENTRY32);
	BOOL bRet=(*pfnModule32First)(hSnapShot, moduleInfo);
	if (bRet) strcpy(szProcessName,moduleInfo->szExePath);
	
	CloseHandle(hSnapShot);
	delete moduleInfo;
	::FreeLibrary(hDll);
	return bRet;
}

//******************************* end ****************************************
//**********************private function for this file************************
//****************************************************************************

