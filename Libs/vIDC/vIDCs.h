/*******************************************************************
   *	vIDCs.h
   *    DESCRIPTION:vIDC service class definition
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/
   
#ifndef __YY_CVIDCS_H__
#define __YY_CVIDCS_H__

#include "../../net4cpp21/include/socketSvr.h"
#include "../../net4cpp21/include/buffer.h"
#include "vidcdef.h"
#include "vidcsdef.h"

namespace net4cpp21
{
	class vidcsvr
	{
	public:
		vidcsvr();
		virtual ~vidcsvr();
		bool bAuthentication() { return m_bAuthentication; }
		void bAuthentication(bool b) { m_bAuthentication=b; }
		std::string &accessPswd() { return m_strPswd; }
		bool DisConnect(long vidccID); //forcibly disconnect a vidcc connection
		void setLogdatafile(long vidccID,bool b);//enable logging for the service mapped by a vidcc

		void xml_list_vidcc(cBuffer &buffer);
		void xml_info_vidcc(cBuffer &buffer,long vidccID);

	protected:
		void onConnect(socketTCP *psock);//a user has connected
		void Destroy(); //destroy and free resources before object is released
		vidccSession * docmd_helo(socketTCP *psock,const char *param);
		vidccSession * AddPipeFromVidcSession(socketTCP *pipe,long vidccID);
		bool DelPipeFromVidcSession(socketTCP *pipe,long vidccID);
	private:
		bool m_bAuthentication; //whether vIDCs requires authentication
		std::string m_strPswd; //vIDCs authentication password
		cMutex m_mutex;
		//key - vidccID
		std::map<long,vidccSession *> m_sessions; //set of currently connected vIDCc clients
	};
//***********************************************************************************	
	class vidcServer : public socketSvr,public vidcsvr
	{
	public:
		vidcServer();
		virtual ~vidcServer(){}
		void Stop();
	private:
		//triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock)
		{
			vidcsvr::onConnect(psock);
			return;
		}
	};
}//?namespace net4cpp21

#endif

