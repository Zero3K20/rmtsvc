   /*******************************************************************
   *	cInjectDll.h
   *    DESCRIPTION:Remote DLL injection, execute specified function
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2003-02-25
   *
   *******************************************************************/

  
#ifndef __CINJECTDLL_H__
#define __CINJECTDLL_H__

typedef HINSTANCE (WINAPI *PLOADLIBRARY)(LPCTSTR );
typedef BOOL (WINAPI *PFREELIBRARY)(HINSTANCE);
typedef HMODULE (WINAPI* PGETMODULEHANDLE)(LPCTSTR );
typedef PVOID (WINAPI* PGETPROCADDRESS)(HINSTANCE,LPCSTR);
typedef DWORD (WINAPI* PGETLASTERROR)(VOID);
typedef VOID (WINAPI* PEXITTHREAD)(DWORD);
typedef BOOL (WINAPI* PVIRTUALFREE)(LPVOID,SIZE_T,DWORD);
typedef BOOL (WINAPI *PCLOSEHANDLE)(HANDLE);
typedef HANDLE (WINAPI *PCREATEFILE)(LPCTSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
typedef BOOL (WINAPI *PWRITEFILE)(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);

#define MAXINJECTCODESIZE 1024 //maximum bytes of injection code
//type declaration for remotely executing a function in a dll
typedef DWORD (WINAPI *PCALLBACKFUNC)(PVOID);

typedef struct _INJECTLIBINFO
{
	PLOADLIBRARY pfnLoadLibrary;
	PFREELIBRARY pfnFreeLibrary;
	PGETPROCADDRESS pfnGetProcaddr;
	PGETLASTERROR pfnGetLastError;
	PEXITTHREAD pfnExitThread;
	PVIRTUALFREE pfnVirtualFree;
	PCLOSEHANDLE pfnCloseHandle;
	PCREATEFILE pfnCreateFile;
	PWRITEFILE pfnWriteFile;

	TCHAR szDllName[256];
	TCHAR szFuncName[256];
	bool bFree;//in sync mode, whether to unload the dll loaded by remote thread after it finishes
			   //in async mode, bFree setting is invalid, the loaded DLL will always be released after remote thread finishes
	HINSTANCE hModule;//[in|out] - if hModule is specified, szDllName is ignored and LoadLibrary is not called to load the dll
					  //otherwise load the dll specified by szDllName into the target process; in sync mode, returns the module handle of the loaded DLL
	DWORD dwReturnValue;
	DWORD dwParam;//parameter passed to the called process, i.e., user parameter
} INJECTLIBINFO;
//remote thread execution function type declaration
typedef DWORD (WINAPI *PREMOTEFUNC)(INJECTLIBINFO *pInfo);

class cInjectDll
{
private:
	INJECTLIBINFO m_InjectLibInfo;
	DWORD m_dwProcessId;//remote process ID

	static DWORD WINAPI remoteThreadProc(INJECTLIBINFO *pInfo);
	//remote execution, asynchronous call mode
	static DWORD WINAPI remoteThreadProc_syn(INJECTLIBINFO *pInfo);

	//return value: 0 -- remote function executed successfully, otherwise an error occurred, returns error code
	//if szDllName==NULL, then szFunctionName is a PREMOTEFUNC function pointer
	//BOOL ifSyn --- whether to execute remote thread asynchronously
	DWORD _run(LPCTSTR szDllName,LPCTSTR szFunctionName,BOOL ifSyn,PVOID param=NULL,DWORD dwParamSize=0);
public:
	//modify current process privileges
	static bool EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable);
	//-----------------------------------------------------
	// get the real entry address of the target function.
	// in debug builds, some function addresses are actually jump table addresses
	// we use this jump table address to get the real entry address
	//-----------------------------------------------------
	static PVOID GetFuncAddress(PVOID addr);
	//get the ID of the specified remote process by name
	static DWORD GetPIDFromName(LPCTSTR szRemoteProcessName);
	
private:
	static DWORD FindProcessID (LPCTSTR szRemoteProcessName);
public:
	cInjectDll(LPCTSTR szRemoteProcessName);
	~cInjectDll(){}
	DWORD Inject(LPCTSTR szRemoteProcessName); //set the target exe for injection
	//execute remote thread synchronously
	//can set m_InjectLibInfo.bFree to decide whether to unload the DLL after remote thread completes; default is unload
	//can read m_InjectLibInfo.hModule to get the module handle of the dll loaded by the remote thread
	//return value: 0 -- remote function executed successfully, otherwise an error occurred, returns error code
	DWORD run(LPCTSTR szDllName,LPCTSTR szFunctionName,PVOID param=NULL,DWORD dwParamSize=0)
	{
		if(szDllName==NULL||szFunctionName==NULL) return (DWORD)(-1);
		return _run(szDllName,szFunctionName,FALSE,param,dwParamSize);
	}
	//execute remote thread asynchronously
	//in async mode, bFree setting is invalid, the loaded DLL will always be released after remote thread finishes
	//return value: 0 -- remote function executed successfully, otherwise an error occurred, returns error code
	DWORD run_syn(LPCTSTR szDllName,LPCTSTR szFunctionName,PVOID param=NULL,DWORD dwParamSize=0)
	{
		if(szDllName==NULL||szFunctionName==NULL) return (DWORD)(-1);
		return _run(szDllName,szFunctionName,TRUE,param,dwParamSize);
	}
	//remotely execute a function in this exe
	//the function prototype is of PREMOTEFUNC type; any system APIs in the function must be called explicitly
	DWORD Call(DWORD pid,PREMOTEFUNC pfunc,PVOID param,DWORD paramLen)
	{
		if(pid!=0) m_dwProcessId=pid;
		return _run(NULL,(LPCTSTR)pfunc,FALSE,param,paramLen);
	}	
	//unload a dll from the specified target process
	DWORD DeattachDLL(DWORD pid,HMODULE hmdl);

//-------------------2005-01-25 monitor self for abnormal exit begin-------------
//-------------------2005-01-25 monitor self for abnormal exit  end -------------
};


#endif

