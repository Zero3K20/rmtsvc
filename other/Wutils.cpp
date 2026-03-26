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

// Returns true for virtual keys that must be sent with KEYEVENTF_EXTENDEDKEY
// so that the Windows input system routes them to the correct scan-code prefix
// (0xE0). Without this flag, navigation keys (arrows, Delete, Home/End, etc.)
// are silently misrouted in many applications (including MFC controls used by
// TortoiseGit).  List derived from the Windows SDK and DWService agent.
static inline bool IsExtendedKey(BYTE bVk)
{
	switch (bVk)
	{
	case VK_RCONTROL:  // 0xA3 Right Ctrl
	case VK_RMENU:     // 0xA5 Right Alt
	case VK_INSERT:    // 0x2D
	case VK_DELETE:    // 0x2E
	case VK_HOME:      // 0x24
	case VK_END:       // 0x23
	case VK_PRIOR:     // 0x21 Page Up
	case VK_NEXT:      // 0x22 Page Down
	case VK_LEFT:      // 0x25
	case VK_RIGHT:     // 0x27
	case VK_UP:        // 0x26
	case VK_DOWN:      // 0x28
	case VK_NUMLOCK:   // 0x90
	case VK_SNAPSHOT:  // 0x2C Print Screen
	case VK_DIVIDE:    // 0x6F Numpad /
	case VK_LWIN:      // 0x5B
	case VK_RWIN:      // 0x5C
	case VK_APPS:      // 0x5D Application key
		return true;
	default:
		return false;
	}
}

// Fill one INPUT slot for a keyboard event and advance the count.
static inline void AppendKeyInput(INPUT &inp, BYTE bVk, DWORD dwFlags)
{
	memset(&inp, 0, sizeof(INPUT));
	inp.type = INPUT_KEYBOARD;
	inp.ki.wVk        = bVk;
	inp.ki.wScan      = (WORD)MapVirtualKey(bVk, MAPVK_VK_TO_VSC);
	if (IsExtendedKey(bVk))
		dwFlags |= KEYEVENTF_EXTENDEDKEY;
	inp.ki.dwFlags    = dwFlags;
	inp.ki.dwExtraInfo = Wutils::mskbEvent_dwExtraInfo;
}

// Fill one INPUT slot for a mouse button or wheel event (no position data).
static inline void AppendMouseInput(INPUT &inp, DWORD dwFlags, DWORD mouseData = 0)
{
	memset(&inp, 0, sizeof(INPUT));
	inp.type = INPUT_MOUSE;
	inp.mi.dwFlags     = dwFlags;
	inp.mi.mouseData   = mouseData;
	inp.mi.dwExtraInfo = Wutils::mskbEvent_dwExtraInfo;
}

// Normalize a virtual-screen pixel coordinate pair (in, in) to the 0-65535
// range that MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK requires.
// Coordinates are clamped to the virtual-screen rectangle before normalising.
static inline void NormalizeToVirtualScreen(int x, int y, LONG &nx, LONG &ny)
{
	int screenLeft   = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int screenTop    = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int screenWidth  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	int cx = x < screenLeft ? screenLeft : (x >= screenLeft + screenWidth  ? screenLeft + screenWidth  - 1 : x);
	int cy = y < screenTop  ? screenTop  : (y >= screenTop  + screenHeight ? screenTop  + screenHeight - 1 : y);
	nx = (screenWidth  > 0) ? (LONG)((double)(cx - screenLeft) * 65535.0 / screenWidth  + 0.5) : 0;
	ny = (screenHeight > 0) ? (LONG)((double)(cy - screenTop)  * 65535.0 / screenHeight + 0.5) : 0;
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
			ret=sprintf(m_buffer,"%d CPUs (x86 Family %d)",sysi.dwNumberOfProcessors,
					sysi.wProcessorLevel);
		else
			ret=sprintf(m_buffer,"%d CPUs (other Family %d)",sysi.dwNumberOfProcessors,
				sysi.wProcessorLevel);
	}
	else
		ret=sprintf(m_buffer,"%d CPUs (x86 Family %d)",
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
// Tracks the Ctrl/Shift/Alt modifier keys that are currently held in the
// injected input stream.  Maintained across sendMouseEvent calls so that
// modifier keys stay pressed persistently while the client reports them
// (e.g. during continuous mouse movement with Ctrl/Shift held), rather than
// being press-released around every individual event.  sendKeyEvent resets
// this to 0 because it always releases any modifiers it injected.
static short s_injectedModifiers = 0;

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
	// Maximum entries for the mouse batch:
	//   1  cursor MOVE
	//  12  button events  (3 buttons × down+up × 2 for double-click)
	// ---- total 13, so 14 is a safe upper bound.
	INPUT inputs[14];
	int count = 0;

	// --- 1. Move cursor using absolute virtual-screen coordinates ---
	{
		LONG nx, ny;
		NormalizeToVirtualScreen(x, y, nx, ny);
		memset(&inputs[count], 0, sizeof(INPUT));
		inputs[count].type = INPUT_MOUSE;
		inputs[count].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE;
		inputs[count].mi.dx = nx;
		inputs[count].mi.dy = ny;
		inputs[count].mi.dwExtraInfo = Wutils::mskbEvent_dwExtraInfo;
		count++;
	}

	// --- 2. Sync modifier key state ---
	// Modifier keys are kept persistently held (or released) based on the
	// altk flags reported by the client.  This means Ctrl/Shift/Alt stay
	// injected across consecutive mouse-move events (hover, drag, etc.) so
	// that applications like Windows Explorer can respond to
	// Ctrl/Shift+hover-selection and multi-selection clicks correctly.
	// Only the transitions (keys that changed state) are injected.
	{
		short newMods = (flags & (MSEVENT_CTRL | MSEVENT_SHIFT | MSEVENT_ALT));
		short toRelease = s_injectedModifiers & ~newMods;
		short toPress   = newMods & ~s_injectedModifiers;
		if (toRelease || toPress)
		{
			INPUT modInputs[6]; // up to 3 releases + 3 presses
			int modCount = 0;
			// Release in reverse order: Alt, Shift, Ctrl
			if (toRelease & MSEVENT_ALT)   { AppendKeyInput(modInputs[modCount], VK_MENU,    KEYEVENTF_KEYUP); modCount++; }
			if (toRelease & MSEVENT_SHIFT) { AppendKeyInput(modInputs[modCount], VK_SHIFT,   KEYEVENTF_KEYUP); modCount++; }
			if (toRelease & MSEVENT_CTRL)  { AppendKeyInput(modInputs[modCount], VK_CONTROL, KEYEVENTF_KEYUP); modCount++; }
			// Press in order: Ctrl, Shift, Alt
			if (toPress & MSEVENT_CTRL)  { AppendKeyInput(modInputs[modCount], VK_CONTROL, 0); modCount++; }
			if (toPress & MSEVENT_SHIFT) { AppendKeyInput(modInputs[modCount], VK_SHIFT,   0); modCount++; }
			if (toPress & MSEVENT_ALT)   { AppendKeyInput(modInputs[modCount], VK_MENU,    0); modCount++; }
			UINT ret = SendInput((UINT)modCount, modInputs, sizeof(INPUT));
			if (ret != (UINT)modCount)
				RW_LOG_DEBUG("sendMouseEvent: modifier-sync SendInput sent %u of %d, err=%lu\r\n",
					ret, modCount, (unsigned long)GetLastError());
			s_injectedModifiers = newMods;
		}
	}

	if((flags & MSEVENT_EVENT_ALL) == MSEVENT_EVENT_NONE)
	{
		SendInput((UINT)count, inputs, sizeof(INPUT));
		return TRUE; // cursor-move only
	}

	int evType   = (flags & MSEVENT_EVENT_ALL);
	int btnFlags = (flags & 0x0f);

	// --- 3. Mouse button / wheel event ---
	// Modifier state is already correct (set in step 2 above); the cursor
	// move and button events are batched together so the OS sees them as
	// a single atomic operation.
	if (evType == MSEVENT_EVENT_WHEEL)
	{
		DWORD delta = ((LONG)dwData > 0) ? (DWORD)WHEEL_DELTA : (DWORD)(-(LONG)WHEEL_DELTA);
		AppendMouseInput(inputs[count], MOUSEEVENTF_WHEEL, delta); count++;
	}
	else if (evType == MSEVENT_EVENT_DRAG)
	{
		if (btnFlags & MSEVENT_BUTTON_LEFT)   { AppendMouseInput(inputs[count], MOUSEEVENTF_LEFTDOWN);   count++; }
		if (btnFlags & MSEVENT_BUTTON_RIGHT)  { AppendMouseInput(inputs[count], MOUSEEVENTF_RIGHTDOWN);  count++; }
		if (btnFlags & MSEVENT_BUTTON_MIDDLE) { AppendMouseInput(inputs[count], MOUSEEVENTF_MIDDLEDOWN); count++; }
	}
	else if (evType == MSEVENT_EVENT_DROP || evType == MSEVENT_EVENT_BUTTONUP)
	{
		if (btnFlags & MSEVENT_BUTTON_LEFT)   { AppendMouseInput(inputs[count], MOUSEEVENTF_LEFTUP);   count++; }
		if (btnFlags & MSEVENT_BUTTON_RIGHT)  { AppendMouseInput(inputs[count], MOUSEEVENTF_RIGHTUP);  count++; }
		if (btnFlags & MSEVENT_BUTTON_MIDDLE) { AppendMouseInput(inputs[count], MOUSEEVENTF_MIDDLEUP); count++; }
	}
	else // click or double-click
	{
		if (btnFlags & MSEVENT_BUTTON_LEFT)
		{
			AppendMouseInput(inputs[count], MOUSEEVENTF_LEFTDOWN); count++;
			AppendMouseInput(inputs[count], MOUSEEVENTF_LEFTUP);   count++;
			if (evType == MSEVENT_EVENT_DBLCLICK)
			{
				AppendMouseInput(inputs[count], MOUSEEVENTF_LEFTDOWN); count++;
				AppendMouseInput(inputs[count], MOUSEEVENTF_LEFTUP);   count++;
			}
		}
		if (btnFlags & MSEVENT_BUTTON_RIGHT)
		{
			AppendMouseInput(inputs[count], MOUSEEVENTF_RIGHTDOWN); count++;
			AppendMouseInput(inputs[count], MOUSEEVENTF_RIGHTUP);   count++;
			if (evType == MSEVENT_EVENT_DBLCLICK)
			{
				AppendMouseInput(inputs[count], MOUSEEVENTF_RIGHTDOWN); count++;
				AppendMouseInput(inputs[count], MOUSEEVENTF_RIGHTUP);   count++;
			}
		}
		if (btnFlags & MSEVENT_BUTTON_MIDDLE)
		{
			AppendMouseInput(inputs[count], MOUSEEVENTF_MIDDLEDOWN); count++;
			AppendMouseInput(inputs[count], MOUSEEVENTF_MIDDLEUP);   count++;
			if (evType == MSEVENT_EVENT_DBLCLICK)
			{
				AppendMouseInput(inputs[count], MOUSEEVENTF_MIDDLEDOWN); count++;
				AppendMouseInput(inputs[count], MOUSEEVENTF_MIDDLEUP);   count++;
			}
		}
	}

	{
		UINT ret = SendInput((UINT)count, inputs, sizeof(INPUT));
		if(ret != (UINT)count)
			RW_LOG_DEBUG("sendMouseEvent: SendInput sent %u of %d inputs, err=%lu\r\n",
				ret, count, (unsigned long)GetLastError());
	}
	return TRUE;
}
//send virtual key press
//low 1 byte represents the key ASCII code
//bits 8-10 of the value represent Ctrl, Shift, Alt key status (same encoding as mouse events):
//          bit 8  (0x0100) represents whether Ctrl key is pressed
//          bit 9  (0x0200) represents whether Shift key is pressed
//          bit 10 (0x0400) represents whether Alt key is pressed
//bit 11    (0x0800) is a special flag meaning "held-modifier release": the client
//          was holding this modifier as part of key combinations and is now
//          releasing it.  The low byte is the VK code of the modifier being
//          released.  The server injects only a key-up for that modifier (no
//          press+release), keeping it atomic with any prior held injections.
BOOL Wutils :: sendKeyEvent(short vkey)
{
	RW_LOG_DEBUG("sendKeyEvent: vkey=0x%04x (key=0x%02x ctrl=%d shift=%d alt=%d heldrel=%d)\r\n",
		(unsigned short)vkey, (unsigned)(vkey&0x0ff),
		(vkey&0x0100)!=0, (vkey&0x0200)!=0, (vkey&0x0400)!=0,
		((unsigned short)vkey&0x0800u)!=0);
	if(!Wutils::inputDesktopSelected())
	{
		RW_LOG_DEBUG("sendKeyEvent: not on input desktop (%s), switching\r\n", Wutils::getLastInfo());
		if(!Wutils::selectInputDesktop())
			RW_LOG_DEBUG("sendKeyEvent: selectInputDesktop failed: %s\r\n", Wutils::getLastInfo());
		else
			RW_LOG_DEBUG("sendKeyEvent: selectInputDesktop ok: %s\r\n", Wutils::getLastInfo());
	}

	// Maximum entries: 3 mod transitions + 1 key down + 1 key up = 5; 8 is a safe bound.
	INPUT inputs[8];
	int count = 0;

	// Bit 11 (0x0800): explicit held-modifier release sent by the client when a
	// modifier key that was held for key combinations is released.  Inject only a
	// key-up for the modifier; do not inject a press+release (bare-tap) sequence.
	if (((unsigned short)vkey & 0x0800u) != 0)
	{
		BYTE key = (BYTE)(vkey & 0x0ff);
		if ((key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT) &&
		    (s_injectedModifiers & MSEVENT_SHIFT))
		{
			AppendKeyInput(inputs[count], VK_SHIFT, KEYEVENTF_KEYUP); count++;
			s_injectedModifiers &= ~MSEVENT_SHIFT;
		}
		else if ((key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL) &&
		         (s_injectedModifiers & MSEVENT_CTRL))
		{
			AppendKeyInput(inputs[count], VK_CONTROL, KEYEVENTF_KEYUP); count++;
			s_injectedModifiers &= ~MSEVENT_CTRL;
		}
		else if ((key == VK_MENU || key == VK_LMENU || key == VK_RMENU) &&
		         (s_injectedModifiers & MSEVENT_ALT))
		{
			AppendKeyInput(inputs[count], VK_MENU, KEYEVENTF_KEYUP); count++;
			s_injectedModifiers &= ~MSEVENT_ALT;
		}
		// If the modifier wasn't tracked as held (e.g. a mouse event already
		// released it), skip the injection — a redundant key-up is harmless but
		// the count=0 path below handles the no-op case gracefully.
		if (count > 0)
		{
			UINT ret = SendInput((UINT)count, inputs, sizeof(INPUT));
			if (ret != (UINT)count)
				RW_LOG_DEBUG("sendKeyEvent(heldrel): SendInput sent %u of %d, err=%lu\r\n",
					ret, count, (unsigned long)GetLastError());
		}
		return TRUE;
	}

	// Normal key event (or bare modifier tap from keyup).
	// Instead of always injecting all modifier keys down then up (which releases
	// and re-presses Shift between consecutive Shift+Arrow auto-repeat events and
	// resets the selection anchor), we keep modifiers persistently held via
	// s_injectedModifiers and only inject the transitions that are needed.
	// This mirrors the approach already used by sendMouseEvent.

	short eventMods = 0;
	if ((vkey & 0x0100) != 0) eventMods |= MSEVENT_CTRL;
	if ((vkey & 0x0200) != 0) eventMods |= MSEVENT_SHIFT;
	if ((vkey & 0x0400) != 0) eventMods |= MSEVENT_ALT;

	BYTE key = (BYTE)(vkey & 0x0ff);

	// Sync modifier state: only inject the transitions (same logic as sendMouseEvent).
	short toRelease = s_injectedModifiers & ~eventMods;
	short toPress   = eventMods & ~s_injectedModifiers;
	// Release in reverse order: Alt, Shift, Ctrl
	if (toRelease & MSEVENT_ALT)   { AppendKeyInput(inputs[count], VK_MENU,    KEYEVENTF_KEYUP); count++; }
	if (toRelease & MSEVENT_SHIFT) { AppendKeyInput(inputs[count], VK_SHIFT,   KEYEVENTF_KEYUP); count++; }
	if (toRelease & MSEVENT_CTRL)  { AppendKeyInput(inputs[count], VK_CONTROL, KEYEVENTF_KEYUP); count++; }
	// Press in order: Ctrl, Shift, Alt
	if (toPress & MSEVENT_CTRL)  { AppendKeyInput(inputs[count], VK_CONTROL, 0); count++; }
	if (toPress & MSEVENT_SHIFT) { AppendKeyInput(inputs[count], VK_SHIFT,   0); count++; }
	if (toPress & MSEVENT_ALT)   { AppendKeyInput(inputs[count], VK_MENU,    0); count++; }
	s_injectedModifiers = eventMods;

	if (key != 0)
	{
		// Is the key itself a modifier VK?  (Sent from the client's keyup handler
		// for standalone modifier presses, e.g. Alt to focus the menu bar.)
		bool keyIsModifier =
			(key == VK_SHIFT   || key == VK_LSHIFT   || key == VK_RSHIFT   ||
			 key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL ||
			 key == VK_MENU    || key == VK_LMENU    || key == VK_RMENU);

		if (keyIsModifier)
		{
			// If the modifier sync step already released this key (it was held
			// via prior key events and the client's altk bits no longer include
			// it), the release has already been injected above — do not add
			// another press+release.  If it was NOT held (bare modifier tap,
			// e.g. Alt to open the Windows menu bar), inject a press+release.
			bool wasReleasedBySync =
				((key == VK_SHIFT   || key == VK_LSHIFT   || key == VK_RSHIFT)   && (toRelease & MSEVENT_SHIFT)) ||
				((key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL) && (toRelease & MSEVENT_CTRL))  ||
				((key == VK_MENU    || key == VK_LMENU    || key == VK_RMENU)    && (toRelease & MSEVENT_ALT));
			if (!wasReleasedBySync)
			{
				// Bare modifier tap — inject press+release.
				AppendKeyInput(inputs[count], key, 0);               count++;
				AppendKeyInput(inputs[count], key, KEYEVENTF_KEYUP); count++;
			}
		}
		else
		{
			// Regular key: inject press and release.
			// Modifiers are left held in s_injectedModifiers so that consecutive
			// auto-repeat events (e.g. Shift+Down held for text selection) do not
			// momentarily release and re-press the modifier between keystrokes,
			// which would reset the selection anchor in apps like WinDbg.
			AppendKeyInput(inputs[count], key, 0);               count++;
			AppendKeyInput(inputs[count], key, KEYEVENTF_KEYUP); count++;
		}
	}

	if (count > 0)
	{
		UINT ret = SendInput((UINT)count, inputs, sizeof(INPUT));
		if(ret != (UINT)count)
			RW_LOG_DEBUG("sendKeyEvent: SendInput sent %u of %d inputs, err=%lu\r\n",
				ret, count, (unsigned long)GetLastError());
		// s_injectedModifiers is maintained across calls so modifiers stay held
		// between consecutive keystrokes with the same modifier bits.  It will be
		// synced (and released if needed) by subsequent sendKeyEvent or
		// sendMouseEvent calls.
	}
	return TRUE;
}

//simulate key press to input string, can only input ASCII strings
BOOL Wutils :: sendText(const char *strTxt)
{
	if(strTxt==NULL || strTxt[0]==0) return TRUE;
	if(!Wutils::inputDesktopSelected())
		Wutils::selectInputDesktop();
	const char *ptr=strTxt;
	INPUT inp[4]; // reused for each character: [shift-down,] key-down, key-up, [shift-up]
	while(*ptr)
	{
		if(*ptr=='\n'){ ptr++; continue; }
		SHORT VkKey=VkKeyScan(*ptr);
		BYTE vk = LOBYTE(VkKey);
		bool needShift = (HIBYTE(VkKey) & 1) != 0;
		// Batch shift (if needed) + key-down + key-up into one SendInput call
		int cnt = 0;
		if (needShift) { AppendKeyInput(inp[cnt], VK_SHIFT, 0); cnt++; }
		AppendKeyInput(inp[cnt], vk, 0);            cnt++;
		AppendKeyInput(inp[cnt], vk, KEYEVENTF_KEYUP); cnt++;
		if (needShift) { AppendKeyInput(inp[cnt], VK_SHIFT, KEYEVENTF_KEYUP); cnt++; }
		SendInput((UINT)cnt, inp, sizeof(INPUT));
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
			// Use absolute-position SendInput so the move and click are injected
			// through the same input mechanism as all other events.
			{
				LONG nx, ny;
				NormalizeToVirtualScreen(x, y, nx, ny);
				INPUT inp[3];
				memset(inp, 0, sizeof(inp));
				inp[0].type = INPUT_MOUSE;
				inp[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE;
				inp[0].mi.dx = nx;
				inp[0].mi.dy = ny;
				inp[0].mi.dwExtraInfo = Wutils::mskbEvent_dwExtraInfo;
				AppendMouseInput(inp[1], MOUSEEVENTF_RIGHTDOWN);
				AppendMouseInput(inp[2], MOUSEEVENTF_RIGHTUP);
				SendInput(3, inp, sizeof(INPUT));
			}
			return TRUE;
		}//
	}//?if(hWnd)
	//otherwise simulate Ctrl+V key press
	#define VK_V 0x56
	{
		INPUT inp[4];
		AppendKeyInput(inp[0], VK_CONTROL, 0);
		AppendKeyInput(inp[1], VK_V,       0);
		AppendKeyInput(inp[2], VK_V,       KEYEVENTF_KEYUP);
		AppendKeyInput(inp[3], VK_CONTROL, KEYEVENTF_KEYUP);
		SendInput(4, inp, sizeof(INPUT));
	}
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

// Cached WinSta0 handle used when the process runs as a Windows service.
// OpenInputDesktop fails in Session 0, so we open WinSta0 directly and keep
// this handle open for the lifetime of the process (one handle per service).
static HWINSTA s_hWinSta0 = NULL;

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
    // OpenInputDesktop fails when running as a Windows service in Session 0.
    // If we have already switched the process to WinSta0 and the thread is on
    // the "Default" desktop, we are correctly positioned on the interactive
    // desktop – no further switch is needed.
    if (s_hWinSta0 != NULL) {
      DWORD size = 256;
      char currentname[256]; currentname[0] = 0;
      if (GetUserObjectInformation(current, UOI_NAME, currentname, 256, &size) &&
          strcmp(currentname, "Default") == 0) {
        sprintf(m_buffer, "service: already on WinSta0\\Default");
        return true;
      }
    }
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
  // - Try to open the input desktop the standard way.
  HDESK desktop = OpenInputDesktop(0, FALSE,
        DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
        DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
        DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
        DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
  if (!desktop) {
    // OpenInputDesktop fails when the process runs as a Windows service
    // (Session 0 isolation on Windows Vista and later).  Fall back to opening
    // WinSta0\Default directly so that screen capture functions such as
    // GetDC(NULL) and BitBlt access the interactive user's desktop instead of
    // the invisible Session 0 desktop.
    if (!s_hWinSta0) {
      s_hWinSta0 = OpenWindowStation("winsta0", FALSE,
            WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ACCESSCLIPBOARD |
            WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS |
            WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN |
            STANDARD_RIGHTS_REQUIRED);
      if (!s_hWinSta0) {
        sprintf(m_buffer, "unable to OpenWindowStation(winsta0):%u", GetLastError());
        return false;
      }
      // Switch the process window station to WinSta0 so that GetDC(NULL)
      // returns a DC for the interactive user's screen, not the Session 0
      // desktop.  The handle is intentionally kept open for the lifetime of
      // the process so the window station reference remains valid.
      if (!SetProcessWindowStation(s_hWinSta0)) {
        sprintf(m_buffer, "unable to SetProcessWindowStation:%u", GetLastError());
        CloseWindowStation(s_hWinSta0);
        s_hWinSta0 = NULL;
        return false;
      }
    }
    desktop = OpenDesktop("Default", 0, FALSE,
          DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
          DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
          DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS);
    if (!desktop) {
      sprintf(m_buffer, "unable to OpenDesktop(Default):%u", GetLastError());
      return false;
    }
  }

  // - Switch the current thread into the selected desktop.
  if (!switchToDesktop(desktop)) {
    CloseDesktop(desktop);
    return false;
  }

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
//for modern NT (Vista/7/8/10), use CreateToolhelp32Snapshot which provides process names
//without requiring OpenProcess access; fall back to PSAPI for old NT 3.x/4.x
DWORD procList_NT(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter)
{
	//try Toolhelp32 first (available on all Windows 2000+ including Vista/7/8/10)
	HINSTANCE hKernel=::LoadLibrary("KERNEL32.dll");
	if(hKernel!=NULL)
	{
		typedef HANDLE (WINAPI *pfnCreateToolhelp32Snapshot_D)(DWORD,DWORD);
		typedef BOOL (WINAPI *pfnProcess32Next_D)(HANDLE,LPPROCESSENTRY32);
		typedef BOOL (WINAPI *pfnProcess32First_D)(HANDLE,LPPROCESSENTRY32);
		pfnCreateToolhelp32Snapshot_D pfnCreateToolhelp32Snapshot=(pfnCreateToolhelp32Snapshot_D)::GetProcAddress(hKernel,"CreateToolhelp32Snapshot");
		pfnProcess32First_D pfnProcess32First=(pfnProcess32First_D)::GetProcAddress(hKernel,"Process32First");
		pfnProcess32Next_D pfnProcess32Next=(pfnProcess32Next_D)::GetProcAddress(hKernel,"Process32Next");

		if(pfnCreateToolhelp32Snapshot!=NULL && pfnProcess32First!=NULL && pfnProcess32Next!=NULL)
		{
			HANDLE hSnapShot=(*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS,0);
			PROCESSENTRY32 processInfo;
			memset((void *)&processInfo,0,sizeof(PROCESSENTRY32));
			processInfo.dwSize=sizeof(PROCESSENTRY32);

			if((*pfnProcess32First)(hSnapShot, &processInfo))
			{
				const char *ptrFilename=NULL;
				int filternums=0;
				if(filter && filter[0]!=0) filternums=(strchr(filter,','))?2:1;
				do
				{
					if((ptrFilename=strrchr(processInfo.szExeFile,'\\'))==NULL)
						ptrFilename=processInfo.szExeFile;
					else
						ptrFilename+=1;

					bool bMatch=true;
					if(filternums==1)
						bMatch=MatchingString(ptrFilename,filter,false);
					else if(filternums>1)
						bMatch=ifMatch(ptrFilename,filter);
					if(bMatch)
					{
						std::pair<DWORD,std::string> p(processInfo.th32ProcessID,ptrFilename);
						vecList.push_back(p);
					}
				}while((*pfnProcess32Next)(hSnapShot,&processInfo));
			}
			CloseHandle(hSnapShot);
			::FreeLibrary(hKernel);
			return vecList.size();
		}
		::FreeLibrary(hKernel);
	}

	//fallback: use PSAPI for old NT 3.x/4.x where Toolhelp32 is unavailable
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

