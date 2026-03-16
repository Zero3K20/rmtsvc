/*******************************************************************
   *	ftpserver.h 
   *    DESCRIPTION: FTP service
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *******************************************************************/

#include "net4cpp21/include/sysconfig.h"
#include "net4cpp21/include/cCoder.h"
#include "net4cpp21/include/cLogger.h"

#include "net4cpp21/include/ftpsvr.h"

using namespace std;
using namespace net4cpp21;

//FTP service parameter structure
typedef struct _TFTPSetting
{
	bool autoStart;
	int svrport;
	bool ifSSLsvr;
	int dataportB;
	int dataportE;
	int maxUsers;
	int logEvent;
	std::string bindip;
	std::string tips; //welcome message
}TFTPSetting;

typedef struct _TFTPUser
{
	std::string username;//account name, case-insensitive (converted to lowercase)
	std::string userpwd;//password, if password=="" then no password verification required
	std::string userdesc;
	unsigned long maxupratio;//max upload rate K/s, 0 means unlimited
	unsigned long maxdwratio;//max download rate K/s, 0 means unlimited
	unsigned long maxupfilesize;//max upload file size in KBytes, 0 means unlimited
	unsigned long maxdisksize;//maximum disk space limit in KBytes, 0 means unlimited
	unsigned long curdisksize;//current used disk space in KBytes.
	std::map<std::string,std::pair<std::string,long> > dirAccess;//directory access permissions; directory names are case-sensitive
			//first --- string: FTP virtual directory path, ending with /, e.g. / or /aa/,
			//second --- pair : 此ftp虚directorycorresponding实际directoryanddirectory的访问permissions，实际directory必须为\结尾(winplatform)
	long ipaccess;
	std::string ipRules;//IP access rules
	long maxLoginusers;//limit the maximum simultaneous logged-in users for this account; <=0 means unlimited 
	time_t limitedTime;//limit this account to be valid only before a certain date; ==0 means unlimited
	long pswdmode;
	long disphidden; //whether显示隐藏file
	long forbid; //whether禁用此account
}TFTPUser;

class ftpsvrEx : public ftpServer
{
public:
	ftpsvrEx();
	virtual ~ftpsvrEx(){}
	bool Start();
	void Stop();
	int deleUser(const char *ptr_user);
	bool modiUser(TFTPUser &ftpuser);
	bool readIni();
	bool saveIni();
	void initSetting();
	bool parseIni(char *pbuffer,long lsize);
	bool saveAsstring(std::string &strini);

	TFTPSetting m_settings;
	std::map<std::string,TFTPUser> m_userlist;
protected:
	virtual void onLogEvent(long eventID,cFtpSession &session);
	void docmd_sets(const char *strParam);
	void docmd_ssls(const char *strParam);
	void docmd_ftps(const char *strParam);
	void docmd_user(const char *strParam);
	void docmd_vpath(const char *strParam);
	void docmd_iprules(const char *strParam);
private:
	
};
