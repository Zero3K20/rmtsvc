/*******************************************************************
   *	sntpclnt.h
   *    DESCRIPTION:SNTP simple network time synchronization protocol client declaration
   *
   *	http://hi.baidu.com/yycblog/home
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SNTP_CLINET_H__
#define __YY_SNTP_CLINET_H__

#include "sntpdef.h"
#include "socketUdp.h"

namespace net4cpp21
{

	class CSNTPClient
	{
	public:
		CSNTPClient();
		~CSNTPClient(){}
		DWORD GetTimeout() const { return m_dwTimeout; };
		void  SetTimeout(DWORD dwTimeout) { m_dwTimeout = dwTimeout; };
		const char *GetLastError() { return m_strLastError.c_str(); }
		//General functions
		bool  GetServerTime(const char * szHostName, NtpServerResponse& response,
					int nPort = SNTP_SERVER_PORT);
		//synchronize local clock based on returned result
		bool SynchroniseClock(NtpServerResponse& response);
		
	protected:
		bool EnableSetTimePriviledge();
		void RevertSetTimePriviledge();
		bool  SetClientTime(const CNtpTime& NewTime);

		DWORD            m_dwTimeout; //network response timeout
		HANDLE           m_hToken;
		TOKEN_PRIVILEGES m_TokenPriv;
		std::string		 m_strLastError;	//errorinfo
	};
};//namespace net4cpp21

#endif

