/*******************************************************************
   *	webAction_fport.cpp web request processing - enumerate process and port associations
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    approach: enumerate associations by calling undocumented functions in Iphlpapi.dll
   *    Iphlpapi.dll public functions include GetTcpTable/GetUdpTable, but cannot associate with processes
   *	
   *******************************************************************/

#include "rmtsvc.h"
#include "other\wutils.h"
#include <Iprtrmib.h>

BOOL portList(cBuffer &buffer);
//buffer - returned xml document, format:
//<?xml version="1.0" encoding="gb2312" ?>
//<xmlroot>
//<fport>
//<id>sequence number</id>
//<pid>process ID</pid>
//<pname>process name</pname>
//<ptype>type</ptype>
//<laddr>localaddress</laddr>
//<raddr>remoteaddress</raddr>
//<status>status</status>
//</fport>
//...
//</xmlroot>
bool webServer::httprsp_fport(socketTCP *psock,httpResponse &httprsp)
{
	cBuffer buffer(2048);
	portList(buffer);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len()); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}


//----------------------private function--------------------------------
typedef struct _MIB_TCPROW_EX
{
DWORD dwState; // MIB_TCP_STATE_*
DWORD dwLocalAddr;
DWORD dwLocalPort;
DWORD dwRemoteAddr;
DWORD dwRemotePort;
DWORD dwProcessId;
} MIB_TCPROW_EX, *PMIB_TCPROW_EX;

typedef struct _MIB_TCPTABLE_EX
{
DWORD dwNumEntries;
MIB_TCPROW_EX table[ANY_SIZE];
} MIB_TCPTABLE_EX, *PMIB_TCPTABLE_EX;

typedef struct _MIB_UDPROW_EX
{
DWORD dwLocalAddr;
DWORD dwLocalPort;
DWORD dwProcessId;
} MIB_UDPROW_EX, *PMIB_UDPROW_EX;

typedef struct _MIB_UDPTABLE_EX
{
DWORD dwNumEntries;
MIB_UDPROW_EX table[ANY_SIZE];
} MIB_UDPTABLE_EX, *PMIB_UDPTABLE_EX;


/*
DWORD
WINAPI
AllocateAndGetTcpExTableFromStack(
OUT PMIB_TCPTABLE_EX *pTcpTableEx,
IN BOOL bOrder,
IN HANDLE hAllocHeap,
IN DWORD dwAllocFlags,
IN DWORD dwProtocolVersion; // 2 - TCP, 23 - TCPv6 (size of *pTcpTableEx must be 56!)
);
*/
typedef DWORD (WINAPI *PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)(PMIB_TCPTABLE_EX*,BOOL,HANDLE,DWORD,DWORD);

/*
DWORD
WINAPI
AllocateAndGetUdpExTableFromStack(
OUT PMIB_UDPTABLE_EX *pUdpTable,
IN BOOL bOrder,
IN HANDLE hAllocHeap,
IN DWORD dwAllocFlags,
IN DWORD dwProtocolVersion; // 2 - UDP, 23 - UDPv6 (size of *pUdpTable must be 28!)
);
*/
typedef DWORD (WINAPI *PROCALLOCATEANDGETUDPEXTABLEFROMSTACK)(PMIB_UDPTABLE_EX*,BOOL,HANDLE,DWORD,DWORD);

// GetExtendedTcpTable / GetExtendedUdpTable (documented, Windows XP SP2+)
// Used as fallback when the undocumented Allocate* functions are unavailable (Windows Vista+).
// With AF_INET (2) and TCP_TABLE_OWNER_PID_ALL (5) / UDP_TABLE_OWNER_PID (1) the returned
// table entries are binary-compatible with MIB_TCPROW_EX / MIB_UDPROW_EX above.
typedef DWORD (WINAPI *PROCGETEXTENDEDTCPTABLE)(PVOID,PDWORD,BOOL,ULONG,DWORD,ULONG);
typedef DWORD (WINAPI *PROCGETEXTENDEDUDPTABLE)(PVOID,PDWORD,BOOL,ULONG,DWORD,ULONG);

// Constants for GetExtendedTcpTable / GetExtendedUdpTable
#ifndef FPORT_AF_INET
#define FPORT_AF_INET               2   // AF_INET
#define FPORT_TCP_TABLE_OWNER_PID_ALL 5 // TCP_TABLE_CLASS::TCP_TABLE_OWNER_PID_ALL
#define FPORT_UDP_TABLE_OWNER_PID   1   // UDP_TABLE_CLASS::UDP_TABLE_OWNER_PID
#endif

static char *TCP_STATE[13]={"","CLOSED","LISTEN","SYN_SENT","SYN_RCVD","ESTAB","FIN_WAIT1",
			"FIN_WAIT2","CLOSE_WAIT","CLOSING","LAST_ACK","TIME_WAIT","DELETE_TCB"};

BOOL portList(cBuffer &buffer)
{

	HMODULE hModule=NULL;
	PROCALLOCATEANDGETTCPEXTABLEFROMSTACK lpfnAllocateAndGetTcpExTableFromStack = NULL;
	PROCALLOCATEANDGETUDPEXTABLEFROMSTACK lpfnAllocateAndGetUdpExTableFromStack = NULL;

	hModule=::LoadLibrary("iphlpapi.dll");
	if(hModule==NULL) return FALSE; //load DLL failed
	//getfunctionpointer // XP and later - actually tested on 2k as well
	lpfnAllocateAndGetTcpExTableFromStack = (PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetTcpExTableFromStack");
	lpfnAllocateAndGetUdpExTableFromStack = (PROCALLOCATEANDGETUDPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetUdpExTableFromStack");

	// Fall back to documented GetExtendedTcpTable/GetExtendedUdpTable (Windows XP SP2+)
	// when undocumented functions are unavailable (Windows Vista and later).
	BOOL bUseUndocumented = (lpfnAllocateAndGetTcpExTableFromStack != NULL && lpfnAllocateAndGetUdpExTableFromStack != NULL);
	PROCGETEXTENDEDTCPTABLE lpfnGetExtendedTcpTable = NULL;
	PROCGETEXTENDEDUDPTABLE lpfnGetExtendedUdpTable = NULL;
	if (!bUseUndocumented)
	{
		lpfnGetExtendedTcpTable = (PROCGETEXTENDEDTCPTABLE)GetProcAddress(hModule,"GetExtendedTcpTable");
		lpfnGetExtendedUdpTable = (PROCGETEXTENDEDUDPTABLE)GetProcAddress(hModule,"GetExtendedUdpTable");
		if (lpfnGetExtendedTcpTable == NULL || lpfnGetExtendedUdpTable == NULL)
		{
			::FreeLibrary(hModule);
			return FALSE;
		}
	}
	
	Wutils::EnablePrivilege(SE_DEBUG_NAME,true);
	if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
	 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"utf-8\" ?><xmlroot>");

	DWORD dwLastError=NO_ERROR,dwSize,dwState,dwCount=0;
	PMIB_TCPTABLE_EX lpBuffer = NULL;
	PMIB_UDPTABLE_EX lpBuffer1 = NULL;
	//enumallTCP
	if (bUseUndocumented)
	{
		dwLastError = lpfnAllocateAndGetTcpExTableFromStack(&lpBuffer,TRUE,GetProcessHeap(),0,2);
	}
	else
	{
		// GetExtendedTcpTable two-call pattern; result layout is binary-compatible with MIB_TCPTABLE_EX.
		DWORD dwTableSize = 0;
		lpfnGetExtendedTcpTable(NULL,&dwTableSize,FALSE,FPORT_AF_INET,FPORT_TCP_TABLE_OWNER_PID_ALL,0);
		if (dwTableSize > 0)
		{
			lpBuffer = (PMIB_TCPTABLE_EX)HeapAlloc(GetProcessHeap(),0,dwTableSize);
			if (lpBuffer)
				dwLastError = lpfnGetExtendedTcpTable(lpBuffer,&dwTableSize,TRUE,FPORT_AF_INET,FPORT_TCP_TABLE_OWNER_PID_ALL,0);
			else
				dwLastError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}
	if (dwLastError == NO_ERROR)
	{
		char strRemoteAddr[24];
//		printf("Local IP\tLocal Port\tRemote Ip\tRemote Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer->dwNumEntries; dwSize++)
		{
			if(lpBuffer->table[dwSize].dwProcessId==0) continue;
			if(buffer.Space()<512) buffer.Resize(buffer.size()+512);

			dwState=lpBuffer->table[dwSize].dwState;
			if(dwState>MIB_TCP_STATE_DELETE_TCB) dwState=0;
			sprintf(strRemoteAddr,"%s",socketBase::IP2A(lpBuffer->table[dwSize].dwRemoteAddr));
			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer->table[dwSize].dwProcessId);
			{
				const char *pname_src=szProcessName?szProcessName:"OS kernel";
				int pnlen=strlen(pname_src);
				size_t space_needed=pnlen*3+512;
				if(buffer.Space()<space_needed) buffer.Resize(buffer.size()+space_needed);
				char utf8pname[MAX_PATH*4];
				cCoder::utf8_encode(pname_src, pnlen, utf8pname);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),
					"<fport><id>%d</id>"
					"<ptype>TCP</ptype>"
					"<pid>%04d</pid>"
					"<pname>%s</pname>"
					"<laddr>%s:%d</laddr>"
					"<raddr>%s:%d</raddr>"
					"<status>%s</status>"
					"</fport>",++dwCount,
					lpBuffer->table[dwSize].dwProcessId,
					utf8pname,
					socketBase::IP2A(lpBuffer->table[dwSize].dwLocalAddr),
					ntohs((u_short)lpBuffer->table[dwSize].dwLocalPort),
					strRemoteAddr,
					ntohs((u_short)lpBuffer->table[dwSize].dwRemotePort),
					TCP_STATE[dwState] );
			}
		}//?for
	}//?if (dwLastError == NO_ERROR)
	
	//enumallUDP
	dwLastError = NO_ERROR;
	if (bUseUndocumented)
	{
		dwLastError = lpfnAllocateAndGetUdpExTableFromStack(&lpBuffer1,TRUE,GetProcessHeap(),0,2);
	}
	else
	{
		// GetExtendedUdpTable two-call pattern; result layout is binary-compatible with MIB_UDPTABLE_EX.
		DWORD dwTableSize = 0;
		lpfnGetExtendedUdpTable(NULL,&dwTableSize,FALSE,FPORT_AF_INET,FPORT_UDP_TABLE_OWNER_PID,0);
		if (dwTableSize > 0)
		{
			lpBuffer1 = (PMIB_UDPTABLE_EX)HeapAlloc(GetProcessHeap(),0,dwTableSize);
			if (lpBuffer1)
				dwLastError = lpfnGetExtendedUdpTable(lpBuffer1,&dwTableSize,TRUE,FPORT_AF_INET,FPORT_UDP_TABLE_OWNER_PID,0);
			else
				dwLastError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}
	if (dwLastError == NO_ERROR)
	{
//		printf("Local IP\tLocal Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer1->dwNumEntries; dwSize++)
		{
			if(lpBuffer1->table[dwSize].dwProcessId==0) continue; 
			if(buffer.Space()<512) buffer.Resize(buffer.size()+512);

			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer1->table[dwSize].dwProcessId);
			{
				const char *pname_src=szProcessName?szProcessName:"OS kernel";
				int pnlen=strlen(pname_src);
				size_t space_needed=pnlen*3+512;
				if(buffer.Space()<space_needed) buffer.Resize(buffer.size()+space_needed);
				char utf8pname[MAX_PATH*4];
				cCoder::utf8_encode(pname_src, pnlen, utf8pname);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),
					"<fport><id>%d</id>"
					"<ptype>UDP</ptype>"
					"<pid>%04d</pid>"
					"<pname>%s</pname>"
					"<laddr>%s:%d</laddr>"
					"<raddr>*.*</raddr>"
					"<status></status>"
					"</fport>",++dwCount,
					lpBuffer1->table[dwSize].dwProcessId,
					utf8pname,
					socketBase::IP2A(lpBuffer1->table[dwSize].dwLocalAddr),
					ntohs((u_short)lpBuffer1->table[dwSize].dwLocalPort) );
			}
		}//?for
	}//?if (dwLastError == NO_ERROR)

	if (lpBuffer) HeapFree(GetProcessHeap(),0,lpBuffer);
	if (lpBuffer1) HeapFree(GetProcessHeap(),0,lpBuffer1);
	::FreeLibrary(hModule);

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	return TRUE;
}

BOOL portList(string &strret)
{

	HMODULE hModule=NULL;
	PROCALLOCATEANDGETTCPEXTABLEFROMSTACK lpfnAllocateAndGetTcpExTableFromStack = NULL;
	PROCALLOCATEANDGETUDPEXTABLEFROMSTACK lpfnAllocateAndGetUdpExTableFromStack = NULL;

	hModule=::LoadLibrary("iphlpapi.dll");
	if(hModule==NULL) return FALSE; //load DLL failed
	//getfunctionpointer // XP and later - actually tested on 2k as well
	lpfnAllocateAndGetTcpExTableFromStack = (PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetTcpExTableFromStack");
	lpfnAllocateAndGetUdpExTableFromStack = (PROCALLOCATEANDGETUDPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetUdpExTableFromStack");

	// Fall back to documented GetExtendedTcpTable/GetExtendedUdpTable (Windows XP SP2+)
	// when undocumented functions are unavailable (Windows Vista and later).
	BOOL bUseUndocumented = (lpfnAllocateAndGetTcpExTableFromStack != NULL && lpfnAllocateAndGetUdpExTableFromStack != NULL);
	PROCGETEXTENDEDTCPTABLE lpfnGetExtendedTcpTable = NULL;
	PROCGETEXTENDEDUDPTABLE lpfnGetExtendedUdpTable = NULL;
	if (!bUseUndocumented)
	{
		lpfnGetExtendedTcpTable = (PROCGETEXTENDEDTCPTABLE)GetProcAddress(hModule,"GetExtendedTcpTable");
		lpfnGetExtendedUdpTable = (PROCGETEXTENDEDUDPTABLE)GetProcAddress(hModule,"GetExtendedUdpTable");
		if (lpfnGetExtendedTcpTable == NULL || lpfnGetExtendedUdpTable == NULL)
		{
			::FreeLibrary(hModule);
			return FALSE;
		}
	}
	
	Wutils::EnablePrivilege(SE_DEBUG_NAME,true);
	char stmp[512]; long len=0;
	len=sprintf(stmp,"id\ttype\tLocal\tRemote\tpname\r\n");
	strret.append(stmp,len);
	DWORD dwLastError=NO_ERROR,dwSize,dwState,dwCount=0;
	PMIB_TCPTABLE_EX lpBuffer = NULL;
	PMIB_UDPTABLE_EX lpBuffer1 = NULL;
	//enumallTCP
	if (bUseUndocumented)
	{
		dwLastError = lpfnAllocateAndGetTcpExTableFromStack(&lpBuffer,TRUE,GetProcessHeap(),0,2);
	}
	else
	{
		// GetExtendedTcpTable two-call pattern; result layout is binary-compatible with MIB_TCPTABLE_EX.
		DWORD dwTableSize = 0;
		lpfnGetExtendedTcpTable(NULL,&dwTableSize,FALSE,FPORT_AF_INET,FPORT_TCP_TABLE_OWNER_PID_ALL,0);
		if (dwTableSize > 0)
		{
			lpBuffer = (PMIB_TCPTABLE_EX)HeapAlloc(GetProcessHeap(),0,dwTableSize);
			if (lpBuffer)
				dwLastError = lpfnGetExtendedTcpTable(lpBuffer,&dwTableSize,TRUE,FPORT_AF_INET,FPORT_TCP_TABLE_OWNER_PID_ALL,0);
			else
				dwLastError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}
	if (dwLastError == NO_ERROR)
	{
		char strRemoteAddr[24];
//		printf("Local IP\tLocal Port\tRemote Ip\tRemote Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer->dwNumEntries; dwSize++)
		{
			if(lpBuffer->table[dwSize].dwProcessId==0) continue;

			dwState=lpBuffer->table[dwSize].dwState;
			if(dwState>MIB_TCP_STATE_DELETE_TCB) dwState=0;
			sprintf(strRemoteAddr,"%s",socketBase::IP2A(lpBuffer->table[dwSize].dwRemoteAddr));
			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer->table[dwSize].dwProcessId);
			len=sprintf(stmp,"%d\tTCP\t%s:%d\t%s:%d\t%s\t%s\r\n",++dwCount,
						socketBase::IP2A(lpBuffer->table[dwSize].dwLocalAddr),
					ntohs((u_short)lpBuffer->table[dwSize].dwLocalPort),
					strRemoteAddr,
					ntohs((u_short)lpBuffer->table[dwSize].dwRemotePort),
					TCP_STATE[dwState],
						((szProcessName)?szProcessName:"OS kernel") );
			strret.append(stmp,len);
		}//?for
	}//?if (dwLastError == NO_ERROR)
	
	//enumallUDP
	dwLastError = NO_ERROR;
	if (bUseUndocumented)
	{
		dwLastError = lpfnAllocateAndGetUdpExTableFromStack(&lpBuffer1,TRUE,GetProcessHeap(),0,2);
	}
	else
	{
		// GetExtendedUdpTable two-call pattern; result layout is binary-compatible with MIB_UDPTABLE_EX.
		DWORD dwTableSize = 0;
		lpfnGetExtendedUdpTable(NULL,&dwTableSize,FALSE,FPORT_AF_INET,FPORT_UDP_TABLE_OWNER_PID,0);
		if (dwTableSize > 0)
		{
			lpBuffer1 = (PMIB_UDPTABLE_EX)HeapAlloc(GetProcessHeap(),0,dwTableSize);
			if (lpBuffer1)
				dwLastError = lpfnGetExtendedUdpTable(lpBuffer1,&dwTableSize,TRUE,FPORT_AF_INET,FPORT_UDP_TABLE_OWNER_PID,0);
			else
				dwLastError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}
	if (dwLastError == NO_ERROR)
	{
//		printf("Local IP\tLocal Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer1->dwNumEntries; dwSize++)
		{
			if(lpBuffer1->table[dwSize].dwProcessId==0) continue; 

			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer1->table[dwSize].dwProcessId);
			len=sprintf(stmp,"%d\tUDP\t%s:%d\t*.*\t \t%s\r\n",++dwCount,
						socketBase::IP2A(lpBuffer1->table[dwSize].dwLocalAddr),
					ntohs((u_short)lpBuffer1->table[dwSize].dwLocalPort),
					((szProcessName)?szProcessName:"OS kernel") );
			strret.append(stmp,len);
		}//?for
	}//?if (dwLastError == NO_ERROR)

	if (lpBuffer) HeapFree(GetProcessHeap(),0,lpBuffer);
	if (lpBuffer1) HeapFree(GetProcessHeap(),0,lpBuffer1);
	::FreeLibrary(hModule);
	return TRUE;
}
