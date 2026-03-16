/*******************************************************************
   *	execCommand.cpp 
   *    DESCRIPTION:handle exec and kill commands
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-30
   *	
   *******************************************************************/

#include <windows.h>
#include <string>
#include "cInjectDll.h"

inline bool isDigest(const char *str)
{
	if(str==NULL) return false;
	while(*str!=0){if(*str>='0' && *str<='9') str++; else return false; }
	return true;
}
//kill specified process
//command format: 
//	kill <pid/process_name>,<pid/process_name>,<pid/process_name>...
BOOL docmd_kill(const char *processName)
{
	if(processName==NULL || processName[0]==0)
		return FALSE;
	std::string tmpStr(processName); BOOL bRet=FALSE;
	char *pstr=(char *)(tmpStr.c_str());
	char *token = strtok(pstr, ",");
	while( token != NULL )
	{
		long pID_l=0;
		if(isDigest(token)) pID_l=atol(token);
		DWORD pID=(pID_l>0)?((DWORD)pID_l):
				  (cInjectDll::GetPIDFromName(token));
		if(pID==0 && strcmp(token,"ME")==0) pID=GetCurrentProcessId();
		if(pID)
		{
			HANDLE hProcess=::OpenProcess(PROCESS_ALL_ACCESS,TRUE,pID);
			if(hProcess)
			{
				::TerminateProcess(hProcess,0);
				::CloseHandle(hProcess);
				bRet=TRUE;
			}
		}//?if(token[0]!=0)
		/* Get next token: */
		token = strtok( NULL, ",");
	}//?while(...
	return bRet;
}

extern BOOL StartInteractiveClientProcess (
    LPTSTR lpszUsername,    // client to log on
    LPTSTR lpszDomain,      // domain of client's account
    LPTSTR lpszPassword,    // client's password
    LPTSTR lpCommandLine,    // command line to execute
	bool ifHide,
	long lwaittimeout
) ;
//start specified process
//command format: 
//	exec [-hide] [-check <programname>] [-user <[Domain\]account:password>] [-wait <dwMilliseconds>] <full path program>
//-hide  - whether to run the specified program in the background
//-check <programname> - before running the specified program, check if <programname> is already running; if so, do not run it
//-user <[Domain\]account:password> - start program with the specified account; only valid when program runs as a service.
//			Domain can be omitted; if omitted, defaults to local account
//-wait <dwMilliseconds> - after successfully starting the process, whether to wait: ==0 no wait, <0 wait until process ends, >0 wait specified milliseconds
//-shell //run in shell mode
//<full path program>  - specify the program to execute
BOOL docmd_exec(const char *strParam)
{
	if(strParam==NULL || strParam[0]==0) return FALSE;
	bool ifHide=false;//whether to run hidden in the background
	bool ifShell=false; //yyc add 2011-01-05
	std::string strcheck="";//whether to check if program is already running; if so, run only once
	std::string strAccount="",strPwd="",strDomain;
	long lwaittimeout=0;//whether to wait for process to end
	char *ptr=(char *)strParam;
	while(ptr[0]=='-')
	{
		if(strncmp(ptr,"-hide ",6)==0)
		{	ifHide=true; ptr+=6; }
		else if(strncmp(ptr,"-shell ",7)==0)
		{	ifShell=true; ptr+=7; }
		else if(strncmp(ptr,"-check ",7)==0)
		{
			char *pos=(*(ptr+7)=='-')?NULL:strchr(ptr+7,' ');
			if(pos){
				*pos=0; strcheck.assign(ptr+7); *pos=' ';
				ptr=pos+1;
			}
			else ptr+=7;
		}
		else if(strncmp(ptr,"-wait ",6)==0)
		{//whether to wait for process to end
			char *pos=(*(ptr+6)=='-')?NULL:strchr(ptr+6,' ');
			if(pos){
				*pos=0; lwaittimeout=atol(ptr+6); *pos=' ';
				ptr=pos+1;
			}
			else ptr+=6;
		}//?else if(strncmp(ptr,"-wait ",6)==0)
		else if(strncmp(ptr,"-user ",6)==0)
		{//run with specified user, format: -user domain\account:password
			char *pos=(*(ptr+6)=='-')?NULL:strchr(ptr+6,' ');
			if(pos){//account specified
				*pos=0; std::string strUser;
				strUser.assign(ptr+6); *pos=' ';
				ptr=pos+1;
				//parse out domain, account, and password
				const char *ptrBegin=strUser.c_str();
				const char *pTmp=strchr(ptrBegin,'\\');
				if(pTmp){
					*(char *)pTmp=0;
					strDomain.assign(ptrBegin);
					*(char *)pTmp='\\';
					ptrBegin=pTmp+1;
				}
				if( (pTmp=strchr(ptrBegin,':')) )
				{//account and password specified
					*(char *)pTmp=0;
					strAccount.assign(ptrBegin);
					*(char *)pTmp=':';
					strPwd.assign(pTmp+1);
				}
				else //only account specified, password is ""
					strAccount.assign(ptrBegin);
			}//?if(pos){//account specified
			else ptr+=6;
		}//?else if(strncmp(ptr,"-user ",6)==0)
		else ptr++;
	}//?while
	if(strcheck!=""){
		//check if program is already running; if so, do not execute again
		DWORD pID=cInjectDll::GetPIDFromName(strcheck.c_str());
		if(pID) return TRUE;
	}
	
	if(ifShell){ //open specified file in Shell mode; specifying account is not supported
		unsigned long iret=(unsigned long)::ShellExecute(NULL,"open",
			ptr,NULL,NULL,((ifHide)?SW_HIDE:SW_SHOW) );
		return (iret>32)?TRUE:FALSE;
	}//?if(ifShell) //yyc add 2011-01-05

	if(strAccount!=""){ //run process with specified account; LocalSystem privilege is required
		if(strDomain=="") strDomain.assign(".");
		BOOL b=StartInteractiveClientProcess((char *)strAccount.c_str(),(char *)strDomain.c_str(),
			(char *)strPwd.c_str(),ptr,ifHide,lwaittimeout);
		if( b )
		{
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - success to execute %s as User(%s)!\r\n",
//				ptr,strAccount.c_str());
			return TRUE;
		}
//		else
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - failed to execute %s as user(%s\\%s)!\r\n",
//			ptr,strDomain.c_str(),strAccount.c_str());
	}//?if(strAccount!=""){ //run process with specified account

	STARTUPINFO si; PROCESS_INFORMATION pi;
	memset((void *)&pi,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow =(ifHide)?SW_HIDE:SW_NORMAL;
	if(::CreateProcess(NULL,ptr,NULL,NULL,FALSE,0,NULL, NULL,&si,&pi))
	{
		if(lwaittimeout!=0 && pi.hProcess != INVALID_HANDLE_VALUE)
		{
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - success to execute %s,waiting %d!\r\n",ptr,lwaittimeout);
			if(lwaittimeout<0)
				WaitForSingleObject(pi.hProcess, INFINITE);
			else
				WaitForSingleObject(pi.hProcess, lwaittimeout);
		}//?if(lwaittimeout!=0)
//		else
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - success to execute %s!\r\n",ptr);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return TRUE;
	}
//	RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - failed to execute %s!\r\n",ptr);
	return FALSE;
}

//execute specified console program or DOS command and capture output
//[in|out] strBuffer - [in] pointer to the console program or DOS command to execute
//					 - [out] returns the execution output
//iTimeout -- command execution timeout in seconds, -1 for unlimited
BOOL docmd_exec2buf(std::string &strBuffer,bool ifHide,int iTimeout)
{
	STARTUPINFO si; PROCESS_INFORMATION pi;
	memset((void *)&pi,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow =(ifHide)?SW_HIDE:SW_NORMAL;
	
	char buffer[4096]; long buflen=0; BOOL bret=TRUE;
	HANDLE hChildStdoutRd=NULL,hChildStdoutWr=NULL; 
	//standard output can be set to point to a specific pipe here---------------------------
	SECURITY_ATTRIBUTES saAttr;
	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = NULL;
	try{// Create a pipe for the child's STDOUT.
		if(!::CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
			throw "[CMD] Failed to create stdout pipe.\r\n";
		si.hStdOutput = hChildStdoutWr;
		si.hStdError = hChildStdoutWr;
		char *lpCommandLine=(char *)strBuffer.c_str();
//		printf("%s\r\n",lpCommandLine);
		if(!::CreateProcess(NULL,lpCommandLine ,NULL,NULL,TRUE,0,NULL, NULL,&si,&pi))
			throw "[CMD] Failed to CreateProcess.\r\n";
		DWORD dwret=(iTimeout<0)?INFINITE:iTimeout*1000;
		dwret=WaitForSingleObject(pi.hProcess, dwret);	
		if(dwret!=WAIT_OBJECT_0)
		{
			::TerminateProcess(pi.hProcess,0);
			throw "[CMD] - The time-out interval elapsed executing command.\r\n";
		}else{ //read data from the pipe
			unsigned long dw=0;
			if( ::WriteFile(hChildStdoutWr, " ",1, &dw, NULL) )
			{
				if (! ::ReadFile(hChildStdoutRd, buffer, 4095, &dw, NULL) || dw==0)
					throw "[CMD] - Failed to ReadFile\r\n";
				else buflen=dw;
			}//?if( ::WriteFile(...
			else throw "[CMD] Failed to WriteFile.\r\n";
		} //if(dwret!=WAIT_OBJECT_0)...end
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}catch(const char *message)
	{
		buflen=sprintf(buffer,"%s",message);
		bret=FALSE;
	}
	if(buflen>0) strBuffer.assign(buffer,buflen);
	else strBuffer="";
//	printf("aaaastrBuffer:%s\r\n",strBuffer.c_str());
	if(hChildStdoutWr) ::CloseHandle(hChildStdoutWr);
	if(hChildStdoutRd) ::CloseHandle(hChildStdoutRd);
	return bret;
}
/* yyc remove 2009-03-26 found when testing Fetion robot: the original docmd_exec2buf crashes when called frequently in succession
BOOL docmd_exec2buf(std::string &strBuffer,bool ifHide,int iTimeout)
{
	STARTUPINFO si; PROCESS_INFORMATION pi;
	memset((void *)&pi,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow =(ifHide)?SW_HIDE:SW_NORMAL;
	
	HANDLE hChildStdoutRd=NULL,hChildStdoutWr=NULL;
	//save standard output handle
	HANDLE hSaveStdout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hSaveStderr = ::GetStdHandle(STD_ERROR_HANDLE);
//standard output can be set to point to a specific pipe here---------------------------
	SECURITY_ATTRIBUTES saAttr;
	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = NULL;
	// Create a pipe for the child's STDOUT.
	if (! ::CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
	{
		strBuffer.assign("[CMD] Failed to create stdout pipe.\r\n");
		return FALSE;
	}
	// Set a write handle to the pipe to be STDOUT.
	::SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr);
	::SetStdHandle(STD_ERROR_HANDLE, hChildStdoutWr);
	si.hStdOutput = hChildStdoutWr;
	si.hStdError = hChildStdoutWr;
//standard output can be set to point to a specific pipe here---------------------------
	char buffer[4096]; 
	if(strBuffer.length()>4095) strBuffer[4095]=0; //protection
	sprintf(buffer,"%s",strBuffer.c_str());

//	printf("%s\r\n",buffer);

	BOOL bret=::CreateProcess(NULL,buffer ,NULL,NULL,TRUE,0,NULL, NULL,&si,&pi);
	if(bret)
	{
		DWORD dwret=(iTimeout<0)?INFINITE:iTimeout*1000;
		dwret=WaitForSingleObject(pi.hProcess, dwret);
		if(dwret!=WAIT_OBJECT_0)
		{
			bret=::TerminateProcess(pi.hProcess,0);
			strBuffer.assign("[CMD] - The time-out interval elapsed executing command.\r\n");
			char ss[32]; sprintf(ss,"kill it return %s\r\n",(bret)?"true":"false");
			strBuffer.append(ss);
			bret=FALSE;
		}
		else //?if(dwret!=WAIT_OBJECT_0)...else
		{//read data from the pipe
			unsigned long dw=0;
			if( ::WriteFile(hChildStdoutWr, " ",1, &dw, NULL) )
			{
				if (! ::ReadFile(hChildStdoutRd, buffer, 4095, &dw, NULL) || dw==0)
				{
					strBuffer.assign("[CMD] - Failed to ReadFile\r\n"); 
					bret=FALSE;
				}else{
					buffer[dw]=0;
					strBuffer.assign(buffer);
				}
			}//?if( ::WriteFile(...
			else{
				strBuffer=""; bret=FALSE;
			}
		} //if(dwret!=WAIT_OBJECT_0)...end
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}
	else
	{
		strBuffer.assign("[CMD] - failed to execute command!\r\n");
	}
//	if(hSaveStdout) 
	::SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	::SetStdHandle(STD_OUTPUT_HANDLE, hSaveStderr);
	if(hChildStdoutWr) ::CloseHandle(hChildStdoutWr);
	if(hChildStdoutRd) ::CloseHandle(hChildStdoutRd);

	return bret;
}
*/