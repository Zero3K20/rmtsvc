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
	bool bCascade; //whether to enable secondary/cascading proxy
	std::string cassvrip; //secondary proxy serviceaddressandport
	int castype; //proxy type supported by the secondary proxy
	bool casAuth; //whether secondary proxy requires authentication
	std::string casuser; //account and password for accessing the secondary proxy
	std::string caspswd;
	long ipaccess;   //IP filter rules for accessing this proxy service
	std::string ipRules;
	bool bLogdatafile; //whether to log proxy forwarded data
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
	long forbid; //whether to disable this account

	std::string strAccessDest;//allowed or forbidden destination addresses
	int bAccessDest; //whether the above specified destination is forbidden or allowed: 0 = forbidden, otherwise = allowed
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
