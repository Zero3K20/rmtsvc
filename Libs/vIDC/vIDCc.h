/*******************************************************************
   *	vIDCc.h
   *    DESCRIPTION:vIDC client class definition
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/
   
#ifndef __YY_CVIDCC_H__
#define __YY_CVIDCC_H__

#include "../../net4cpp21/include/proxyclnt.h"
#include "../../net4cpp21/include/buffer.h"
#include "vidcdef.h"
#include "vidccdef.h"

namespace net4cpp21
{
	//vidcc client class
	class vidcClient : public socketProxy
	{
	public:
		explicit vidcClient(const char *strname,const char *strdesc);
		virtual ~vidcClient();
		VIDCSINFO &vidcsinfo() { return m_vidcsinfo; }
		void Destroy(); //destroy and free resources
		//connect to the specified vIDCs service
		SOCKSRESULT ConnectSvr();
		void DisConnSvr(); //disconnect from vIDCs
		bool mapinfoDel(const char *mapname);
		mapInfo * mapinfoGet(const char *mapname,bool bCreate);
		
		//returns SOCKSERR_OK on success
		int Mapped(const char *mapname,mapInfo *pinfo); //map the specified service
		int Unmap(const char *mapname,mapInfo *pinfo); //cancelmap the specified service

		void xml_list_mapped(cBuffer &buffer,VIDC_MAPTYPE maptype);
		void str_list_mapped(const char *vname,std::string &strini);
	private:
		bool sendCommand(int response_expected,const char *buf,int buflen);
		void parseCommand(const char *ptrCommand);
		static void onPipeThread(vidcClient *pvidcc);
		static void onCommandThread(vidcClient *pvidcc);
	private:
		std::map<std::string,mapInfo *> m_mapsets; //mapset
		time_t m_lTimeout;//maximum wait timeout return in seconds
		std::string m_strName; //this vidcc's name
		std::string m_strDesc;
		VIDCSINFO m_vidcsinfo;
		cThreadPool m_threadpool;//servicethread pool

		char m_szLastResponse[VIDC_MAX_COMMAND_SIZE]; //saves the most recent command return from vIDCs
	};
	
	class vidccSets
	{
	public:
		vidccSets();
		~vidccSets();
		void Destroy();
		void autoConnect();
		vidcClient *GetVidcClient(const char *vname,bool bCreate);
		bool DelVidcClient(const char *vname);
		
		bool xml_info_vidcc(cBuffer &buffer,const char *vname,VIDC_MAPTYPE maptype);
		void xml_list_vidcc(cBuffer &buffer);
		void str_list_vidcc(std::string &strini);
	private:
		cMutex m_mutex;
		//each vidcClient corresponds to a connection to one vIDCs
		std::map<std::string,vidcClient *> m_vidccs;
		std::string m_strName; //vidcc's name
		std::string m_strDesc;
	};

}//?namespace net4cpp21

#endif

