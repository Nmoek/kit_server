#ifndef _KIT_MUTEX_H_
#define _KIT_MUTEX_H_

#include <memory>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>


#include "noncopyable.h"


namespace kit_server
{
//信号量类
class Semaphore : Noncopyable
{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    //数+1
    void wait();
    //数-1
    void notify();

private:
    //禁止拷贝赋值
    Semaphore(const Semaphore &) = delete;
    Semaphore(const Semaphore &&) = delete;
    Semaphore& operator=(const Semaphore &) = delete;

private:
    sem_t m_semaphore;
};


//局部区域通用锁模板
template<class T>
class ScopedLockImpl
{
public:
    ScopedLockImpl(T &mutex)
        :m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!m_locked)
        {
            m_locked = true;
            m_mutex.lock();
        }
    }

    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;

};

//通用锁
class Mutex : Noncopyable
{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() {pthread_mutex_init(&m_mutex, nullptr);}
    ~Mutex() {pthread_mutex_destroy(&m_mutex);}

    void lock() {pthread_mutex_lock(&m_mutex);}
    void unlock() {pthread_mutex_unlock(&m_mutex);}
private:
    pthread_mutex_t m_mutex;

};

//自旋锁
class SpinMutex: Noncopyable
{
public:
    typedef ScopedLockImpl<SpinMutex> Lock;
    
    SpinMutex() {pthread_spin_init(&m_mutex, 0);}
        
    ~SpinMutex() {pthread_spin_destroy(&m_mutex);}

    void lock() {pthread_spin_lock(&m_mutex);}

    void unlock() {pthread_spin_unlock(&m_mutex);}

private:
    pthread_spinlock_t m_mutex;
};

//原子锁
class CASMutex: Noncopyable
{
public:
    typedef ScopedLockImpl<CASMutex> Lock;

    CASMutex(){ m_mutex.clear();}
    ~CASMutex(){}


    void lock() 
    {
        //执行本次原子操作之前 所有读原子操作必须全部完成
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() 
    {
        //执行本次原子操作之前 所有写原子操作必须全部完成
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }


private:
    volatile std::atomic_flag m_mutex;

};


//空通用锁
class NullMutex: Noncopyable
{
public:
    typedef ScopedLockImpl<NullMutex> Lock;

    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};


//局部区域读锁模板
template<class T>
class ReadScopedLockImpl
{
public:
    ReadScopedLockImpl(T &mutex)
        :m_mutex(mutex)
    {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!m_locked)
        {
            m_locked = true;
            m_mutex.rdlock();
        }
    }

    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;

};

//局部区域写锁模板
template<class T>
class WriteScopedLockImpl
{
public:
    WriteScopedLockImpl(T &mutex)
        :m_mutex(mutex)
    {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!m_locked)
        {
            m_locked = true;
            m_mutex.wrlock();
        }
    }

    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;

};

//读写锁
class RWMutex: Noncopyable
{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() {pthread_rwlock_init(&m_rwlock, nullptr);}
    
    ~RWMutex() {pthread_rwlock_destroy(&m_rwlock);}
    
    void rdlock() {pthread_rwlock_rdlock(&m_rwlock);}

    void wrlock() {pthread_rwlock_wrlock(&m_rwlock);}

    void unlock() {pthread_rwlock_unlock(&m_rwlock);}
  
private:
    pthread_rwlock_t m_rwlock;
};


//空读写锁
class NUllRWMutex: Noncopyable
{
public:
    typedef ReadScopedLockImpl<NUllRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NUllRWMutex> WriteLock;

    NUllRWMutex() {}
    ~NUllRWMutex() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};


}
#endif
