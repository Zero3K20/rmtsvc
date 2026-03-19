/*******************************************************************
   *	vidcManager.h
   *    DESCRIPTION:vIDC collection management class
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/

#ifndef __YY_VIDC_MANAGERDEF_H__
#define __YY_VIDC_MANAGERDEF_H__

#include "../../../net4cpp21/include/proxysvr.h"
#include "../mportsvr.h"
#include "../vidcs.h"
#include "../vidcc.h"

#ifdef _DEBUG
#pragma comment( lib, "libs/bin/vidc_d" )
#else
#pragma comment( lib, "libs/bin/vidc" )
#endif

using namespace std;
using namespace net4cpp21;

extern std::string g_strMyCert;
extern std::string g_strMyKey;
extern std::string g_strKeyPswd;
extern std::string g_strCaCert;
extern std::string g_strCaCRL;

//structure for saving extra parameter info for local port mapping
typedef struct _TMapParam
{
	bool bAutorun;
	long ipaccess;
	std::string ipRules;//IP access rules 
	//-ssl service client authentication certificate
	std::string clicert;
	std::string clikey;
	std::string clikeypswd;
}TMapParam,*PMapParam;

class vidcServerEx : public vidcServer
{
public:
	vidcServerEx(){
		m_svrport=VIDC_SERVER_PORT;
		m_autorun=false;
		m_ipaccess=1; m_ipRules="";
	}
	virtual ~vidcServerEx(){}
	SOCKSRESULT Start()
	{
		//set vIDCs SSL certificate info; remote-mapped services need certificate info obtained from this object
		//therefore the vIDCs service cannot enable SSL encryption; if enabled, it will either require certificate authentication
		//or not require certificate authentication, and the program cannot decide dynamically
#ifdef _SUPPORT_TLSCLIENT_
		this->setCacert(g_strMyCert.c_str(),g_strMyKey.c_str(),g_strKeyPswd.c_str(),false
					,g_strCaCert.c_str(),g_strCaCRL.c_str());
#endif
		
		//setIP filter rules
		this->rules().addRules_new(RULETYPE_TCP,m_ipaccess,m_ipRules.c_str());
		const char *ip=(m_bindip=="")?NULL:m_bindip.c_str();
		BOOL bReuseAddr=(ip)?SO_REUSEADDR:FALSE;//绑定了IP则允许port重用
		return this->Listen(m_svrport,bReuseAddr,ip);
	}
	
	int m_svrport;
	std::string m_bindip;
	bool m_autorun;
	long m_ipaccess;   //访问本proxy service IP filtering规则
	std::string m_ipRules;
};

#endif

