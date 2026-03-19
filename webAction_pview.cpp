/*******************************************************************
   *	webAction_pview.cpp web request processing - process browser
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/
#include "rmtsvc.h"
#include "other\wutils.h"
#include "other\stringMatch.h"
#include <tlhelp32.h> //enumerate all processes

DWORD procList_NT(cBuffer &buffer,const char *filter);
DWORD procList_2K(cBuffer &buffer,const char *filter);
DWORD moduleList_NT(cBuffer &buffer,DWORD processID);
DWORD moduleList_2K(cBuffer &buffer,DWORD processID);
//list all processes on local machine
//return count of processes matching filter, separated by comma. Filter supports *? wildcards
//buffer - returned xml document, format:
//<?xml version="1.0" encoding="gb2312" ?>
//<xmlroot>
//<process>
//<id>sequence number</id>
//<pid>process ID</pid>
//<ppid>parent process ID</ppid>
//<pname>process name</pname>
//<priority>priority</priority>
//<threads>thread count</threads>
//<ppath>program path</ppath>
//</process>
//...
//</xmlroot>
DWORD procList(cBuffer &buffer,const char *filter)
{
	MSOSTYPE ost=Wutils::winOsType();
	if(ost==MSOS_TYPE_NT)
		return procList_NT(buffer,filter);
	return procList_2K(buffer,filter);
}
//list all modules of a specific process
//buffer - returned xml document, format:
//<?xml version="1.0" encoding="utf-8" ?>
//<xmlroot>
//<module>
//<id>sequence number</id>
//<hmdl></hmdl>
//<mbase>base address</mbase>
//<mname>module name</mname>
//</module>
//...
//</xmlroot>
DWORD moduleList(cBuffer &buffer,DWORD pid)
{
	MSOSTYPE ost=Wutils::winOsType();
	if(ost==MSOS_TYPE_NT)
		return moduleList_NT(buffer,pid);
	return moduleList_2K(buffer,pid);
}


bool webServer::httprsp_plist(socketTCP *psock,httpResponse &httprsp)
{
	cBuffer buffer(1024);
	procList(buffer,NULL);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len()); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

bool webServer::httprsp_mlist(socketTCP *psock,httpResponse &httprsp,DWORD pid)
{
	cBuffer buffer(1024);
	moduleList(buffer,pid);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len()); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

bool webServer::httprsp_pkill(socketTCP *psock,httpResponse &httprsp,DWORD pid)
{
	char buf[64];
	int buflen=sprintf(buf,"true");
	HANDLE hProcess=::OpenProcess(PROCESS_ALL_ACCESS,TRUE,pid);
	if(hProcess)
	{
		::TerminateProcess(hProcess,0);
		::CloseHandle(hProcess);
	}else buflen=sprintf(buf,"Can not open processID %d",pid);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//set response content length
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

#include "cInjectDll.h"
//unload a specific DLL from a certain process
bool webServer::httprsp_mdattach(socketTCP *psock,httpResponse &httprsp,DWORD pid,HMODULE hmdl,long count)
{
	cInjectDll inject(NULL);
	if(count<=0 || count>100) count=1;
	for(int i=0;i<count;i++) inject.DeattachDLL(pid,hmdl);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//set response content length
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK");
	return true;
}
//****************************************************************************
//**********************private function for this file************************
//****************************** start ***************************************
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
DWORD procList_NT(cBuffer &buffer,const char *filter)
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

			DWORD dwret=0;
			if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"utf-8\" ?><xmlroot>");
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
						if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
						buffer.len()+=sprintf(buffer.str()+buffer.len(),
							"<process>"
							"<id>%d</id>"
							"<pid>%04d</pid>"
							"<ppid>%04d</ppid>"
							"<pname>%s</pname>"
							"<priority>%02d</priority>"
							"<threads>%02d</threads>"
							"<ppath></ppath>"
							"</process>",++dwret,processInfo.th32ProcessID,
							processInfo.th32ParentProcessID,
							ptrFilename,processInfo.pcPriClassBase,
							processInfo.cntThreads);
					}
				}while((*pfnProcess32Next)(hSnapShot,&processInfo));
			}
			if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");

			CloseHandle(hSnapShot);
			::FreeLibrary(hKernel);
			return dwret;
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

	DWORD dwret=0;
	if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"utf-8\" ?><xmlroot>");

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
				int pnlen=strlen(szProcessName);
				long space_needed=pnlen*3+256;
				if(buffer.Space()<space_needed) buffer.Resize(buffer.size()+space_needed);
				char utf8pname[MAX_PATH*4];
				cCoder::utf8_encode(szProcessName, pnlen, utf8pname);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),
					"<process>"
					"<id>%d</id>"
					"<pid>%04d</pid>"
					"<ppid>%04d</ppid>"
					"<pname>%s</pname>"
					"<priority>%02d</priority>"
					"<threads>%02d</threads>"
					"<ppath></ppath>"
					"</process>",++dwret,aProcesses[i],
					0,utf8pname,0,0);
			}
		}//?for(...
	}//?if (pfnEnumProcesses!=NULL &&
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");

	::FreeLibrary(hDll);
	return dwret;
}

//enumerate win9x/2k system processes
//for win9x/2k, use toolhelp32 functions to enumerate processes and module info
//only 2k&&win9x support CreateToolhelp32Snapshot and similar functions
DWORD procList_2K(cBuffer &buffer,const char *filter)
{
	HINSTANCE hDll=::LoadLibrary("KERNEL32.dll");
	if(hDll==NULL) return 0;
	typedef HANDLE (WINAPI *pfnCreateToolhelp32Snapshot_D)(DWORD,DWORD);
	typedef BOOL (WINAPI *pfnProcess32Next_D)(HANDLE,LPPROCESSENTRY32);
	typedef BOOL (WINAPI *pfnProcess32First_D)(HANDLE,LPPROCESSENTRY32);
	pfnProcess32First_D pfnProcess32First=(pfnProcess32First_D)::GetProcAddress(hDll,"Process32First");
	pfnCreateToolhelp32Snapshot_D pfnCreateToolhelp32Snapshot=(pfnCreateToolhelp32Snapshot_D)::GetProcAddress(hDll,"CreateToolhelp32Snapshot");
	pfnProcess32Next_D pfnProcess32Next=(pfnProcess32Next_D)::GetProcAddress(hDll,"Process32Next");
	
	HANDLE hSnapShot=(*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS,0);
	 PROCESSENTRY32* processInfo=new PROCESSENTRY32;
	 memset((void *)processInfo,0,sizeof(PROCESSENTRY32));
	 processInfo->dwSize=sizeof(PROCESSENTRY32);
	
	 DWORD dwret=0;
	 if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
	 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"utf-8\" ?><xmlroot>");
	 if ((*pfnProcess32First)(hSnapShot, processInfo))
	 {
		const char *ptrFilename=NULL;
		int filternums=0;//filter condition count
		if(filter && filter[0]!=0 ) filternums=(strchr(filter,','))?2:1;//2 means multiple
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
				int pnlen=strlen(ptrFilename);
				long space_needed=pnlen*3+256;
				if(buffer.Space()<space_needed) buffer.Resize(buffer.size()+space_needed);
				char utf8pname[MAX_PATH*4];
				cCoder::utf8_encode(ptrFilename, pnlen, utf8pname);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),
					"<process>"
					"<id>%d</id>"
					"<pid>%04d</pid>"
					"<ppid>%04d</ppid>"
					"<pname>%s</pname>"
					"<priority>%02d</priority>"
					"<threads>%02d</threads>"
					"<ppath></ppath>"
					"</process>",++dwret,processInfo->th32ProcessID,
					processInfo->th32ParentProcessID,
					utf8pname,processInfo->pcPriClassBase,
					processInfo->cntThreads);
			}
		}while ((*pfnProcess32Next)(hSnapShot,processInfo));
	}//?if ((*pfnProcess32First)(hSnapShot, processInfo))
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");

	CloseHandle(hSnapShot);
	delete processInfo;
	::FreeLibrary(hDll);
	return dwret;
}

//enumerate NT system process modules
//for NT operating system, use PSAPI.DLL to enumerate processes and module info
DWORD moduleList_NT(cBuffer &buffer,DWORD processID)
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
	if(hDll==NULL) return 0;

	pfnEnumProcessModules_D pfnEnumProcessModules=(pfnEnumProcessModules_D)::GetProcAddress(hDll,"EnumProcessModules");
	pfnGetModuleBaseName_D pfnGetModuleBaseName=(pfnGetModuleBaseName_D)::GetProcAddress(hDll,"GetModuleBaseNameA");
	pfnGetModuleFileNameEx_D pfnGetModuleFileNameEx=(pfnGetModuleFileNameEx_D)::GetProcAddress(hDll,"GetModuleFileNameExA");

	HMODULE aModules[1024];DWORD cbNeeded, cModules;
	HANDLE hProcess = ::OpenProcess( PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE, processID);
	
	DWORD dwret=0;
	if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<xmlroot>");

	if (hProcess!=NULL && (*pfnEnumProcessModules)(hProcess, aModules,sizeof(aModules), &cbNeeded ) )
	{
		cModules = cbNeeded / sizeof(DWORD);
		char szProcessName[MAX_PATH];
		for (unsigned int i = 0; i < cModules; i++ )
		{
			if(pfnGetModuleFileNameEx)
				(*pfnGetModuleFileNameEx)( hProcess, aModules[i], szProcessName,sizeof(szProcessName) );
			else
				strcpy(szProcessName,"unknown path");

			int mnlen=strlen(szProcessName);
			long space_needed=mnlen*3+256;
			if(buffer.Space()<space_needed) buffer.Resize(buffer.size()+space_needed);
			char utf8mname[MAX_PATH*4];
			cCoder::utf8_encode(szProcessName, mnlen, utf8mname);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),
				"<module>"
				"<id>%d</id>"
				"<hmdl>%ld</hmdl>"
				"<usage>%d</usage>"
				"<mbase>0x%08lX</mbase>"
				"<mname>%s</mname>"
				"</module>",
				++dwret,(LONG_PTR)aModules[i],-1,0,utf8mname);
		}
		::CloseHandle(hProcess);
	}//?if (pfnEnumProcesses!=NULL &&
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");

	::FreeLibrary(hDll);
	return dwret;
}

//enumerate win9x/2k system process modules
//for win9x/2k, use toolhelp32 functions to enumerate processes and module info
//only 2k&&win9x support CreateToolhelp32Snapshot and similar functions
DWORD moduleList_2K(cBuffer &buffer,DWORD processID)
{
	HINSTANCE hDll=::LoadLibrary("KERNEL32.dll");
	if(hDll==NULL) return 0;
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
	
	 DWORD dwret=0;
	 if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
	 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<xmlroot>");
	 if ((*pfnModule32First)(hSnapShot, moduleInfo))
	 {
		do
		{
			int mnlen=strlen(moduleInfo->szExePath);
			long space_needed=mnlen*3+256;
			if(buffer.Space()<space_needed) buffer.Resize(buffer.size()+space_needed);
			char utf8mname[MAX_PATH*4];
			cCoder::utf8_encode(moduleInfo->szExePath, mnlen, utf8mname);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),
				"<module>"
				"<id>%d</id>"
				"<hmdl>%ld</hmdl>"
				"<usage>%d</usage>"
				"<mbase>0x%08lX</mbase>"
				"<mname>%s</mname>"
				"</module>",
				++dwret,(LONG_PTR)moduleInfo->hModule,
				moduleInfo->ProccntUsage,
				(DWORD)(DWORD_PTR)moduleInfo->modBaseAddr,
				utf8mname);
		}while ((*pfnModule32Next)(hSnapShot,moduleInfo));
	}//?if ((*pfnProcess32First)(hSnapShot, processInfo))
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");

	CloseHandle(hSnapShot);
	delete moduleInfo;
	::FreeLibrary(hDll);
	return dwret;
}

//******************************* end ****************************************
//**********************private function for this file************************
//****************************************************************************
