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
	//-ssl 服务clientauthentication证书
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
		//设置vIDCs的SSL证书info，remote映射的服务需要证书info，将从此对象获取
		//因此vIDCs服务无法启用SSL加密，如果启用了必将根据证书设置情况要么是需要证书authentication的
		//要么是无需证书authentication的，程序将不能动态决定
#ifdef _SURPPORT_OPENSSL_
		this->setCacert(g_strMyCert.c_str(),g_strMyKey.c_str(),g_strKeyPswd.c_str(),false
					,g_strCaCert.c_str(),g_strCaCRL.c_str());
#endif
		
		//设置IP filter rules
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

