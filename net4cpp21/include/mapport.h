/*******************************************************************
   *	mapport.h
   *    DESCRIPTION:port mapping service
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#ifndef __YY_MAPPORT_H__
#define __YY_MAPPORT_H__

#include "socketSvr.h"

namespace net4cpp21
{
	
	class mapServer : public socketSvr
	{
	public:
		mapServer();
		virtual ~mapServer();
		//set the application service to map
		bool mapped(const char *appsvr,int appport,int apptype);

	protected:
		//connect to the mapped application service
		socketTCP * connect_mapped(std::pair<std::string,int>* &p);

		//当有一个新的客户connect此服务触发此函数
		virtual void onAccept(socketTCP *psock);
		//收到forward data，用于data分析handle
		virtual void onData(char *buf,long len,socketTCP *from,socketTCP *to)
		{ return; }
	private:
		static void transThread(socketTCP *psock);
		//被mapped application service
		std::vector<std::pair<std::string,int> > m_mappedApp;
	};
}//?namespace net4cpp21

#endif
