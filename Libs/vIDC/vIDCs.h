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
		void onConnect(socketTCP *psock);//有一个用户connect上来
		void Destroy(); //对象释放前，销毁释放资源动作
		vidccSession * docmd_helo(socketTCP *psock,const char *param);
		vidccSession * AddPipeFromVidcSession(socketTCP *pipe,long vidccID);
		bool DelPipeFromVidcSession(socketTCP *pipe,long vidccID);
	private:
		bool m_bAuthentication; //vIDCs是否需要authentication
		std::string m_strPswd; //vIDCs的authenticationpassword
		cMutex m_mutex;
		//key - vidccID
		std::map<long,vidccSession *> m_sessions; //current已connect的vIDCcclient集合
	};
//***********************************************************************************	
	class vidcServer : public socketSvr,public vidcsvr
	{
	public:
		vidcServer();
		virtual ~vidcServer(){}
		void Stop();
	private:
		//当有一个新的客户connect此服务触发此函数
		virtual void onAccept(socketTCP *psock)
		{
			vidcsvr::onConnect(psock);
			return;
		}
	};
}//?namespace net4cpp21

#endif

