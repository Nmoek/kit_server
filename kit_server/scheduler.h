#ifndef _KIT_SCHEDULER_H_
#define _KIT_SCHEDULER_H_

#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <pthread.h>
#include <iostream>

#include "coroutine.h"
#include "thread.h"
#include "mutex.h"
#include "Log.h"


namespace kit_server
{


class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    //use_caller作用：main线程也需要纳入到线程池的管理中来，
    //执行构造的地方如果该参数为true 代表当前线程需要纳入进来
    Scheduler(const std::string& name = "", size_t threads_size = 1, bool use_caller = true);

    virtual ~Scheduler();

    //获取调度器的名称
    const std::string& getName() const {return m_name;}
    
    void start();

    void stop();

    bool isIdleThreads() {return m_idleThreadCount > 0;}

    //调度函数  单个放入队列
    template<class CorOrCB>
    void schedule(CorOrCB cc, int threadId = -1)
    {
        bool isEmpty = false;

        {
            MutexType::Lock lock(m_mutex);
            isEmpty = scheduleNoLock(cc, threadId);
        }

        if(isEmpty)
            tickle();
    }

    //调度函数 批量放入队列
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end)
    {
        bool isEmpty = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end)
            {
                //只要有一次为真 就认为之前经历了空队列  必然有休眠 就必然要唤醒
                isEmpty = scheduleNoLock((&(*begin))) || isEmpty; 
                ++begin;
            }
        }

        if(isEmpty)
            tickle();
    }

protected:
    //唤醒函数
    virtual void tickle();
    
    //真正执行协程的调度
    void run();

    //让子类有其他的清理功能
    virtual bool stopping();

    //协程没有任务可做要执行它
    virtual void idle();

    //设置线程当前调度器
    void setThis();

private:
    //添加任务函数 真正执行添加动作
    template<class CorOrCB>
    bool scheduleNoLock(CorOrCB cc, int threadId = -1)
    {
        //如果为 true 则说明之前没有任何任务需要执行 现在放入一个任务
        bool isEmpty = m_coroutines.empty();

        CoroutineObject co(cc, threadId);
        if(co.cor || co.cb)
        {
            m_coroutines.push_back(co);
            //KIT_LOG_INFO(KIT_LOG_ROOT()) << "任务已添加";
        
        }

        return isEmpty; 
    }

public:
    //获取到调度器
    static Scheduler* GetThis();
    //获取主协程  和线程里的母协程不是一个概念
    static Coroutine* GetMainCor();

private:
    //自定义一个可执行对象结构体
    struct CoroutineObject
    {
        Coroutine::ptr cor;
        std::function<void()> cb;
        pid_t threadId;

        CoroutineObject(Coroutine::ptr p, pthread_t t)
            :cor(p), threadId(t){ }

        CoroutineObject(Coroutine::ptr* p, pthread_t t)
            :threadId(t)
        { 
            //减少一次智能指针引用
            cor.swap(*p);
        }    

        CoroutineObject(std::function<void()> f, pthread_t t)
            :cb(f), threadId(t) { }

        CoroutineObject(std::function<void()> *f, pthread_t t)
            :threadId(t) 
        {
            //减少一次智能指针引用
            cb.swap(*f);
        }

        //和STL结合必须有默认构造函数
        CoroutineObject()
            :threadId(-1){ }

        void reset()
        {
            cor = nullptr;
            cb = nullptr;
            threadId = -1;
        }

    };


protected:
    //线程ID数组
    std::vector<int> m_threadIds;
    //总共线程数
    size_t m_threadSum;
    //活跃线程数
    std::atomic<size_t> m_activeThreadCount = {0};
    //空闲线程数
    std::atomic<size_t> m_idleThreadCount = {0};
    //主线程ID
    pid_t m_mainThreadId = 0;
    //正在停止运行标志
    bool m_stopping = true;
    //是否自动停止标志
    bool m_autoStop = false;

private:    
    //线程池 工作队列
    std::vector<Thread::ptr> m_threads;
    //任务队列
    std::list<CoroutineObject> m_coroutines;
    //互斥锁
    MutexType m_mutex;
    //主协程智能指针 
    Coroutine::ptr m_mainCoroutine;
    //调度器名称
    std::string m_name;
};


}

#endif