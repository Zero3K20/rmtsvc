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
	unsigned long maxdisksize;//限制最大磁盘使用空间 KBytes,0--不限
	unsigned long curdisksize;//current已使用磁盘空间 KBytes.
	std::map<std::string,std::pair<std::string,long> > dirAccess;//目录访问permissions,目录区分size写
			//first --- string : ftp的虚目录path,最后以/end，例如/ 或 /aa/，
			//second --- pair : 此ftp虚目录对应的实际目录和目录的访问permissions，实际目录必须为\结尾(win平台)
	long ipaccess;
	std::string ipRules;//IP access rules
	long maxLoginusers;//限制此account的最大同时登录用户数,<=0则不限制 
	time_t limitedTime;//限制此account只在某个date之前有效，==0不限制
	long pswdmode;
	long disphidden; //是否显示隐藏文件
	long forbid; //是否禁用此account
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
