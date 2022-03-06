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

/**
 * @brief 调度器类
 */
class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 调度器类构造函数
     * @param[in] name 调度器名称
     * @param[in] threads_size 初始线程数量
     * @param[in] use_caller 当前线程是否纳入调度队列 默认纳入调度
     */
    Scheduler(const std::string& name = "", size_t threads_size = 1, bool use_caller = true);

    /**
     * @brief 调度器类析构函数
     */
    virtual ~Scheduler();

    /**
     * @brief 获取调度器的名称
     * @return const std::string& 
     */
    const std::string& getName() const {return m_name;}
    
    /**
     * @brief 开启调度器
     */     
    void start();

    /**
     * @brief 停止调度器
     */
    void stop();

    /**
     * @brief 是否存在空闲线程
     * @return true 存在空闲线程
     * @return false 不存在空闲线程
     */
    bool isIdleThreads() {return m_idleThreadCount > 0;}

    /**
     * @brief 将单个任务加入队列
     * @tparam CorOrCB 协程/函数类型
     * @param[in] cc 具体的被调度对象
     * @param[in] threadId 指定任务被哪一个线程调度 默认不指定
     */
    template<class CorOrCB>
    void schedule(CorOrCB cc, int threadId = -1)
    {
        //该标志判断放入任务之前 队列是否空的
        bool isEmpty = false;
        {
            MutexType::Lock lock(m_mutex);
            isEmpty = scheduleNoLock(cc, threadId);
        }

        //如果放入任务之前队列为空 要去唤醒线程抢任务
        if(isEmpty)
            tickle();
    }

    /**
     * @brief 将多个任务加入队列
     * @tparam InputIterator 协程/函数容器迭代器类型
     * @param[in] begin 起始迭代器 
     * @param[in] end 末尾迭代器
     */
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
    /**
     * @brief 线程唤醒函数
     */
    virtual void tickle();
    
    /**
     * @brief 执行协程调度的回调函数
     */
    void run();

    /**
     * @brief 调度器停止的判断条件
     * @return true 停止成功
     * @return false 停止失败
     */
    virtual bool stopping();

    /**
     * @brief 执行协程空转的回调函数
     */
    virtual void idle();

    /**
     * @brief 设置当前线程下运行的调度器this指针
     */
    void setThis();

private:
    /**
     * @brief 真正执行任务添加动作
     * @tparam CorOrCB 协程/函数类型
     * @param[in] cc 具体的被调度对象
     * @param[in] threadId 指定任务被哪一个线程调度 默认不指定
     * @return true 放入任务之前队列为空
     * @return false 放入任务之前队列不空
     */
    template<class CorOrCB>
    bool scheduleNoLock(CorOrCB cc, int threadId = -1)
    {
        //如果为 true 则说明之前没有任何任务需要执行 现在放入一个任务
        bool isEmpty = m_coroutines.empty();

        CoroutineObject co(cc, threadId);
        if(co.cor || co.cb)
        {
            m_coroutines.push_back(co);
        }

        return isEmpty; 
    }

public:
    /**
     * @brief 获取当前线程下运行的调度器this指针
     * @return Scheduler* 
     */
    static Scheduler* GetThis();

    /**
     * @brief 获取持有调度器的协程指针
     * @return Coroutine* 
     */
    static Coroutine* GetMainCor();

private:
    /**
     * @brief 可执行对象结构体
     */
    struct CoroutineObject
    {
        /// 协程
        Coroutine::ptr cor;
        /// 函数
        std::function<void()> cb;
        /// 指定的执行线程
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

        /*和STL结合必须有默认构造函数*/
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
    /// 线程ID数组
    std::vector<int> m_threadIds;
    /// 总共线程数
    size_t m_threadSum;
    /// 活跃线程数
    std::atomic<size_t> m_activeThreadCount = {0};
    /// 空闲线程数
    std::atomic<size_t> m_idleThreadCount = {0};
    /// 主线程ID
    pid_t m_mainThreadId = 0;
    /// 正在停止运行标志
    bool m_stopping = true;
    /// 是否自动停止标志
    bool m_autoStop = false;

private:    
    /// 线程池 工作队列
    std::vector<Thread::ptr> m_threads;
    /// 任务队列
    std::list<CoroutineObject> m_coroutines;
    /// 互斥锁
    MutexType m_mutex;
    /// 主协程智能指针 
    Coroutine::ptr m_mainCoroutine;
    // 调度器名称
    std::string m_name;
};


}

#endif