   /*******************************************************************
   *	cThread.cpp
   *    DESCRIPTION:thread class implementation
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/

#include "../include/sysconfig.h"  
#include "../include/cThread.h"
#include "../include/cLogger.h"

#include <vector>
using namespace std;
using namespace net4cpp21;

cThread::cThread()
{
	m_thrid=0;
	m_bStarted=false;
	m_threadfunc=NULL;
	m_pArgs=NULL;
}
cThread::~cThread()
{
	join(0); //yyc add 2009-12-15
	//threadobjectrelease时并not管threadyesnoend，ifuser要求必须end，自己调用joinfunction
#ifdef WIN32 //Windows system platform
	if(m_thrid) CloseHandle((HANDLE)m_thrid);//closeopen的thread handle
#else //unix/linux平台
	if(m_thrid) pthread_detach(m_thrid);//将thread分离,thread end后自己release相关资源
#endif
}

bool cThread::start(THREAD_CALLBACK *pfunc,void *pargs)
{
	if((m_threadfunc=pfunc)==NULL) return false;
	m_pArgs=pargs;
#ifdef WIN32 //Windows system platform
	unsigned int id;
	m_thrid=_beginthreadex(0,0,&threadfunc,(void *)this,0,&id);
	if(m_thrid==0)  return false;
#else //unix/linux平台
	int res = 0;
    	res = pthread_create(&m_thrid, 0, &threadfunc, (void *)this);
    	if (res != 0){ m_thrid=0; return false;}
#endif
	return true;
}

void cThread::join(time_t timeout)//stopthread并wait for thread to end才return
{
	if(m_thrid==0) return;
#ifdef WIN32 //Windows system platform
	DWORD dwMilliseconds=INFINITE;
	if(timeout>=0) dwMilliseconds=(DWORD)(timeout*1000);
	int res=WaitForSingleObject((HANDLE)m_thrid, dwMilliseconds);
	if(res==WAIT_TIMEOUT) ::TerminateThread((HANDLE)m_thrid,0);
//	if(res==WAIT_TIMEOUT){printf("aaaaaaqqqq\r\n"); ::TerminateThread((HANDLE)m_thrid,0);}
	CloseHandle((HANDLE)m_thrid);
#else //unix/linux平台
	//endall正阻塞waiting的function；如usleep(),select()等...
	pthread_kill(m_thrid,SIGALRM);
	pthread_join(m_thrid, 0);
#endif
	m_thrid=0;
	return;
}

#ifdef WIN32 //Windows system platform
unsigned int __stdcall cThread::threadfunc(void* param)
#else //unix/linux平台
void* cThread::threadfunc(void* param)
#endif
{
	cThread *pthread=(cThread *)param;
	pthread->m_bStarted=true;
	(*(pthread->m_threadfunc))(pthread->m_pArgs);
	//yyc 2009-12-15resume ，使用cTreadobject时必须保证cThreadobjectrelease时thread已经end
	pthread->m_bStarted=false; //yyc remove cThread *object可能已经invalid了
	pthread_exit(0);
	return 0;	
}

//--------------------------------------------------------------------------------------------
//---------------------cThreadPoolclass的implementation-----------------------------------------------------
cThreadPool :: cThreadPool()
{
	m_taskid=0;
}
cThreadPool :: ~cThreadPool()
{
	join();
}
//stopthread pool中的allthread并wait for thread to end才return
void cThreadPool :: join(time_t timeout)
{
	vector<pthread_t> vec;//临时saveallworker thread的句柄orid
	m_mutex.lock();
	m_tasklist.clear(); //清除任务list
	if(!m_thrmaps.empty()){
		map<pthread_t,THREADPARAM>::iterator it=m_thrmaps.begin();
		for(;it!=m_thrmaps.end();it++)
		{
			THREADPARAM &thrparam=(*it).second;
			vec.push_back(thrparam.m_thrid);
			thrparam.m_thrid=0;//告诉thread可以not必从m_thrmaps中erase
			thrparam.m_waittime=0;
			thrparam.m_pcond->active();
		}
	}//?if(!m_thrmaps.empty()){
	m_mutex.unlock();
	//waitingall正执行的worker threadend
	if(!vec.empty()){
		DWORD dwMilliseconds=INFINITE;
		if(timeout>=0) dwMilliseconds=(DWORD)(timeout*1000);
		vector<pthread_t>::iterator itVec=vec.begin();
		for(;itVec!=vec.end();itVec++)
		{
#ifdef WIN32 //Windows system platform
			WaitForSingleObject((HANDLE)(*itVec), dwMilliseconds);
#else //unix/linux平台
			//endall正阻塞waiting的function；如usleep(),select()等...
			pthread_kill(*itVec,SIGALRM);
			pthread_join(*itVec, 0);
#endif		
		}//?for(;itVec...
	}//?if(!vec.empty()){
	m_thrmaps.clear();
}

//initializationworker thread个数
//threadnum --- 要新create的thread个数
//waittime --- 新create的threadif休眠specified的time后仍然没有任务handle自动end
//		if==-1则一直休眠知道有specified的任务要handle
//returncurrenttotal的worker thread个数
long cThreadPool :: initWorkThreads(long threadnum,time_t waittime)
{
	for(long i=0;i<threadnum;i++)
		createWorkThread(waittime);
	long threads=0;
	m_mutex.lock();
	threads=m_thrmaps.size();
	m_mutex.unlock();
	return threads;
}
//add一个任务进入任务queue
//pfunc --- 任务functionpointer
//pargs --- 传递给任务function的parameter
//waittime --- ifcurrent thread池中的thread都被占用yesno临时create一个新的thread进入thread pool
//		if<0则notcreate，waiting其他threadnull闲后handle；otherwisecreate，此时waittime为when此threadcomplete此任务atthread pool中的maximum休眠waitingtime
//ifsuccess则return任务TASKID，otherwisereturn0
TASKID cThreadPool :: addTask(THREAD_CALLBACK *pfunc,void *pargs,time_t waittime)
{
	if(pfunc==NULL) return 0;
	TASKPARAM taskparam;
	taskparam.m_taskid=++m_taskid;
	taskparam.m_pFunc=pfunc;
	taskparam.m_pArgs=pargs;
	m_mutex.lock();
	m_tasklist.push_back(taskparam); //add到任务list中
	m_mutex.unlock();
	bool bStarted=false; long threads; //current thread个数
	m_mutex.lock();
	threads=m_thrmaps.size();
	map<pthread_t,THREADPARAM>::iterator it=m_thrmaps.begin();
	for(;it!=m_thrmaps.end();it++)
	{
		THREADPARAM &thrparam=(*it).second;
		if(thrparam.m_pcond && thrparam.m_pcond->status()) //此thread正处于休眠status
		{
			thrparam.m_pcond->active();
			bStarted=true;
			break;
		}
	}//?for(;it!=...
	m_mutex.unlock();
	if(!bStarted && waittime>=0 ){ //create一个临时thread执行handle任务
		if(createWorkThread(waittime)==0){ //create threadfailure
			if(delTask(taskparam.m_taskid)){
				RW_LOG_PRINT(LOGLEVEL_WARN,0,"有一个任务未被执行!\r\n");
				return 0; //delete任务，ifdeletenotsuccess说明任务已被其他thread运行
			}//?if(delTask(taskparam.m_taskid))
		}
		else
			threads++;
	}//?if(!bStarted &&...
//	RW_LOG_DEBUG("taskid=%d,threads=%d\r\n",taskparam.m_taskid,threads);
	return taskparam.m_taskid;
}
//检测某个任务yesnoat任务list中待执行
//ifbRemove==true则从任务list中delete
//ifat任务list中则returntrueotherwisereturnfalse
bool cThreadPool :: delTask(TASKID taskid,bool bRemove)
{
	bool bRet=false;
	m_mutex.lock();
	deque<TASKPARAM>::iterator it=m_tasklist.begin();
	for(;it!=m_tasklist.end();it++)
	{
		TASKPARAM &taskparam=*it;
		if(taskparam.m_taskid==taskid)
		{
			if(bRemove) m_tasklist.erase(it);
			bRet=true;
			break;
		}
	}//?for(;it!=...
	m_mutex.unlock();
	return bRet;
}
//清除all待执行的任务
void cThreadPool :: clearTasks()
{
	m_mutex.lock();
	m_tasklist.clear();
	m_mutex.unlock();
	return;
}

//create一个worker thread，successreturnthread ID，otherwisereturn0
pthread_t cThreadPool :: createWorkThread(time_t waittime)
{
	THREADPARAM thrparam;
	pthread_t thrid=0;
	//initializationTHREADPARAM
	thrparam.m_waittime=waittime;
	if( (thrparam.m_pcond=new cCond())==NULL) return 0;
	thrparam.m_thrid=0;
	//createworker thread
	m_mutex.lock();//先lock，这样thread运行后会先阻塞，直到unlockm_thrmaps[thrid]=thrparam;
#ifdef WIN32 //Windows system platform
	if( (thrparam.m_thrid=_beginthreadex(0,0,&workThread,(void *)this,0,(unsigned int *)&thrid))==0 )
		thrid=0;
#else //unix/linux平台
	int res = 0;
    	res = pthread_create(&m_thrid, 0, &threadfunc, (void *)this);
    	if (res != 0)
    		thrparam.m_thrid=0;
    	else
    		thrid=thrparam.m_thrid;
#endif
	//threadcreate完毕后add到threadqueue中
	if(thrid!=0)
	{
		m_thrmaps[thrid]=thrparam;
		thrparam.m_pcond=NULL;
	}
	m_mutex.unlock();
	if(thrparam.m_pcond) delete thrparam.m_pcond;	
	return thrid;
}
	
//thread pool中的worker thread
#ifdef WIN32 //Windows system platform
unsigned int __stdcall cThreadPool::workThread(void* param)
#else //unix/linux平台
void* cThreadPool::workThread(void* param)
#endif
{
	cThreadPool *pthreadpool=(cThreadPool *)param;
	if(pthreadpool==NULL) { pthread_exit(0); return 0; }
	pthread_t thrid=pthread_self();//得到唯一标识此thread的identifier
	THREADPARAM *pthrparam=NULL;
	pthreadpool->m_mutex.lock();
	if (pthreadpool->m_thrmaps.count(thrid)>0 )
		pthrparam=&pthreadpool->m_thrmaps[thrid];
	pthreadpool->m_mutex.unlock();
	if(pthrparam)
	{
#ifdef WIN32
		HANDLE Hthread=(HANDLE)pthrparam->m_thrid;
#endif
		cCond *pcond=pthrparam->m_pcond;
		TASKPARAM taskparam;
		do
		{//get工作任务
NEXTTASK:		 
			taskparam.m_taskid=0;
			pthreadpool->m_mutex.lock();
			if(pthreadpool->m_tasklist.size()>0)
			{
				taskparam=pthreadpool->m_tasklist.front();
				pthreadpool->m_tasklist.pop_front();
			}
			pthreadpool->m_mutex.unlock();
			if(taskparam.m_taskid!=0)
			{
				(*(taskparam.m_pFunc))(taskparam.m_pArgs);
				goto NEXTTASK;
			}
		}while(pcond->wait(pthrparam->m_waittime));
		pthrparam->m_pcond=NULL; delete pcond;
		if(pthrparam->m_thrid!=0){
			pthreadpool->m_mutex.lock();
			pthreadpool->m_thrmaps.erase(thrid);
			pthreadpool->m_mutex.unlock();
		}
#ifdef WIN32
		CloseHandle(Hthread);//closeopen的thread handle
#endif
	}//?if(pthrparam->)
	//worker threadend
	pthread_exit(0);
	return 0;	
}

/*about the difference between _beginthreadex and CreateThread
_beginthreadexyes微软的C/C++运行时库function，CreateThreadyes操作系统的function。
_beginthreadex通过调用CreateThread来implementation的，但比CreateThread多做了许多工作.
CreateThread、_beginthreadand_beginthreadex都yes用来startthread的
，_beginthreadyes_beginthreadex的功能子集，although_beginthread内部yes调用
_beginthreadex但他屏蔽了象安全特性这样的功能，所以_beginthread与CreateThreadnotyes同等级别，
_beginthreadexandCreateThreadat功能上完全可替代，我们就来比较一下_beginthreadex与CreateThread!

CRT的function库atthread出现之前就已经存at，所以原有的CRTnot能true正支持thread，这导致我们at编程的时候
有了CRT库的选择，atMSDN中查阅CRT的function时都有：
Libraries
LIBC.LIB Single thread static library, retail version 
LIBCMT.LIB Multithread static library, retail version 
MSVCRT.LIB Import library for MSVCRT.DLL, retail version 
这样的提示！
对于thread的支持yes后来的事！
这也导致了许多CRT的functionatmultithreading的情况下必须有特殊的支持，not能简单的使用CreateThread就OK。
大多的CRTfunction都可以atCreateThreadthread中使用，看资料说只有signal()functionnot可以，会导致process终止！
但可以用并notyes说没有问题！

有些CRT的function象malloc(), fopen(), _open(), strtok(), ctime(), orlocaltime()等function需要专门
的thread局部存储的data块，这个data块通常需要atcreate thread的时候就建立，if使用CreateThread，
这个data块就没有建立，然后会怎样呢？at这样的thread中还yes可以使用这些function而且没有出错，
实际上function发现这个data块的pointer为null时，会自己建立一个，然后将其与thread联系at一起，这意味着
if你用CreateThread来create thread，然后使用这样的function，会有一块memoryatnot知not觉中create，遗憾的
yes，这些function并not将其delete，而CreateThreadandExitThread也无法知道这件事，于yes就会有
Memory Leak，atthread频繁start的软件中(比如某些server软件)，迟早会让系统的memory资源耗尽！

_beginthreadex(内部也调用CreateThread)and_endthreadex就对这个memory块做了handle，所以没有问题！
(not会有人故意用CreateThreadcreate然后用_endthreadex终止吧，而且thread的终止最好not要显式的调用
终止function，自然exit最好！)

谈到Handle的问题，_beginthread的对应function_endthread自动的调用了CloseHandle，而
_beginthreadex的对应function_endthreadex则没有，所以CloseHandlein any case都yes要调用的not过
_endthread可以帮你执行自己not必写，其他两种就需要自己写！(Jeffrey Richter强烈推荐尽量
not用显式的终止function，用自然exit的方式，自然exitwhen然就一定要自己写CloseHandle)
*/
