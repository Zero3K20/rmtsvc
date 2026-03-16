/*******************************************************************
   *	httpsvr.cpp
   *    DESCRIPTION:HTTP protocol server implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:
   *
   *	net4cpp 2.1
   *	HTTP/1.1 transfer protocol
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/httpsvr.h"
#include "../utils/cTime.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

const long httpSession::SESSION_VALIDTIME=20*60;//20 minutes
const char httpSession::SESSION_IDNAME[]="aspsessionidgggggled";//"ASPSESSIONIDGGGGGLEC";

httpSession::httpSession()
{
	m_startTime=time(NULL);
	m_lastTime=m_startTime;
	//generate unique session ID
	srand( (unsigned)m_startTime );
	/* Display 23 大写字母. */
	for(int i = 0;   i < 23;i++ )
	{
		 m_sessionID[i]=(rand()*25)/RAND_MAX+65;
	} m_sessionID[23]=0;
}
//user可以自己set新的sessionID，而not用自动生成的
bool httpSession::SetSessionID(const char *strID)
{
	if(strID==NULL || strID[0]==0) return false;
	int i;
	for(i=0;i<23 && *strID;i++) m_sessionID[i]=*strID++;
	m_sessionID[i]=0; return true;
}

httpServer :: httpServer()
{
	m_strSvrname="HTTP Server";
}

httpServer :: ~httpServer()
{
	Close();
	//HTTPservice析构前要保证thread都end，因为thread中访问了httpServerclass的object
	m_threadpool.join(); 

	std::map<std::string,httpSession *>::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();it++) delete (*it).second;
}

//set web service root directory and virtual directories
bool httpServer :: setvpath(const char *vpath,const char *rpath,long lAccess)
{
	if(vpath==NULL || rpath==NULL) return false;
	if(vpath[0]!='/') return false;
	std::string str_vpath(vpath);
	::_strlwr((char *)str_vpath.c_str());
	if(str_vpath[str_vpath.length()-1]!='/') str_vpath.append("/");
	std::pair<std::string,long> p(rpath,lAccess);
#ifdef WIN32
	if(p.first!="") //末尾add'\'
		if(p.first[p.first.length()-1]!='\\') p.first.append("\\");
#endif
	m_dirAccess[str_vpath]=p;
	return true;
}

//triggered when current connection count exceeds the configured maximum connections
void httpServer :: onTooMany(socketTCP *psock)
{
	psock->Send(0,"HTTP/1.1 200 OK\r\n\r\n"
				"<html><head><title>Too many connections</title></head>"
				"<body>Too many connections,try later!</body></html>",-1);
	return;
}

//at此event中checkSessiontimeout
void httpServer :: onIdle(void)
{
	static time_t checkedTime=time(NULL);
	if(checkedTime-time(NULL)>20){//20秒钟check一次sessionyesnotimeout
		checkedTime=time(NULL);
		m_mutex.lock();
		std::map<std::string,httpSession *>::iterator it=m_sessions.begin();
		for(;it!=m_sessions.end();it++)
		{
			httpSession *psession=(*it).second;
			if(!psession->isValid(checkedTime)) { delete psession; m_sessions.erase(it); }
		}
		m_mutex.unlock(); 
	}
	return;
}
//triggered when a new client connects to this service
void httpServer :: onAccept(socketTCP *psock)
{
	httpRequest httpreq;
	while(true){ //循环handlereceive多个HTTP request
		SOCKSRESULT sr=httpreq.recv_reqH(psock,HTTP_MAX_RESPTIMEOUT);
		if(sr<=HTTP_REQ_UNKNOWN) break; //notyesHTTP requestor者timeout
		httpResponse httprsp; httpSession *psession;//handle并getsessionobject
		const char *strSessionID=httpreq.Cookies(httpSession::SESSION_IDNAME);
		m_mutex.lock();
		if(strSessionID){
			std::map<std::string,httpSession *>::iterator it=m_sessions.find(strSessionID);
			if(it!=m_sessions.end()) psession=(*it).second; else psession=NULL;
		}else psession=NULL;
		if(psession==NULL){ //set新的sessionID
			if( (psession=new httpSession)==NULL ) return;
			m_sessions[psession->sessionID()]=psession;	
			httprsp.SetCookie(httpSession::SESSION_IDNAME,psession->sessionID(),"/");
		}//防止userhandle阻塞or者非常耗时超过了设定的session的validtime，导致此session被删掉
		psession->m_lastTime=0x7fffffff; 
		m_mutex.unlock();
		if(!httpreq.ifReceivedAll() && httpreq.get_contentType(NULL)==HTTP_CONTENT_APPLICATION)
			if(!httpreq.recv_remainder(psock,-1)) return; //receive完剩余的form提交data,未receive完则returnclose
	//	RW_LOG_PRINTMAPS(httpreq.Cookies(),"Cookies");
	//	RW_LOG_PRINTMAPS(httpreq.QueryString(),"Get param");
	//	RW_LOG_PRINTMAPS(httpreq.Form(),"Post Param");

		//start准备handle HTTP request
		if(!onHttpReq(psock,httpreq,*psession,m_application,httprsp)){
			string vpath=httpreq.url(); //转交web servicedefaulthandle
			long lAccess=cvtVPath2RPath(vpath);
			if( lAccess!=HTTP_ACCESS_NONE) //将虚directory转化为实directory并getdirectory的访问permissions
			{
				if(vpath.length()>0 && vpath[vpath.length()-1]=='\\') 
					vpath.erase(vpath.length()-1);
				long iret=FILEIO::fileio_exist(vpath.c_str());
				if(iret==-1) //specified的fileordirectory does not exist
					httprsp_fileNoFind(psock,httprsp);
				else if(iret==-2) //specified的yesdirectory,listdirectory中的内容
				{	
					if((lAccess & HTTP_ACCESS_LIST)==0)
						httprsp_listDenied(psock,httprsp);
					else{
						httprsp.NoCache(); //CacheControl("No-cache");
						vpath.append("\\*"); 
						httprsp_listDir(psock,vpath,httpreq,httprsp);
					}
				}else{ //specified的yesfile
					//判断fileyesno被modify-------------- start---------------------
					cTime ct0; time_t t0=0,t1=1;
					const char *p=httpreq.Header("If-Modified-Since");
					if(p && ct0.parseDate(p) ){
						WIN32_FIND_DATA finddata;
						HANDLE hd=::FindFirstFile(vpath.c_str(), &finddata);
						if(hd!=INVALID_HANDLE_VALUE)
						{
							FILETIME ft=finddata.ftLastWriteTime;
							::FindClose(hd); t0=ct0.Gettime();
							cTime ct1(ft);
							t1=ct1.Gettime()+_timezone;//进行时区调整						 
						}
					}//?if(p)
					if(t1<=t0) //file没有被modify过
						httprsp_NotModify(psock,httprsp);
					else
					//判断fileyesno被modify--------------  end ---------------------
					{
						long lstartpos,lendpos;//getfile的范围
						int iRangeNums=httpreq.get_requestRange(&lstartpos,&lendpos,0);
						if(iRangeNums>1){
							long *lppos=new long[iRangeNums*2];
							if(lppos){
								for(int i=0;i<iRangeNums;i++) httpreq.get_requestRange(&lppos[i],&lppos[iRangeNums+i],i);
								httprsp.sendfile(psock,vpath.c_str(),MIMETYPE_UNKNOWED,&lppos[0],&lppos[iRangeNums],iRangeNums);
							} delete[] lppos;
						}else{
							if(iRangeNums==0) { lstartpos=0; lendpos=-1; }
							httprsp.sendfile(psock,vpath.c_str(),MIMETYPE_UNKNOWED,lstartpos,lendpos);
						}//?if(iRangeNums>1)
					}//?file被modify过
				} //specified的yesfile
			} else httprsp_accessDenied(psock,httprsp);
		}//?if(!onHttpReq(psock,httpreq,*psession,m_application,httprsp))

		psession->m_lastTime=time(NULL); //setsession的last访问time
		if(!httpreq.bKeepAlive()) break;
	}//?while(true)
}

//将绝对虚pathconvert为绝对实path(!!!虚pathnot区分size写)
//return对current实path的可操作permissions
//vpath -- [in|out] 输入绝对虚path，输出绝对实path
long httpServer :: cvtVPath2RPath(std::string &vpath)
{
	if(vpath=="" || vpath[0]!='/') return HTTP_ACCESS_NONE; //异常保护
	::_strlwr((char *)vpath.c_str());
	std::map<std::string,std::pair<std::string,long> >::iterator it=m_dirAccess.end();
	const char *ptr=vpath.c_str();
	const char *ptrBegin=ptr;
	do
	{	
		ptr++; char c=*ptr;
		*(char *)ptr=0;
		std::map<std::string,std::pair<std::string,long> >::iterator itTmp=m_dirAccess.find(ptrBegin);
		*(char *)ptr=c;
		if(itTmp==m_dirAccess.end()) break;
		it=itTmp;
	}while( (ptr=strchr(ptr,'/'))!=NULL );

	//this should never actually happen; m_dirAccess always has the root '/' set
	if(it==m_dirAccess.end()) { return HTTP_ACCESS_NONE;}
 
	vpath.erase(0,(*it).first.length());
	long lAccess=(*it).second.second;
	if((lAccess & HTTP_ACCESS_SUBDIR_INHERIT)!=0)
	{
		if(strchr(vpath.c_str(),'/')!=NULL) lAccess=0; //directory permissions inheritance disabled; subdirectory permissions are 0
	}
	for(int i=0;i<(int)vpath.length();i++) if(vpath[i]=='/') vpath[i]='\\';
	vpath.insert(0,(*it).second.first.c_str());
	return lAccess;
}
