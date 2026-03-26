/*******************************************************************
 *	cInjectDll.cpp
 *    DESCRIPTION:Remote DLL injection, execute specified function
 *
 *    AUTHOR:yyc
 *
 *    http://hi.baidu.com/yycblog/home
 *
 *    DATE:2003-02-25
 *
 *******************************************************************/

#include <windows.h>
#include <tlhelp32.h> //enumerate all processes

#include "cInjectDll.h"

#ifdef UNICODE
LPCSTR LoadLibraryFuncStr = "LoadLibraryW";
LPCSTR GetModuleHandleFuncStr = "GetModuleHandleW";
LPCSTR CreateFileFuncStr="CreateFileW";
#else
LPCSTR LoadLibraryFuncStr = "LoadLibraryA";
LPCSTR GetModuleHandleFuncStr = "GetModuleHandleA";
LPCSTR CreateFileFuncStr="CreateFileA";
#endif
LPCSTR FreeLibraryFuncStr = "FreeLibrary";
LPCSTR GetProcAddressFuncStr = "GetProcAddress";
LPCSTR GetLastErrorFuncStr = "GetLastError";
LPCSTR ExitThreadFuncStr = "ExitThread";
LPCSTR VirtualFreeFuncStr = "VirtualFree";
LPCSTR CloseHandleFuncStr= "CloseHandle";
LPCSTR WriteFileFuncStr="WriteFile";

cInjectDll::cInjectDll(LPCTSTR szRemoteProcessName)
{
	if(szRemoteProcessName)
		 m_dwProcessId=GetPIDFromName(szRemoteProcessName);
	else m_dwProcessId=0;
	//initialize parameters
	memset((void *)&m_InjectLibInfo,0,sizeof(INJECTLIBINFO));
    m_InjectLibInfo.pfnLoadLibrary = (PLOADLIBRARY)GetProcAddress(GetModuleHandle
("Kernel32.dll"),LoadLibraryFuncStr);
	m_InjectLibInfo.pfnFreeLibrary = (PFREELIBRARY)GetProcAddress(GetModuleHandle
("Kernel32.dll"),FreeLibraryFuncStr);
	m_InjectLibInfo.pfnGetProcaddr = (PGETPROCADDRESS)GetProcAddress(GetModuleHandle
("Kernel32.dll"),GetProcAddressFuncStr);
    m_InjectLibInfo.pfnGetLastError = (PGETLASTERROR)GetProcAddress(GetModuleHandle
("Kernel32.dll"),GetLastErrorFuncStr);
	
	m_InjectLibInfo.pfnExitThread = (PEXITTHREAD)GetProcAddress(GetModuleHandle
("Kernel32.dll"),ExitThreadFuncStr);
    m_InjectLibInfo.pfnVirtualFree = (PVIRTUALFREE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),VirtualFreeFuncStr);
	
	m_InjectLibInfo.pfnCloseHandle = (PCLOSEHANDLE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),CloseHandleFuncStr);
    m_InjectLibInfo.pfnCreateFile = (PCREATEFILE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),CreateFileFuncStr);
	m_InjectLibInfo.pfnWriteFile = (PWRITEFILE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),WriteFileFuncStr);
    
	m_InjectLibInfo.szDllName[0]=0;
	m_InjectLibInfo.szFuncName[0]=0;
	m_InjectLibInfo.bFree =true;
	m_InjectLibInfo.hModule =NULL;
}
//set target exe for injection
DWORD cInjectDll::Inject(LPCTSTR szRemoteProcessName)
{
	if(szRemoteProcessName)
		 m_dwProcessId=GetPIDFromName(szRemoteProcessName);
	return m_dwProcessId;
}
//unload a dll from the specified target process
DWORD cInjectDll::DeattachDLL(DWORD pid,HMODULE hmdl)
{
	m_dwProcessId=pid;
	m_InjectLibInfo.hModule=hmdl;
	m_InjectLibInfo.bFree=true;
	return _run(" "," ",FALSE,NULL,0);
}
//modify current process privileges
bool  cInjectDll::EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if(!::OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES |
        TOKEN_QUERY | TOKEN_READ,&hToken))
        return false;
    if(!::LookupPrivilegeValue(NULL, lpszPrivilegeName, &luid))
        return false;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;

    ::AdjustTokenPrivileges(hToken,FALSE,&tp,NULL,NULL,NULL);

    ::CloseHandle(hToken);

    return (GetLastError() == ERROR_SUCCESS);
}

//-----------------------------------------------------
// get the real entry address of the target function.
// in debug builds, some function addresses are actually jump table addresses
// we use this jump table address to get the real entry address
//-----------------------------------------------------
PVOID cInjectDll::GetFuncAddress(PVOID addr)
{
#ifdef _DEBUG
	//check if instruction is relative jump (E9)
	if (0xE9 != *((UCHAR*)addr))
		return addr;
	// calculate base of relative jump
	ULONG base = (ULONG)((UCHAR*)addr + 5);
	// calculate offset 
	ULONG *offset = (ULONG*)((UCHAR*)addr + 1);
	return (PVOID)(base + *offset);
#else
	// in release, don't have to mess with jumps
	return addr;
#endif
}

//get the ID of the specified remote process by name
DWORD cInjectDll::GetPIDFromName(LPCTSTR szRemoteProcessName)
{
	if(szRemoteProcessName==NULL || szRemoteProcessName[0]==0)
		return 0;
	DWORD dwRet=0;

	//yyc add 2003-03-21 support NT ******************
	HINSTANCE hDll=::LoadLibrary("KERNEL32.dll");
	typedef HANDLE (WINAPI *pfnCreateToolhelp32Snapshot_D)(DWORD,DWORD);
	typedef BOOL (WINAPI *pfnProcess32Next_D)(HANDLE,LPPROCESSENTRY32);
	pfnCreateToolhelp32Snapshot_D pfnCreateToolhelp32Snapshot=
		(pfnCreateToolhelp32Snapshot_D)::GetProcAddress(hDll,"CreateToolhelp32Snapshot");
	pfnProcess32Next_D pfnProcess32Next=
		(pfnProcess32Next_D)::GetProcAddress(hDll,"Process32Next");
	if(pfnCreateToolhelp32Snapshot==NULL) //only Win2K supports CreateToolhelp32Snapshot and similar functions
	{
		dwRet=FindProcessID(szRemoteProcessName);
	} //yyc add 2003-03-21 end******************
	else 
	{
		HANDLE hSnapShot=(*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS,0);
		PROCESSENTRY32* processInfo=new PROCESSENTRY32;
		processInfo->dwSize=sizeof(PROCESSENTRY32);
		while((*pfnProcess32Next)(hSnapShot,processInfo)!=FALSE)
		{
				if(_stricmp(szRemoteProcessName,processInfo->szExeFile)==0)
				{
					dwRet=processInfo->th32ProcessID;
					break;
				}
		}
		CloseHandle(hSnapShot);
		delete processInfo;
	}//?if(pfnCreateToolhelp32Snapshot==NULL)

	//yyc add 2003-03-21 support NT ******************
	::FreeLibrary(hDll);
	//yyc add 2003-03-21 end *************************	
	return dwRet;
}
//remote execution, synchronous call mode
DWORD WINAPI cInjectDll::remoteThreadProc(INJECTLIBINFO *pInfo)
{
	HINSTANCE hDll=NULL;
	pInfo->dwReturnValue = 0;
	if(pInfo->hModule!=NULL)
		hDll=pInfo->hModule;
	else
		hDll = (HINSTANCE)pInfo->pfnLoadLibrary(pInfo->szDllName);
	if(hDll!=NULL)
	{
		PCALLBACKFUNC pfn=(PCALLBACKFUNC)pInfo->pfnGetProcaddr(hDll,pInfo->szFuncName);
		if(pfn!=NULL)
			pInfo->dwReturnValue=(*pfn)(&pInfo->dwParam);
		else
			pInfo->dwReturnValue=(DWORD)(-2);
		if(pInfo->bFree)
		{ pInfo->pfnFreeLibrary(hDll);hDll=NULL;}
		pInfo->hModule =hDll;
	}
	else
		pInfo->dwReturnValue = pInfo->pfnGetLastError();
	return 0;
}
//execute remote thread asynchronously
//bFree setting is invalid in async mode; the loaded DLL is always freed after remote thread completes
//INJECTLIBINFO::hModule always returns NULL; setting hModule has no effect
//remote execution, asynchronous call mode
DWORD WINAPI cInjectDll::remoteThreadProc_syn(INJECTLIBINFO *pInfo)
{
	pInfo->dwReturnValue = 0;
	HINSTANCE hDll = (HINSTANCE)pInfo->pfnLoadLibrary(pInfo->szDllName);
	if(hDll!=NULL)
	{
		PCALLBACKFUNC pfn=(PCALLBACKFUNC)pInfo->pfnGetProcaddr(hDll,pInfo->szFuncName);
		if(pfn!=NULL)
			pInfo->dwReturnValue=(*pfn)(&pInfo->dwParam);
		else
			pInfo->dwReturnValue=(DWORD)(-2);
		pInfo->pfnFreeLibrary(hDll);
	}
	//----------------------------------------------------
	// if async call mode is chosen, the remote thread must free itself
	//----------------------------------------------------
	PEXITTHREAD fnExitThread = pInfo->pfnExitThread;
	PVIRTUALFREE fnVirtualFree = pInfo->pfnVirtualFree;
	//----------------------------------------------------
	// after freeing itself, call ExitThread to end the thread.
	// the following assembly code is equivalent to:
	// VirtualFree( pInfo, 0, MEM_RELEASE );
	// ExitThread( 0 );
	//----------------------------------------------------
	__asm {
		push 0;				// parameter of ExitThread
		push 0;				// return address of ExitThread
		push MEM_RELEASE;	// parameter of VirtualFree
		push 0;				// parameter of VirtualFree
		push pInfo;			// parameter of VirtualFree
		push fnExitThread;	// return address of VirtualFree
		push fnVirtualFree;
		ret;				// call VirtualFree
		}

	return 0;
}

//BOOL ifSyn --- whether to execute remote thread asynchronously
//if szDllName==NULL, then szFunctionName is a PREMOTEFUNC function pointer
//in async mode, does not wait for remote thread to finish and will not get its return value
//return value: 0 -- remote function executed successfully, otherwise an error occurred, returns error code
DWORD cInjectDll::_run(LPCTSTR szDllName,LPCTSTR szFunctionName,BOOL ifSyn,PVOID param,DWORD dwParamSize)
{
	if(m_dwProcessId==0 || szFunctionName==NULL) return (DWORD)(-1);
	DWORD dwRet=0;

    //elevate current process privileges and open the target process
    //the current user must have debug privileges
    cInjectDll::EnablePrivilege(SE_DEBUG_NAME,true);
    HANDLE hRemoteProcess = ::OpenProcess(PROCESS_ALL_ACCESS,false,m_dwProcessId);
	if(hRemoteProcess == NULL)
    {
        //RW_LOG_DEBUG("[INJECT] Failed to Open Process. Err =%d\n",GetLastError());
        return (DWORD)(-1);
    }
	//else RW_LOG_DEBUG("[INJECT] success to Open Process.\n");
	if(szDllName){
		lstrcpyn(m_InjectLibInfo.szDllName,szDllName,strlen(szDllName)+1);
		lstrcpyn(m_InjectLibInfo.szFuncName,szFunctionName,strlen(szFunctionName)+1);
	}
	else
	{
		m_InjectLibInfo.szDllName[0]=0;
		m_InjectLibInfo.szFuncName[0]=0;
	}
	
	DWORD cbParamSize=0;
	if(param!=NULL)
		cbParamSize=(dwParamSize==0)?(strlen((const char *)param)+1):dwParamSize;

	DWORD cbDataBuffer=sizeof(INJECTLIBINFO)+cbParamSize;
	DWORD cbSize=cbDataBuffer+MAXINJECTCODESIZE;
	LPBYTE p, c;//p is the start address, c is the code start address

	//-----------------------------------------------------------
	// allocate memory in the target process for writing startup code and necessary parameters
	//-----------------------------------------------------------
	if((p = (LPBYTE)::VirtualAllocEx(hRemoteProcess,NULL,cbSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE))!=NULL)
	{
		c = p+cbDataBuffer;
		//write parameter data
		if(::WriteProcessMemory(hRemoteProcess,p,(LPVOID)&m_InjectLibInfo,sizeof(INJECTLIBINFO)/*cbDataBuffer*/,0)!=0)
		{
			if(cbParamSize>0)
			{
				//write parameter data passed to the called function
				LPBYTE paramOffset=p+((PBYTE)&m_InjectLibInfo.dwParam-(PBYTE)&m_InjectLibInfo);
				::WriteProcessMemory(hRemoteProcess,paramOffset,param,cbParamSize,0);
			}
			//write code
			LPVOID remoteThreadAddr;
			if(szDllName)
			{
				if(!ifSyn)
					remoteThreadAddr=cInjectDll::GetFuncAddress(remoteThreadProc);
				else
					remoteThreadAddr=cInjectDll::GetFuncAddress(remoteThreadProc_syn);
			}
			else 
				remoteThreadAddr=cInjectDll::GetFuncAddress((PVOID)szFunctionName);
			
			if(::WriteProcessMemory(hRemoteProcess,c,remoteThreadAddr,cbSize-cbDataBuffer,0)!=0)
			{
				HANDLE hRemoteThread;
				if((hRemoteThread = ::CreateRemoteThread(hRemoteProcess,0,0,(PCALLBACKFUNC)c,(INJECTLIBINFO*)p,0,0))!=NULL)
				{
					//RW_LOG_DEBUG("[INJECT] success to CreateRemoteThread.\n");
					if(!ifSyn) //execute remote thread asynchronously: do nothing, dwRet=0
					{//execute remote thread synchronously
						//wait for remote thread to finish
						::WaitForSingleObject(hRemoteThread,INFINITE);
						//RW_LOG_DEBUG("[INJECT] RemoteThread ended!.\n");
						//read function return value
						DWORD dwReaded,dwReadSize=((PBYTE)&m_InjectLibInfo.hModule-(PBYTE)&m_InjectLibInfo);
						LPBYTE pOfset=p+dwReadSize; dwReadSize=sizeof(INJECTLIBINFO)-dwReadSize;
						dwRet=::ReadProcessMemory(hRemoteProcess,pOfset,(LPVOID)&m_InjectLibInfo.hModule,dwReadSize,&dwReaded);
//						RW_LOG_DEBUG("[INJECT] dwRet=%d,dwReadSize=%d,dwReaded=%d.Err=%d\n",dwRet,dwReadSize,dwReaded,GetLastError());
						if(cbParamSize>0)
						{
							LPBYTE paramOffset=p+((PBYTE)&m_InjectLibInfo.dwParam-(PBYTE)&m_InjectLibInfo);
							dwRet=::ReadProcessMemory(hRemoteProcess,paramOffset,param,cbParamSize,&dwReaded);
//							RW_LOG_DEBUG("[INJECT] dwRet=%d,cbParamSize=%d,dwReaded=%d.Err=%d\n",dwRet,cbParamSize,dwReaded,GetLastError());
						} 
						dwRet=m_InjectLibInfo.dwReturnValue;
//						RW_LOG_DEBUG("5555555555555555 dwret=%d\r\n",dwRet);
					}//?if(ifSyn)
					::CloseHandle( hRemoteThread );
				}
				else
				{
					//RW_LOG_DEBUG("[INJECT] Failed to CreateRemoteThread.Err =%d\n",GetLastError());
					dwRet=(DWORD)(-6);
				}
			}//?if(::WriteProcessMemory(hRemoteProcess,c,cInjectDll...
			else
			{
				//RW_LOG_DEBUG("[INJECT] Failed to Write Code to Remote Process.Err =%d\n",GetLastError());
				dwRet=(DWORD)(-5);
			}
		}//?if(::WriteProcessMemory(hRemoteProcess,p,(LPVOID)...
		else
		{
			//RW_LOG_DEBUG("[INJECT] Failed to Write Param to Remote Process.Err =%d\n",GetLastError());
			dwRet=(DWORD)(-4);
		}
		//free the allocated memory
		if(!ifSyn || dwRet!=0) //non-async mode or error occurred during async execution
		::VirtualFreeEx( hRemoteProcess, p, 0, MEM_RELEASE );
	}//?if((p = (PBYTE)::VirtualAllocEx(hRemoteProcess
	else
	{
        //RW_LOG_DEBUG("[INJECT] Failed to Allocate Memory at Remote Process for Param.Err=%d\n",GetLastError()); 
		dwRet=(DWORD)(-3);
	}
	
	if( hRemoteProcess != NULL )
		::CloseHandle(hRemoteProcess);
	//restore privileges
    EnablePrivilege(SE_DEBUG_NAME,false);
	return dwRet;
}

//only for FindProcessID declaration
// Undocumented typedef's
typedef struct _QUERY_SYSTEM_INFORMATION
{
	DWORD GrantedAccess;
	DWORD PID;
	WORD HandleType;
	WORD HandleId;
	DWORD Handle;
} QUERY_SYSTEM_INFORMATION, *PQUERY_SYSTEM_INFORMATION;
typedef struct _PROCESS_INFO_HEADER
{
	DWORD Count;
	DWORD Unk04;
	DWORD Unk08;
} PROCESS_INFO_HEADER, *PPROCESS_INFO_HEADER;
typedef struct _PROCESS_INFO
{
	DWORD LoadAddress;
	DWORD Size;
	DWORD Unk08;
	DWORD Enumerator;
	DWORD Unk10;
	char Name [0x108];
} PROCESS_INFO, *PPROCESS_INFO;
typedef DWORD (__stdcall *PFNNTQUERYSYSTEMINFORMATION)  (DWORD, PVOID, DWORD, PDWORD);
typedef PVOID (__stdcall *PFNRTLCREATEQUERYDEBUGBUFFER) (DWORD, DWORD);
typedef DWORD (__stdcall *PFNRTLQUERYPROCESSDEBUGINFORMATION) (DWORD, DWORD, PVOID);
typedef void (__stdcall *PFNRTLDESTROYQUERYDEBUGBUFFER) (PVOID);
// Note that the following code eliminates the need
// for PSAPI.DLL as part of the executable.
DWORD cInjectDll::FindProcessID (LPCTSTR szRemoteProcessName)
{
	#define INITIAL_ALLOCATION 0x100
	//function pointer definition
	PFNNTQUERYSYSTEMINFORMATION pfnNtQuerySystemInformation;
	PFNRTLCREATEQUERYDEBUGBUFFER pfnRtlCreateQueryDebugBuffer;
	PFNRTLQUERYPROCESSDEBUGINFORMATION pfnRtlQueryProcessDebugInformation;
	PFNRTLDESTROYQUERYDEBUGBUFFER pfnRtlDestroyQueryDebugBuffer;
	//get function pointer
	HINSTANCE hNtDll = ::LoadLibrary("NTDLL.DLL");
	if(hNtDll==NULL) return 0;
	pfnNtQuerySystemInformation =
		(PFNNTQUERYSYSTEMINFORMATION) GetProcAddress 
			(hNtDll, 
			"NtQuerySystemInformation");
	pfnRtlCreateQueryDebugBuffer =
		(PFNRTLCREATEQUERYDEBUGBUFFER) GetProcAddress 
			(hNtDll, 
			"RtlCreateQueryDebugBuffer");
	pfnRtlQueryProcessDebugInformation =
		(PFNRTLQUERYPROCESSDEBUGINFORMATION) GetProcAddress 
			(hNtDll, 
			"RtlQueryProcessDebugInformation");
	pfnRtlDestroyQueryDebugBuffer =
		(PFNRTLDESTROYQUERYDEBUGBUFFER) GetProcAddress 
			(hNtDll, 
			"RtlDestroyQueryDebugBuffer");
	//-----------------------------------------------------
	DWORD i,NumHandles,rc = 0;
	DWORD SizeNeeded = 0;
	PQUERY_SYSTEM_INFORMATION QuerySystemInformationP;
	PVOID InfoP = HeapAlloc (GetProcessHeap (),HEAP_ZERO_MEMORY,INITIAL_ALLOCATION);


	// Find how much memory is required.
	pfnNtQuerySystemInformation (0x10, InfoP, INITIAL_ALLOCATION, &SizeNeeded);
	HeapFree (GetProcessHeap (),0,InfoP);
	// Now, allocate the proper amount of memory.
	InfoP = HeapAlloc (GetProcessHeap (),HEAP_ZERO_MEMORY,SizeNeeded);
	DWORD SizeWritten = SizeNeeded;
	if(pfnNtQuerySystemInformation (0x10, InfoP, SizeNeeded, &SizeWritten))
		goto EXIT1;
	if((NumHandles = SizeWritten / sizeof (QUERY_SYSTEM_INFORMATION))==0) 
		goto EXIT1;

	QuerySystemInformationP =(PQUERY_SYSTEM_INFORMATION) InfoP;
	for (i = 1; i <= NumHandles; i++)
	{
		// "5" is the value of a kernel object type process.
		if (QuerySystemInformationP->HandleType == 5)
		{
			PVOID DebugBufferP =pfnRtlCreateQueryDebugBuffer (0, 0);
			if (pfnRtlQueryProcessDebugInformation (QuerySystemInformationP->PID,1,DebugBufferP) == 0)
			{
				PPROCESS_INFO_HEADER ProcessInfoHeaderP =(PPROCESS_INFO_HEADER) ((DWORD) DebugBufferP + 0x60);
				DWORD Count =ProcessInfoHeaderP->Count;
				PPROCESS_INFO ProcessInfoP =(PPROCESS_INFO) ((DWORD) ProcessInfoHeaderP + sizeof (PROCESS_INFO_HEADER));
				//if (strstr (_strupr (ProcessInfoP->Name), "WINLOGON") != 0)
				if (strstr (_strlwr (ProcessInfoP->Name), szRemoteProcessName) != 0)//yyc modify 2003-03-21
				{
					rc=QuerySystemInformationP->PID;
					if (DebugBufferP) pfnRtlDestroyQueryDebugBuffer(DebugBufferP);
					break;
					/*
					//unknown purpose???
					DWORD i;
					DWORD dw = (DWORD) ProcessInfoP;
					for (i = 0; i < Count; i++)
					{
						dw += sizeof (PROCESS_INFO);
						ProcessInfoP = (PPROCESS_INFO) dw;
						if (strstr (_strupr (ProcessInfoP->Name), "NWGINA") != 0)
							goto EXIT1;//return (0);
						if (strstr (_strupr (ProcessInfoP->Name), "MSGINA") == 0)
							rc = QuerySystemInformationP->PID;
					}
					if (DebugBufferP) pfnRtlDestroyQueryDebugBuffer(DebugBufferP);
					goto EXIT1;
					*/
				}//?if(strstr (
			}
			if(DebugBufferP) pfnRtlDestroyQueryDebugBuffer(DebugBufferP);
		}//?if (pfnRtlQueryProcessDebug...
		DWORD dw = (DWORD) QuerySystemInformationP;
		dw += sizeof (QUERY_SYSTEM_INFORMATION);
		QuerySystemInformationP = (PQUERY_SYSTEM_INFORMATION) dw;
	}//?for
		
EXIT1:
	HeapFree (GetProcessHeap (),0,InfoP);
	//--------------------------------------------------
	if(hNtDll!=NULL) FreeLibrary(hNtDll);
	return (rc);
} // FindWinLogon

