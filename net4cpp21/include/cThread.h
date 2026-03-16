/*******************************************************************
   *	cTthread.h
   *    DESCRIPTION:thread class
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CTHREAD_H__
#define __YY_CTHREAD_H__

#ifdef WIN32 //Windows system platform
	// When using this multithreaded class, this program requires the multithreaded library. 
	// when compiling with VC, specify /MT or /MD parameter
	// /MT means static link with multithreaded C runtime. Use /MD for dynamic linking.
	//		if VC++ is v5.0 and a higher version of msvcrt.dll is available, dynamic linking should be used.
	// If you are using the Visual C++ development environment, select the 
   	// Multi-Threaded runtime library in the compiler Project Settings dialog box.	
   	// The Multithread Libraries Compile Option
   	// To build a multithread application that uses the C run-time libraries, 
   	// you must tell the compiler to use a special version of the libraries (LIBCMT.LIB).
   	// To select these libraries, first open the Project Settings dialog box (Build menu) 
   	// and click the C/C++ tab. Select Code Generation from the Category drop-down list box. 
   	// From the Use Run-Time Library drop-down box, select Multithreaded. Click OK to return to editing.
	extern "C"
	{//multithreading
		#include <process.h>
	}
	#define pthread_t unsigned long
	#define pthread_self GetCurrentThreadId
	#define pthread_exit(retcode) \
	{ \
		_endthreadex(retcode); \
	}
#elif defined MAC //temporarily not supported
	//....
#else  //unix/linux平台
	//为了保证可移植，此处的多thread function用的yesPOSIX的multithreading库，therefore编译时应specifiedconnect -lpthread库。if用sun系统本身的多thread function则应制定connect-lthread库。
	//关于STL的multithreading安全
	//atsun的系统下编写multithreading程序时，if用了c++的STL库，则要小心，因为你会发现atsun下有可能会发生core dump，而同样的程序atlinux下缺始终没有问题。
	//用gdb定bitcore dumpbit置，发现可能会at你程序的任何地方，但totalatSTL库的stl_alloc.h的memory allocation处发生问题，某个address值被modify为一个非法的address导致出
	//问题，when然这也有可能yes由于你的memory非法操作的问题，如release了某个memorynull间却还atreference此address。查查stl_alloc.hfile你会发现原来STL的memory allocationdefault为
	//single_pthread模式，你必须define某个宏才会支持multithreading模式，packet括POSIXmultithreading、win32multithreading，Solarismultithreading等。
	//thereforesun系统下编写POSIX标准的multithreading程序，if用到了STL库，则at编译时一定要specified-D__STL_PTHREADS宏parameter，
	//linux下可能defaultyes以multithreading安全方式编译的，thereforenot会出问题。其他的multithreading宏define你可看看stl_alloc.h头file。
	//multithreading
	extern "C"
	{ //multi-thread function, mutex
		#include <pthread.h>
	}
#endif

#include "cMutex.h"
#include <map>
#include <deque>

#define THREADLIVETIME 5 //s, temporary thread auto-ends after this time if no tasks
namespace net4cpp21
{
	typedef void (THREAD_CALLBACK)(void *);
	class cThread
	{
		pthread_t m_thrid;//thread IDorthread handle(windows)
		THREAD_CALLBACK *m_threadfunc;
		void *m_pArgs;//传递给thread function的parameter
		bool m_bStarted;//threadyesnoin progress运行
	private:
#ifdef WIN32 //Windows system platform
    	static unsigned int __stdcall threadfunc(void* param);
#else //unix/linux平台
        static void* threadfunc(void* param);
#endif
	public:
		cThread();
		~cThread();
		bool start(THREAD_CALLBACK *pfunc,void *pargs);
		void join(time_t timeout=-1);//stopthread并wait for thread to end才return,timeout :swaitingend的秒
		bool status(){return m_bStarted;} //returncurrent threadyesno运行
	};
	
	//*******************************************************************************
	//thread poolclass************************************************************************
	//可动态create多个thread进入thread pool，thread pool的thread end一个任务后自动取任务list中的任务进行执行
	#define TASKID unsigned long
	typedef struct _TASKPARAM //任务parameterstructure
	{
		TASKID m_taskid;//唯一标识本任务的任务ID
		THREAD_CALLBACK *m_pFunc;//任务function入口pointer
		void *m_pArgs;//传递给任务的parameter
	}TASKPARAM;
	typedef struct _THREADPARAM //threadparameterstructure
	{
		pthread_t m_thrid;//thread IDorthread handle(windows)
		time_t m_waittime;//thread休眠waitingtime
		cCond *m_pcond;
	}THREADPARAM;
	class cThreadPool
	{
		TASKID m_taskid;//任务的唯一标识
		cMutex m_mutex;//mutex
		std::deque<TASKPARAM> m_tasklist;//任务list
		std::map<pthread_t,THREADPARAM> m_thrmaps;//threadqueue
	public:
		cThreadPool();
		~cThreadPool();
		void join(time_t timeout=-1);
		//initializationworker thread个数
		//threadnum --- 要新create的thread个数
		//waittime --- 新create的threadif休眠specified的time后仍然没有任务handle自动end
		//		if==-1则一直休眠知道有specified的任务要handle
		//returncurrenttotal的worker thread个数
		long initWorkThreads(long threadnum,time_t waittime=-1);
		//add一个任务进入任务queue
		//pfunc --- 任务functionpointer
		//pargs --- 传递给任务function的parameter
		//waittime --- ifcurrent thread池中的thread都被占用yesno临时create一个新的thread进入thread pool
		//		if<0则notcreate，waiting其他threadnull闲后handle；otherwisecreate，此时waittime为when此threadcomplete此任务atthread pool中的maximum休眠waitingtime
		//ifsuccess则return任务TASKID，otherwisereturn0
		TASKID addTask(THREAD_CALLBACK *pfunc,void *pargs,time_t waittime);
		//检测某个任务yesnoat任务list中待执行
		//ifbRemove==true则从任务list中delete
		//ifat任务list中则returntrueotherwisereturnfalse
		bool delTask(TASKID taskid,bool bRemove=true);
		//清除all待执行的任务
		void clearTasks();
		//returncurrent待执行的任务数
		long numTasks(){ long lret=0; m_mutex.lock(); lret=m_tasklist.size();m_mutex.unlock();return lret;}
		//returncurrent的worker thread个数
		long numThreads(){ long lret=0; m_mutex.lock(); lret=m_thrmaps.size();m_mutex.unlock();return lret;}
	private:
		//create一个worker thread，successreturnthread ID，otherwisereturn0
		pthread_t createWorkThread(time_t waittime);
#ifdef WIN32 //Windows system platform
    	static unsigned int __stdcall workThread(void* param);
#else //unix/linux平台
        static void* workThread(void* param);
#endif
	};
}//?namespace net4cpp21


#endif





