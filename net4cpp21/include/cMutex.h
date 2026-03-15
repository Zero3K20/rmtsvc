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
#elif defined MAC //暂时not supported
	//....
#else  //unix/linux平台 //运用POSIX库的多线程和mutex，condition variable函数
	extern "C"
	{ //多thread function,mutex
		#include <pthread.h>
	}
#endif


namespace net4cpp21
{
	class cMutexBase
	{
	protected:
		unsigned long m_lLock;//是否处于锁定status
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
		cMutexBase m_mutex; //假的mutex变量，实际没有任何意义，因为windows下condition variable和mutex没有关系
#else
		cMutex m_mutex; //linux/unix下等待需用到mutex，
		//linux/unix下condition variable必须和mutex配合使用,以防止多个线程同时请求pthread_cond_wait()（或pthread_cond_timedwait()，下同）的竞争
		//在调用pthread_cond_wait()前必须由本线程lock,mutex保持锁定status，并在线程挂起进入等待前unlock。在条件满足从而离开pthread_cond_wait()之前，mutex将被重新lock，以与进入pthread_cond_wait()前的lock动作对应
#endif
        pthread_cond_t m_cond;
		int m_status; //0处于非等待status，否则处于等待status
		unsigned long m_args;//用户可传递额外参数
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
		bool wait(time_t seconds=-1) //返回真--被激活，否则timeout
		{
			if(m_status!=0) return false;
			m_status=1;//等待激活status
			m_mutex.lock();
			if(seconds<0) //无限等待，直到被激活
			{
				 pthread_cond_wait(&m_cond,m_mutex.address());
			}
			else
			{
                struct  timespec t;
				t.tv_sec = (unsigned int)(time(NULL) + seconds);
				t.tv_nsec = (long)seconds;//windows 系统下通过tv_nsec取得定时秒数!!!
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

		bool status() //是否为等待status
		{
			return (m_status!=0)?true:false;
		}

		unsigned long getArgs(){return m_args;}
		void setArgs(unsigned long arg){m_args=arg;return;}
	};
}//?namespace net4cpp21

#endif
