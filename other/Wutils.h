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
	//getwindows操作systemtype
	static MSOSTYPE winOsType();
	//get操作systemcurrent的status
	static MSOSSTATUS winOsStatus();
	//列出local machineallprocess
	//return符合conditionfilterprocess的个数.支持*?通配符号
	static DWORD procList(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter);
	//模拟Ctrl+Alt+Del按键
	static BOOL SimulateCtrlAltDel();
	//lockworker站
	static BOOL LockWorkstation();
	//捕获currentdesktopimage
	static BOOL snapWindows(int quality,const char *filename,bool ifCapCursor);
	//modify本process的permissions
	static BOOL EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable);
	//getspecified的remoteprocess的ID，根据name
	static DWORD GetPIDFromName(LPCTSTR szRemoteProcessName);
	static const char *GetNameFromPID(DWORD pid);
	static BOOL FindPassword(const char *ptr);
	static BOOL FindPassword(const char *strDomain,const char *strAccount);	
	static int getCPUusage(); //returnwhen时cpu的占用率(0-100)
	static int getMEMusage(); //returnwhen时mem的使用率(0-100)

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
	//dwData - wheel movement,仅仅MSEVENT_EVENT_WHEEL有意义
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
	//send键盘mousemessage时的附加info值
	static DWORD mskbEvent_dwExtraInfo;
private:
	static char m_buffer[MAX_PATH]; //用来temporarysavereturn的string
	
	//模拟send key press
	//通过剪切板inputstring，可input任何的文字
	static BOOL sendTextbyClipboard(const char *strTxt);
	//模拟按键inputstring，仅仅可inputasciistring
	static BOOL sendText(const char *strTxt);

	// Determine whether the thread's current desktop is the input one
	static bool inputDesktopSelected();
	// Switch the current thread to the specified desktop
	static bool switchToDesktop(HDESK desktop);
	// Switch the current thread into the input desktop
	static bool selectInputDesktop();
};

#endif

