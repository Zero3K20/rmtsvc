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
	bool bAuth; //whether the proxy service requires authentication
	bool autorun;  //whether to auto-start proxy service on program start
	bool bCascade; //whether启用secondary proxy
	std::string cassvrip; //secondary proxy serviceaddressandport
	int castype; //secondary proxy支持的代理type
	bool casAuth; //whether secondary proxy requires authentication
	std::string casuser; //访问secondary proxy的accountandpassword
	std::string caspswd;
	long ipaccess;   //访问本proxy service IP filtering规则
	std::string ipRules;
	bool bLogdatafile; //whether记录代理forward data
}PROXYSETTINGS;


typedef struct _TProxyUser
{
	std::string username;//account name, case-insensitive (converted to lowercase)
	std::string userpwd;//password; if password=="", no password verification is required
	std::string userdesc;
	long ipaccess;
	std::string ipRules;//IP access rules
	unsigned long maxratio;//maximum bandwidth K/s, 0 means unlimited
	long maxLoginusers;//limit the maximum simultaneous logged-in users for this account; <=0 means unlimited 
	time_t limitedTime;//limit this account to be valid only before a certain date; ==0 means unlimited
	long forbid; //whether禁用此account

	std::string strAccessDest;//允许or禁止访问的目的
	int bAccessDest; //上述specified的目的yes禁止还yes允许 0禁止otherwise允许
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
