
#ifndef NTService_h
#define NTService_h

class CNTService 
{
private:	// forbidden functions
	CNTService( const CNTService & );
	CNTService & operator=( const CNTService & );
public:
	CNTService(LPCTSTR ServiceName, LPCTSTR DisplayName = 0);
	virtual ~CNTService();
	BOOL	RegisterService(int argc, char ** argv);
	LPCTSTR GetServiceName() { return m_lpServiceName; }

protected:
	virtual void	Run(DWORD argc, LPTSTR * argv) = 0;
	virtual void	Stop() = 0;
	virtual void	Pause(){}
	virtual void	Continue(){}
	virtual void	Shutdown(){}

	BOOL	DebugService(int argc, char **argv);
	LPTSTR	GetLastErrorText(LPTSTR Buf, DWORD Size);
	void	AddToMessageLog(LPTSTR	Message,
							WORD	EventType = EVENTLOG_ERROR_TYPE,
							DWORD	dwEventID = DWORD(-1) );
	BOOL	ReportStatus(DWORD CurState,DWORD WaitHint=3000,DWORD ErrExit = 0);

private:
	static BOOL WINAPI	ControlHandler(DWORD CtrlType);

protected:	// data members
	LPCTSTR					m_lpServiceName;
	LPCTSTR					m_lpDisplayName;
	LPCTSTR					m_lpServiceDesc;//service description
	BOOL					m_bDebug;			// TRUE when running as a normal (non-service) process
	HANDLE					m_hRunCompleted;	// signaled when Run() returns
};
// Retrieve the one and only CNTService object:
CNTService * AfxGetService();

#endif	// NTService_h

