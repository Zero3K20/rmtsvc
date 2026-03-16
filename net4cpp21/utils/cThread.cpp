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
	//when thread object is released, it does not care whether the thread has ended; if the user requires it to end, call the join function yourself
#ifdef WIN32 //Windows system platform
	if(m_thrid) CloseHandle((HANDLE)m_thrid);//close the opened thread handle
#else //unix/linux platform
	if(m_thrid) pthread_detach(m_thrid);//detach the thread; it will release its resources automatically when it ends
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
#else //unix/linux platform
	int res = 0;
    	res = pthread_create(&m_thrid, 0, &threadfunc, (void *)this);
    	if (res != 0){ m_thrid=0; return false;}
#endif
	return true;
}

void cThread::join(time_t timeout)//stop the thread and wait for it to end before returning
{
	if(m_thrid==0) return;
#ifdef WIN32 //Windows system platform
	DWORD dwMilliseconds=INFINITE;
	if(timeout>=0) dwMilliseconds=(DWORD)(timeout*1000);
	int res=WaitForSingleObject((HANDLE)m_thrid, dwMilliseconds);
	if(res==WAIT_TIMEOUT) ::TerminateThread((HANDLE)m_thrid,0);
//	if(res==WAIT_TIMEOUT){printf("aaaaaaqqqq\r\n"); ::TerminateThread((HANDLE)m_thrid,0);}
	CloseHandle((HANDLE)m_thrid);
#else //unix/linux platform
	//end all currently blocking wait functions such as usleep(), select(), etc.
	pthread_kill(m_thrid,SIGALRM);
	pthread_join(m_thrid, 0);
#endif
	m_thrid=0;
	return;
}

#ifdef WIN32 //Windows system platform
unsigned int __stdcall cThread::threadfunc(void* param)
#else //unix/linux platform
void* cThread::threadfunc(void* param)
#endif
{
	cThread *pthread=(cThread *)param;
	pthread->m_bStarted=true;
	(*(pthread->m_threadfunc))(pthread->m_pArgs);
	//yyc 2009-12-15 resume: when using cThread object, ensure the thread has ended before releasing the cThread object
	pthread->m_bStarted=false; //yyc remove: the cThread * object may already be invalid
	pthread_exit(0);
	return 0;	
}

//--------------------------------------------------------------------------------------------
//---------------------cThreadPool class implementation-----------------------------------------------------
cThreadPool :: cThreadPool()
{
	m_taskid=0;
}
cThreadPool :: ~cThreadPool()
{
	join();
}
//stop all threads in the thread pool and wait for them to end before returning
void cThreadPool :: join(time_t timeout)
{
	vector<pthread_t> vec;//temporarily save all worker thread handles or IDs
	m_mutex.lock();
	m_tasklist.clear(); //clear the task list
	if(!m_thrmaps.empty()){
		map<pthread_t,THREADPARAM>::iterator it=m_thrmaps.begin();
		for(;it!=m_thrmaps.end();it++)
		{
			THREADPARAM &thrparam=(*it).second;
			vec.push_back(thrparam.m_thrid);
			thrparam.m_thrid=0;//tell thread it does not have to erase itself from m_thrmaps
			thrparam.m_waittime=0;
			thrparam.m_pcond->active();
		}
	}//?if(!m_thrmaps.empty()){
	m_mutex.unlock();
	//wait for all currently executing worker threads to end
	if(!vec.empty()){
		DWORD dwMilliseconds=INFINITE;
		if(timeout>=0) dwMilliseconds=(DWORD)(timeout*1000);
		vector<pthread_t>::iterator itVec=vec.begin();
		for(;itVec!=vec.end();itVec++)
		{
#ifdef WIN32 //Windows system platform
			WaitForSingleObject((HANDLE)(*itVec), dwMilliseconds);
#else //unix/linux platform
			//end all currently blocking wait functions such as usleep(), select(), etc.
			pthread_kill(*itVec,SIGALRM);
			pthread_join(*itVec, 0);
#endif		
		}//?for(;itVec...
	}//?if(!vec.empty()){
	m_thrmaps.clear();
}

//initialize number of worker threads
//threadnum --- number of new threads to create
//waittime --- new thread will auto-end if no task arrives within the specified sleep time
//		if==-1则一直sleeping知道有specified的task要handle
//return current total number of worker threads
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
//add a task to the task queue
//pfunc --- task function pointer
//pargs --- parameter passed to the task function
//waittime --- whether to temporarily create a new thread if all threads in the pool are busy
//		if <0, do not create; wait for another thread to become idle. Otherwise create; waittime is the max idle wait time in the pool after completing the task
//returns task TASKID on success, otherwise returns 0
TASKID cThreadPool :: addTask(THREAD_CALLBACK *pfunc,void *pargs,time_t waittime)
{
	if(pfunc==NULL) return 0;
	TASKPARAM taskparam;
	taskparam.m_taskid=++m_taskid;
	taskparam.m_pFunc=pfunc;
	taskparam.m_pArgs=pargs;
	m_mutex.lock();
	m_tasklist.push_back(taskparam); //add to task list
	m_mutex.unlock();
	bool bStarted=false; long threads; //current number of threads
	m_mutex.lock();
	threads=m_thrmaps.size();
	map<pthread_t,THREADPARAM>::iterator it=m_thrmaps.begin();
	for(;it!=m_thrmaps.end();it++)
	{
		THREADPARAM &thrparam=(*it).second;
		if(thrparam.m_pcond && thrparam.m_pcond->status()) //this thread is currently sleeping
		{
			thrparam.m_pcond->active();
			bStarted=true;
			break;
		}
	}//?for(;it!=...
	m_mutex.unlock();
	if(!bStarted && waittime>=0 ){ //create a temporary thread to execute the task
		if(createWorkThread(waittime)==0){ //create threadfailure
			if(delTask(taskparam.m_taskid)){
				RW_LOG_PRINT(LOGLEVEL_WARN,0,"a task was not executed!\r\n");
				return 0; //delete the task; if delete fails it means the task is already running in another thread
			}//?if(delTask(taskparam.m_taskid))
		}
		else
			threads++;
	}//?if(!bStarted &&...
//	RW_LOG_DEBUG("taskid=%d,threads=%d\r\n",taskparam.m_taskid,threads);
	return taskparam.m_taskid;
}
//check whether a task is pending in the task list
//if bRemove==true, delete from the task list
//returns true if in the task list, otherwise false
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
//clear all pending tasks
void cThreadPool :: clearTasks()
{
	m_mutex.lock();
	m_tasklist.clear();
	m_mutex.unlock();
	return;
}

//create a worker thread; returns thread ID on success, otherwise 0
pthread_t cThreadPool :: createWorkThread(time_t waittime)
{
	THREADPARAM thrparam;
	pthread_t thrid=0;
	//initializationTHREADPARAM
	thrparam.m_waittime=waittime;
	if( (thrparam.m_pcond=new cCond())==NULL) return 0;
	thrparam.m_thrid=0;
	//createworker thread
	m_mutex.lock();//lock first so the thread blocks until unlock; m_thrmaps[thrid]=thrparam;
#ifdef WIN32 //Windows system platform
	if( (thrparam.m_thrid=_beginthreadex(0,0,&workThread,(void *)this,0,(unsigned int *)&thrid))==0 )
		thrid=0;
#else //unix/linux platform
	int res = 0;
    	res = pthread_create(&m_thrid, 0, &threadfunc, (void *)this);
    	if (res != 0)
    		thrparam.m_thrid=0;
    	else
    		thrid=thrparam.m_thrid;
#endif
	//after thread is created, add it to the thread queue
	if(thrid!=0)
	{
		m_thrmaps[thrid]=thrparam;
		thrparam.m_pcond=NULL;
	}
	m_mutex.unlock();
	if(thrparam.m_pcond) delete thrparam.m_pcond;	
	return thrid;
}
	
//worker thread in the thread pool
#ifdef WIN32 //Windows system platform
unsigned int __stdcall cThreadPool::workThread(void* param)
#else //unix/linux platform
void* cThreadPool::workThread(void* param)
#endif
{
	cThreadPool *pthreadpool=(cThreadPool *)param;
	if(pthreadpool==NULL) { pthread_exit(0); return 0; }
	pthread_t thrid=pthread_self();//get the unique identifier for this thread
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
		{//get work task
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
		CloseHandle(Hthread);//close the opened thread handle
#endif
	}//?if(pthrparam->)
	//worker threadend
	pthread_exit(0);
	return 0;	
}

/*about the difference between _beginthreadex and CreateThread
_beginthreadex is a Microsoft C/C++ runtime library function; CreateThread is an OS function.
_beginthreadex is implemented by calling CreateThread but does much more work.
CreateThread, _beginthread and _beginthreadex are all used to start threads.
_beginthread is a functional subset of _beginthreadex; although _beginthread internally calls
_beginthreadex, it hides features like security attributes, so _beginthread and CreateThread are not equivalent;
_beginthreadex and CreateThread are fully interchangeable in functionality. Let's compare them:

The CRT function library existed before threads, so the original CRT could not truly support threads. This means when programming we
have choices with CRT libraries; when looking up CRT functions in MSDN you'll see:
Libraries
LIBC.LIB Single thread static library, retail version 
LIBCMT.LIB Multithread static library, retail version 
MSVCRT.LIB Import library for MSVCRT.DLL, retail version 
such warnings!
Thread support was added later!
This also means many CRT functions must have special support in multithreaded situations; simply using CreateThread is not OK.
大多的CRTfunction都可以atCreateThreadthread中使用，看资料说只有signal()functionnot可以，会导致process终止！
但可以用并is not说没有问题！

有些CRT的function象malloc(), fopen(), _open(), strtok(), ctime(), orlocaltime()等function需要专门
的threadlocal存储的data块，这个data块通常需要atcreate thread的时候就建立，if使用CreateThread，
这个data块就没有建立，then会怎样呢？at这样的thread中还yes可以使用这些function而且没有出错，
实际上function发现这个data块的pointer为null时，会自己建立一个，then将其与thread联系at一起，这意味着
if你用CreateThread来create thread，then使用这样的function，会有一块memoryatunknownnot觉中create，遗憾的
yes，这些function并not将其delete，而CreateThreadandExitThread也无法知道这件事，于yes就会有
Memory Leak，in thread频繁start的软件中(比如someserver软件)，迟早会让system的memoryresource耗尽！

_beginthreadex(内部也调用CreateThread)and_endthreadex就对这个memory块做了handle，所以没有问题！
(will not有人故意用CreateThreadcreatethen用_endthreadex终止吧，而且thread的终止最好do not显式的调用
终止function，自然exit最好！)

谈到Handle的问题，_beginthread的对应function_endthreadauto的调用了CloseHandle，而
_beginthreadex的对应function_endthreadex则没有，所以CloseHandlein any case都yes要调用的not过
_endthread可以帮你执行自己not necessary to写，其他两种就需要自己写！(Jeffrey Richter强烈推荐尽量
not用显式的终止function，用自然exit的方式，自然exitwhen然就一定要自己写CloseHandle)
*/
