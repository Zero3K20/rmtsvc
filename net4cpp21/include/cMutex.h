/*******************************************************************
   *	cMutex.h
   *    DESCRIPTION:mutex object, condition variable object
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CMUTEX_H__
#define __YY_CMUTEX_H__

#include <ctime>

#ifdef WIN32 //Windows system platform
	//Win32 mutex
	#define pthread_mutex_t CRITICAL_SECTION 
	#define pthread_mutex_lock(m) EnterCriticalSection(m) 
	#define pthread_mutex_unlock(m) LeaveCriticalSection(m) 
	#define pthread_mutex_init(m,p) InitializeCriticalSection(m)
	#define pthread_mutex_destroy(m) DeleteCriticalSection(m)
	
	//condition
	#define timespec time_my
	#define pthread_cond_t HANDLE
	#define pthread_cond_init(pCond,p) \
	{ \
		*(pCond)=CreateEvent(NULL,false,false,NULL); \
	}
	#define pthread_cond_destroy(pCond) CloseHandle(*(pCond))
	#define pthread_cond_wait(pCond,pMutex) WaitForSingleObject(*(pCond),INFINITE)
	#define pthread_cond_timedwait(pCond,pMutex,pt) WaitForSingleObject(*(pCond),(pt)->tv_nsec*1000)
	#define pthread_cond_signal(pCond) SetEvent(*(pCond))
#elif defined MAC //temporarily not supported
	//....
#else  //unix/linux platform // using POSIX library multithreading, mutex and condition variable functions
	extern "C"
	{ //multi-thread function, mutex
		#include <pthread.h>
	}
#endif


namespace net4cpp21
{
	class cMutexBase
	{
	protected:
		unsigned long m_lLock;//whether currently locked
	public:
		cMutexBase():m_lLock(0){}
		virtual ~cMutexBase(){}
		bool isLock(){return (m_lLock==0);}
		virtual void lock()
		{
			m_lLock++;
		}
		virtual void unlock()
		{
			m_lLock--;
		}
		virtual pthread_mutex_t *address()
		{
			return NULL;
		}
	};
	class cMutex:public cMutexBase
	{
	private:
		pthread_mutex_t m_mutex;
	public:
		cMutex()
		{
			pthread_mutex_init(&m_mutex,NULL);
		}
		virtual ~cMutex()
		{
			pthread_mutex_destroy(&m_mutex);
		}
		
		void lock()
		{
			cMutexBase::lock();
			pthread_mutex_lock(&m_mutex);
		}
		void unlock()
		{
			pthread_mutex_unlock(&m_mutex);
			cMutexBase::unlock();
		}

		pthread_mutex_t *address()
		{
			return &m_mutex;
		}
	};
	
	
	struct time_my
	{
		unsigned int tv_sec;
		long tv_nsec;
	};
	//condition variable
	class cCond
	{
	private:
#ifdef WIN32
		cMutexBase m_mutex; //dummy mutex variable, has no real meaning because on Windows condition variables and mutexes are unrelated
#else
		cMutex m_mutex; //on Linux/Unix, waiting requires a mutex,
		//on Linux/Unix, condition variables must be used together with a mutex to prevent multiple threads from simultaneously calling pthread_cond_wait() (or pthread_cond_timedwait(), same below) in a race condition
		//before calling pthread_cond_wait(), the current thread must lock the mutex; the mutex remains locked, and is unlocked before the thread suspends into wait. Before the condition is met and pthread_cond_wait() returns, the mutex will be re-locked to correspond to the lock action before entering pthread_cond_wait()
#endif
        pthread_cond_t m_cond;
		int m_status; //0: not in waiting status, otherwise: in waiting status
		unsigned long m_args;//user can pass extra parameters
	public:
		cCond()
		{
			m_status=0;
			m_args=0;
            		pthread_cond_init(&m_cond,NULL);
		}
		~cCond()
		{
			if(m_status!=0) pthread_cond_signal(&m_cond);
            		pthread_cond_destroy(&m_cond);
		}
		bool wait(time_t seconds=-1) //returns true if activated, otherwise timeout
		{
			if(m_status!=0) return false;
			m_status=1;//waiting activated status
			m_mutex.lock();
			if(seconds<0) //wait indefinitely until activated
			{
				 pthread_cond_wait(&m_cond,m_mutex.address());
			}
			else
			{
                struct  timespec t;
				t.tv_sec = (unsigned int)(time(NULL) + seconds);
				t.tv_nsec = (long)seconds;//on Windows, the timeout seconds are retrieved via tv_nsec!!!
				pthread_cond_timedwait(&m_cond,m_mutex.address(),&t);
			}
            m_mutex.unlock();
            if(m_status==0) return true;
            m_status=0;
            return false;
		}
		
		void active()
		{
            if(m_status!=0){
            	m_status=0;
            	pthread_cond_signal(&m_cond);
            }
            return;
		}

		pthread_cond_t *address()
		{
			return &m_cond;
		}

		bool status() //whether in waiting status
		{
			return (m_status!=0)?true:false;
		}

		unsigned long getArgs(){return m_args;}
		void setArgs(unsigned long arg){m_args=arg;return;}
	};
}//?namespace net4cpp21

#endif
