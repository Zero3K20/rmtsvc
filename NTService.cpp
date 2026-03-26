
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h> //_ASSERTE

#include "NTService.h"

static CNTService * gpTheService = 0;			// the one and only instance
CNTService * AfxGetService() { return gpTheService; }

/////////////////////////////////////////////////////////////////////////////
// class CNTService -- construction/destruction
CNTService :: CNTService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName )
	: m_lpServiceName(lpServiceName)
	, m_lpDisplayName(lpDisplayName ? lpDisplayName : lpServiceName)
	, m_lpServiceDesc(0)
	, m_bDebug(FALSE)
	, m_hRunCompleted(NULL)
{
	_ASSERTE( gpTheService==0);
	gpTheService = this;
}


CNTService :: ~CNTService() {
	_ASSERTE( gpTheService==0);
	gpTheService = 0;
	if( m_hRunCompleted ) { ::CloseHandle(m_hRunCompleted); m_hRunCompleted = NULL; }
}

/////////////////////////////////////////////////////////////////////////////

BOOL CNTService :: RegisterService( int argc, char ** argv ) 
{	
	return DebugService(argc, argv);
}

BOOL CNTService :: DebugService(int argc, char ** argv) 
{
    DWORD dwArgc; LPTSTR *lpszArgv;
#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif
	
	m_bDebug = TRUE;
	SetConsoleCtrlHandler(ControlHandler, TRUE);

	m_hRunCompleted = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    Run(dwArgc, lpszArgv);
	if( m_hRunCompleted ) ::SetEvent(m_hRunCompleted);
	
#ifdef UNICODE
	::GlobalFree(HGLOBAL)lpszArgv);
#endif
	return TRUE;
}

BOOL CNTService :: ReportStatus(DWORD dwCurrentState,DWORD dwWaitHint,DWORD dwErrExit ) 
{
	return TRUE; // not running as an NT service
}

void CNTService :: AddToMessageLog(LPTSTR lpszMsg, WORD wEventType, DWORD dwEventID) 
{
}


LPTSTR CNTService :: GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize ) 
{
    LPTSTR lpszTemp = 0;
    DWORD dwRet =	::FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
					0,GetLastError(),
					LANG_NEUTRAL,
					(LPTSTR)&lpszTemp,
					0,0 );
    if( dwRet==0 || dwSize < (dwRet+2) )
        lpszBuf[0] = TEXT('\0');
    else _tcscpy(lpszBuf, lpszTemp);

    if( lpszTemp ) LocalFree(HLOCAL(lpszTemp));
    return lpszBuf;
}

BOOL WINAPI CNTService :: ControlHandler(DWORD dwCtrlType) {
	_ASSERTE(gpTheService != 0);
	switch( dwCtrlType ) {
		case CTRL_CLOSE_EVENT:  // user clicked X on the console window
		case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
		case CTRL_C_EVENT:
			gpTheService->Stop();
			if( gpTheService->m_hRunCompleted )
				::WaitForSingleObject(gpTheService->m_hRunCompleted, 10000);
			return TRUE;
	}
	return FALSE;
}
