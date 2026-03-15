/*******************************************************************
   *	proxyserver.h 
   *    DESCRIPTION: proxy service
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *******************************************************************/

#include "net4cpp21/include/sysconfig.h"
#include "net4cpp21/include/cCoder.h"
#include "net4cpp21/include/cLogger.h"

#include "net4cpp21/include//proxysvr.h"

using namespace std;
using namespace net4cpp21;

//proxy service configuration parameters
typedef struct _TPROXYSETTINGS
{
	int svrport; //service port
	std::string bindip; //proxy service bind IP
	int svrtype; //proxy type supported by the service
	bool bAuth; //proxy service是否需要authentication
	bool autorun;  //program execution是否自动启动proxy service
	bool bCascade; //是否启用secondary proxy
	std::string cassvrip; //二级proxy serviceaddress和port
	int castype; //secondary proxy支持的代理type
	bool casAuth; //secondary proxy是否需要authentication
	std::string casuser; //访问secondary proxy的account和password
	std::string caspswd;
	long ipaccess;   //访问本proxy service IP filtering规则
	std::string ipRules;
	bool bLogdatafile; //是否记录代理forward data
}PROXYSETTINGS;


typedef struct _TProxyUser
{
	std::string username;//account,account不区分size写(account转换为小写)
	std::string userpwd;//password,如果password==""则，无需password verification
	std::string userdesc;
	long ipaccess;
	std::string ipRules;//IP access rules
	unsigned long maxratio;//最大带宽 K/s,如果=0则不限
	long maxLoginusers;//限制此account的最大同时登录用户数,<=0则不限制 
	time_t limitedTime;//限制此account只在某个date之前有效，==0不限制
	long forbid; //是否禁用此account

	std::string strAccessDest;//允许或禁止访问的目的
	int bAccessDest; //上述specified的目的是禁止还是允许 0禁止否则允许
}TProxyUser;

class proxysvrEx : public proxyServer
{
public:
	proxysvrEx();
	virtual ~proxysvrEx(){}
	bool Start();
	void Stop();
	int deleUser(const char *ptr_user);
	bool modiUser(TProxyUser &puser);
	bool readIni();
	bool saveIni();
	void initSetting();
	bool parseIni(char *pbuffer,long lsize);
	bool saveAsstring(std::string &strini);

	PROXYSETTINGS m_settings;
	std::map<std::string,TProxyUser> m_userlist;
protected:
	void docmd_psets(const char *strParam);
	void docmd_cassets(const char *strParam);
	void docmd_puser(const char *strParam);
	void docmd_iprules(const char *strParam);
private:
	
};
