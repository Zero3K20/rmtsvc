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

		//triggered when a new client connects to this service
		virtual void onAccept(socketTCP *psock);
		//received forwarded data, used for data analysis handling
		virtual void onData(char *buf,long len,socketTCP *from,socketTCP *to)
		{ return; }
	private:
		static void transThread(socketTCP *psock);
		//mapped application service
		std::vector<std::pair<std::string,int> > m_mappedApp;
	};
}//?namespace net4cpp21

#endif
