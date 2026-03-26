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
#else  //unix/linux platform
	//to ensure portability, the multi-thread functions here use the POSIX threading library; therefore specify -lpthread when linking. If using the Sun system's own thread functions, specify -lthread instead.
	//regarding STL thread safety
	//when writing multi-threaded programs on Sun systems, be careful if using the C++ STL library, because on Sun systems a core dump may occur, while the same program on Linux has no issues.
	//using GDB to pinpoint the core dump location, it may occur anywhere in your program, but always at the memory allocation in stl_alloc.h of the STL library; a certain address value is modified to an illegal address causing the problem,
	//which could also be due to illegal memory operations, such as freeing a memory space but still referencing that address. Check stl_alloc.h and you'll find that the STL memory allocation defaults to
	//single-threaded mode; you must define a certain macro to enable multithreading support, including POSIX multithreading, Win32 multithreading, Solaris multithreading, etc.
	//therefore when writing POSIX-standard multi-threaded programs on Sun systems using the STL library, you must specify the -D__STL_PTHREADS macro at compile time,
	//Linux may compile with thread-safe mode by default, so no issues occur. For other threading macros, see stl_alloc.h.
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
		void *m_pArgs;//parameter passed to the thread function
		bool m_bStarted;//whether the thread is currently running
	private:
#ifdef WIN32 //Windows system platform
    	static unsigned int __stdcall threadfunc(void* param);
#else //unix/linux platform
        static void* threadfunc(void* param);
#endif
	public:
		cThread();
		~cThread();
		bool start(THREAD_CALLBACK *pfunc,void *pargs);
		void join(time_t timeout=-1);//stop the thread and wait for it to end before returning; timeout: seconds to wait
		bool status(){return m_bStarted;} //return whether the current thread is running
	};
	
	//*******************************************************************************
	//thread poolclass************************************************************************
	//can dynamically create multiple threads to enter the thread pool; after completing a task, a thread automatically takes the next task from the task list
	#define TASKID unsigned long
	typedef struct _TASKPARAM //taskparameterstructure
	{
		TASKID m_taskid;//unique identifier for this task
		THREAD_CALLBACK *m_pFunc;//task function entry pointer
		void *m_pArgs;//parameter passed to the task
	}TASKPARAM;
	typedef struct _THREADPARAM //threadparameterstructure
	{
		pthread_t m_thrid;//thread IDorthread handle(windows)
		time_t m_waittime;//threadsleepingwaitingtime
		cCond *m_pcond;
	}THREADPARAM;
	class cThreadPool
	{
		TASKID m_taskid;//unique identifier for the task
		cMutex m_mutex;//mutex
		std::deque<TASKPARAM> m_tasklist;//tasklist
		std::map<pthread_t,THREADPARAM> m_thrmaps;//threadqueue
	public:
		cThreadPool();
		~cThreadPool();
		void join(time_t timeout=-1);
		//initialize number of worker threads
		//threadnum --- number of new threads to create
		//waittime --- new thread will auto-end if no task arrives within the specified sleep time
		//		if==-1, sleep indefinitely until there is a specified task to handle
		//return current total number of worker threads
		long initWorkThreads(long threadnum,time_t waittime=-1);
		//add a task to the task queue
		//pfunc --- task function pointer
		//pargs --- parameter passed to the task function
		//waittime --- whether to temporarily create a new thread if all threads in the pool are busy
		//		if <0, do not create; wait for another thread to become idle. Otherwise create; waittime is the max idle wait time in the pool after completing the task
		//returns task TASKID on success, otherwise returns 0
		TASKID addTask(THREAD_CALLBACK *pfunc,void *pargs,time_t waittime);
		//check whether a task is pending in the task list
		//if bRemove==true, delete from the task list
		//returns true if in the task list, otherwise false
		bool delTask(TASKID taskid,bool bRemove=true);
		//clear all pending tasks
		void clearTasks();
		//return current number of pending tasks
		long numTasks(){ long lret=0; m_mutex.lock(); lret=m_tasklist.size();m_mutex.unlock();return lret;}
		//return current number of worker threads
		long numThreads(){ long lret=0; m_mutex.lock(); lret=m_thrmaps.size();m_mutex.unlock();return lret;}
	private:
		//create a worker thread; returns thread ID on success, otherwise 0
		pthread_t createWorkThread(time_t waittime);
#ifdef WIN32 //Windows system platform
    	static unsigned int __stdcall workThread(void* param);
#else //unix/linux platform
        static void* workThread(void* param);
#endif
	};
}//?namespace net4cpp21


#endif





