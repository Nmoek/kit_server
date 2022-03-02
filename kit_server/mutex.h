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
/**
 * @brief 信号量类
 */
class Semaphore : Noncopyable
{
public:
    /**
     * @brief 信号量类构造函数
     * @param[in] count 计数基准值, 一般以0位基准
     */
    Semaphore(uint32_t count = 0);

    /**
     * @brief 信号量类析构函数
     */
    ~Semaphore();

    //数-1
    void wait();
    //数+1
    void notify();

private:
    //禁止拷贝赋值
    Semaphore(const Semaphore &) = delete;
    Semaphore(const Semaphore &&) = delete;
    Semaphore& operator=(const Semaphore &) = delete;

private:
    /// 信号量结构体
    sem_t m_semaphore;
};

/**
 * @brief 局部区域互斥锁模板类
 * @tparam T 
 */
template<class T>
class ScopedLockImpl
{
public:
    /**
     * @brief 构造时加锁
     * @param[in] mutex 互斥锁变量 
     */
    ScopedLockImpl(T &mutex)
        :m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }

    /**
     * @brief 析构时解锁
     */
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
    /// 锁资源
    T& m_mutex;
    /// 上锁状态
    bool m_locked;

};


/**
 * @brief 互斥锁类
 */
class Mutex : Noncopyable
{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    /**
     * @brief 互斥锁类构造函数
     */
    Mutex() {pthread_mutex_init(&m_mutex, nullptr);}

    /**
     * @brief 互斥锁类析构函数
     */
    ~Mutex() {pthread_mutex_destroy(&m_mutex);}

    /**
     * @brief 加锁
     */
    void lock() {pthread_mutex_lock(&m_mutex);}

    /**
     * @brief 解锁
     */
    void unlock() {pthread_mutex_unlock(&m_mutex);}
private:
    /// 互斥锁
    pthread_mutex_t m_mutex;

};

/**
 * @brief 自旋锁类
 */
class SpinMutex: Noncopyable
{
public:
    typedef ScopedLockImpl<SpinMutex> Lock;
    
    /**
     * @brief 自旋锁类构造函数 
     */
    SpinMutex() {pthread_spin_init(&m_mutex, 0);}
        
    /**
     * @brief 自旋锁类析构函数
     */
    ~SpinMutex() {pthread_spin_destroy(&m_mutex);}

    /**
     * @brief 加锁
     */
    void lock() {pthread_spin_lock(&m_mutex);}

    /**
     * @brief 解锁
     */
    void unlock() {pthread_spin_unlock(&m_mutex);}

private:
    /// 自旋锁
    pthread_spinlock_t m_mutex;
};

/**
 * @brief 原子锁类
 */
class CASMutex: Noncopyable
{
public:
    typedef ScopedLockImpl<CASMutex> Lock;

    /**
     * @brief 原子锁类构造函数
     */
    CASMutex(){ m_mutex.clear();}

    /**
     * @brief 原子锁类析构函数
     */
    ~CASMutex(){}

    /**
     * @brief 加锁
     */
    void lock() 
    {
        //执行本次原子操作之前 所有读原子操作必须全部完成
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    /**
     * @brief 解锁
     */
    void unlock() 
    {
        //执行本次原子操作之前 所有写原子操作必须全部完成
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }


private:
    /// 原子锁
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

/**
 * @brief 局部区域读锁模板类
 * @tparam T 锁类型
 */
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

/**
 * @brief 局部区域写锁模板类
 * @tparam T 锁类型
 */
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

/**
 * @brief 读写锁类
 */
class RWMutex: Noncopyable
{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    /**
     * @brief 读写锁类构造函数 初始化读写锁
     */
    RWMutex() {pthread_rwlock_init(&m_rwlock, nullptr);}
    
    /**
     * @brief 读写锁类构造函数 初始化读写锁
     */
    ~RWMutex() {pthread_rwlock_destroy(&m_rwlock);}
    
    /**
     * @brief 读加锁
     */
    void rdlock() {pthread_rwlock_rdlock(&m_rwlock);}

    /**
     * @brief 写加锁
     */
    void wrlock() {pthread_rwlock_wrlock(&m_rwlock);}

    /**
     * @brief 读写全部解锁
     */
    void unlock() {pthread_rwlock_unlock(&m_rwlock);}
  
private:
    /// 读写锁
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
