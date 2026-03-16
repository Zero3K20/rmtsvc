/*******************************************************************
   *	Wutils.h
   *    DESCRIPTION:windowssystemutility functions collection
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-09-19
   *
   *******************************************************************/
#ifndef __YY_WUTILS_H__
#define __YY_WUTILS_H__
#pragma warning(disable:4786)
#include <windows.h>
#include <vector>
#include <string>
using namespace std;

typedef enum //microsoft OS type
{
	MSOS_TYPE_UNKNOWED,
	MSOS_TYPE_31,
	MSOS_TYPE_9X,
	MSOS_TYPE_NT,
	MSOS_TYPE_2K,
	MSOS_TYPE_XP 
}MSOSTYPE;
typedef enum //microsoft OS status
{
	MSOS_STA_NORMAL,
	MSOS_STA_SCREENSAVER,
	MSOS_STA_LOCKED,
	MSOS_STA_UNLOGIN 
}MSOSSTATUS;

class Wutils
{
public:
	static const char *getLastInfo() { return m_buffer; }
	static const char *computeName();
	//return the number of CPUs
	static int cpuInfo(MSOSTYPE ostype);
	//get windows operating system type
	static MSOSTYPE winOsType();
	//get current operating system status
	static MSOSSTATUS winOsStatus();
	//list all processes on local machine
	//return count of processes matching filter condition. Supports * ? wildcards
	static DWORD procList(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter);
	//simulate Ctrl+Alt+Del key press
	static BOOL SimulateCtrlAltDel();
	//lock workstation
	static BOOL LockWorkstation();
	//capture current desktop image
	static BOOL snapWindows(int quality,const char *filename,bool ifCapCursor);
	//modify permissions of this process
	static BOOL EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable);
	//get ID of specified remote process by name
	static DWORD GetPIDFromName(LPCTSTR szRemoteProcessName);
	static const char *GetNameFromPID(DWORD pid);
	static BOOL FindPassword(const char *ptr);
	static BOOL FindPassword(const char *strDomain,const char *strAccount);	
	static int getCPUusage(); //return current CPU usage (0-100)
	static int getMEMusage(); //return current memory usage rate (0-100)

	static BOOL ShutDown()
	{
		Wutils::EnablePrivilege(SE_SHUTDOWN_NAME,true);
		::ExitWindowsEx(EWX_POWEROFF|EWX_FORCE,0);
		return TRUE;
	}
	static BOOL Restart()
	{
		Wutils::EnablePrivilege(SE_SHUTDOWN_NAME,true);
		::ExitWindowsEx(EWX_REBOOT|EWX_FORCE,0);
		return TRUE;
	}
	static BOOL Logoff()
	{
		::ExitWindowsEx(EWX_LOGOFF|EWX_FORCE,0);
		return TRUE;
	}
	//dwData - wheel movement, only meaningful for MSEVENT_EVENT_WHEEL
	static BOOL sendMouseEvent(int x,int y,short flags,DWORD dwData=0);
	static BOOL sendKeyEvent(short vkey);
	static BOOL sendKeys(const char *str)
	{
		const char *ptr=str; if(ptr==NULL) return FALSE;
		bool ifAsciiString=true;
		while(*ptr) if(*(unsigned char *)ptr>127){ifAsciiString=false; break;} else ptr++;
		return (ifAsciiString)?
			Wutils::sendText(str) :
			Wutils::sendTextbyClipboard(str);
	}
	
	static void selectDesktop(){
		if(!Wutils::inputDesktopSelected()) Wutils::selectInputDesktop();
	}
	//extra info value when sending keyboard/mouse messages
	static DWORD mskbEvent_dwExtraInfo;
private:
	static char m_buffer[MAX_PATH]; //temporary buffer for return string
	
	//simulate key press
	//input string via clipboard, can input any text
	static BOOL sendTextbyClipboard(const char *strTxt);
	//simulate key press to input string, can only input ASCII strings
	static BOOL sendText(const char *strTxt);

	// Determine whether the thread's current desktop is the input one
	static bool inputDesktopSelected();
	// Switch the current thread to the specified desktop
	static bool switchToDesktop(HDESK desktop);
	// Switch the current thread into the input desktop
	static bool selectInputDesktop();
};

#endif

