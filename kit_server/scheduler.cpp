#include "scheduler.h"
#include "Log.h"
#include "macro.h"
#include "util.h"
#include "hook.h"

#include <string>
#include <iostream>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Coroutine* t_sche_coroutine = nullptr;


Scheduler::Scheduler(const std::string& name, size_t threads_size, bool use_caller)
    :m_name(name.size() ? name : Thread::GetName())
{

    KIT_ASSERT(threads_size > 0);

    //当前线程作为调度线程使用
    if(use_caller)
    {
        //初始化母协程  pre_cor_sp 被初始化
        Coroutine::Init();
        --threads_size; //减1是因为当前的这个线程也会被纳入调度 少创建一个线程 

        //这个断言防止 该线程中重复创建调度器
        KIT_ASSERT(Scheduler::GetThis() == nullptr);  

        t_scheduler = this;

        //新创建的主协程会参与到协程调度中
        m_mainCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this), 0, true));
        

        //线程的主协程不再是一开始使用协程初始化出来的那个母协程，而应更改为创建了调度器的协程
        t_sche_coroutine = m_mainCoroutine.get();
        m_mainThreadId = GetThreadId();
        m_threadIds.push_back(m_mainThreadId);
           
    }
    else //当前线程不作为调度线程使用
    {
        m_mainThreadId = -1;
    }

    //防止线程名称没改
    Thread::SetName(m_name);

    m_threadSum = threads_size;
}


Scheduler::~Scheduler()
{
    //只有正在停止运行才能析构
    KIT_ASSERT(m_stopping);

    if(Scheduler::GetThis() == this)
    {
        t_scheduler = nullptr;
    }
}

//开启调度器
void Scheduler::start()
{
    MutexType::Lock lock(m_mutex);

    //一开始 m_stopping = true
    if(!m_stopping)
        return;
    
    
    m_stopping = false;
    KIT_ASSERT(m_threads.empty());

    m_threads.resize(m_threadSum);
    for(size_t i = 0;i < m_threadSum;++i)
    {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));

        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    //开始调度器调度
    // if(m_mainCoroutine)
    //     m_mainCoroutine->call();
 
   
}

//调度器停止
void Scheduler::stop()
{
    /*
    *  用了use_caller的线程 必须在当前线程里去执行stop
    *  没有用use_caller的线程 可以任意在别的线程执行stop
    */
    m_autoStop = true;

    /*1.只有一个主线程在运行的情况  直接停止即可*/
    if(m_mainCoroutine && m_threadSum == 0 && 
        (m_mainCoroutine->getState() == Coroutine::State::TERM ||
         m_mainCoroutine->getState() == Coroutine::State::INIT))
    {
        KIT_LOG_INFO(g_logger) << this << ",scheduler name=" << m_name << ", stopped";

        m_stopping = true;

        if(stopping())
            return;
          
    }

    /*2.多个线程在运行  先把子线程停止  再停止主线程*/

    //主线程Id不为-1说明是创建调度器的线程
    if(m_mainThreadId != -1)
    {
        KIT_ASSERT(GetThis() == this);
    }
    else
    {
        KIT_ASSERT(GetThis() != this);
    }



    //其他线程根据这个标志位退出运行
    /**注意这里 m_stopping 必须放在 m_mainCoroutine->call()调用前 否则
     * 协程中idle()函数里面 stopping()函数永远不会被触发
     * */
    m_stopping = true;
    //唤醒其他线程
    for(size_t i = 0;i < m_threadSum;++i)
    {
        tickle();
    }

    //让主线程也唤醒
    if(m_mainCoroutine)
    {
        tickle();
    }


    //开始调度器调度
    if(m_mainCoroutine)
    {
        //调度器不结束就一直循环
        //方法1
        // while(!stopping())
        // {
          
        //     if(m_mainCoroutine->getState() == Coroutine::State::TERM ||
        //         m_mainCoroutine->getState() == Coroutine::State::EXCEPTION)
        //     {
        //         KIT_LOG_INFO(g_logger) << "main coroutine is term, reset";
        //         m_mainCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this), 0, true));

        //         t_sche_coroutine = m_mainCoroutine.get();
        //     }

        //     m_mainCoroutine->call();
        // }

        //方法2

        if(!stopping())
        {
            m_mainCoroutine->call();
        }
        
    }

    //这里这么写的作用：
    //1.保证工作队列的线程安全 2.快速将工作队列清空 将线程资源拿到这来进行清理
    std::vector<Thread::ptr> threads;
    {
        MutexType::Lock lock(m_mutex);
        threads.swap(m_threads);
    }

    for(auto &x : threads)
    {
        //在这阻塞等待回收线程
        x->join();
    }


}


void Scheduler::run()
{
    KIT_LOG_DEBUG(g_logger) << "run start!";
    //设置当前协程被调度的调度器是哪一个
    setThis();
    //当前线程置为HOOK状态
    SetHookEnable(true);

    //当前线程ID不等于主线程ID
    if(GetThreadId() != m_mainThreadId)
    {
        t_sche_coroutine = Coroutine::GetThis().get();
    }

    //创建一个专门跑idle()的协程
    Coroutine::ptr idle_coroutine(new Coroutine(std::bind(&Scheduler::idle, this)));
    //创建一个协程对象 接函数型的任务 依附到其上进行调度
    Coroutine::ptr cb_coroutine = nullptr;
    //创建一个被调度对象 接队列里拿出来的任务
    CoroutineObject co;

    while(1)
    {
        /* 一、从消息队列取出可执行对象 */

        //清空可执行对象
        co.reset();
        
        //是一个信号 没轮到当前线程执行任务 就要发出信号通知下一个线程去处理
        bool is_tickle = false;
        bool is_work = false;
        //加锁
        {  
            //取出协程消息队列的元素
            MutexType::Lock lock(m_mutex);
            auto it = m_coroutines.begin();
            for(;it != m_coroutines.end();++it)
            {
                //a.当前任务没有指定线程执行 且 不是我当前线程要处理的协程 跳过
                if(it->threadId != -1 && it->threadId != GetThreadId())
                {
                    is_tickle = true;
                    continue;
                }

                KIT_ASSERT(it->cor || it->cb);

                //b.契合线程的协程且正在处理  跳过
                if(it->cor && it->cor->getState() == Coroutine::State::EXEC)
                {
                    continue;
                }

                //b.是我当前线程要处理的任务/协程 就取出并且删除
                co = *it;
                m_coroutines.erase(it);
                ++m_activeThreadCount;
                is_work = true;
                break;
                
            }

            is_tickle |= it != m_coroutines.end();
     
                
            
        }//解锁
        

        if(is_tickle)
        {
            tickle();
        }

        /*二、根据可执行对象的类型 分为 协程和函数 来分别执行对应调度操作*/

        //a. 如果要执行的任务是协程
        //契合当前线程的协程还没执行完毕
        if(co.cor && co.cor->getState() != Coroutine::State::TERM && 
            co.cor->getState() != Coroutine::State::EXCEPTION)
        {

            //++m_activeThreadCount;
            co.cor->swapIn();
            --m_activeThreadCount;

            if(co.cor->getState() == Coroutine::State::READY)
            {
                schedule(co.cor);
            }
            else if(co.cor->getState() != Coroutine::State::TERM &&
                    co.cor->getState() != Coroutine::State::EXCEPTION)
            {
                //协程状态置为HOLD
                co.cor->setState(Coroutine::State::HOLD);
    
            }

            //可执行对象置空
            co.reset();

        }
        else if(co.cb)  //b. 如果要执行的任务是函数
        {

            if(cb_coroutine)    //协程体的指针不为空就继续利用现有空间
            {
                cb_coroutine->reset(co.cb);
            }  
            else    //为空就重新开辟
            {
                cb_coroutine.reset(new Coroutine(co.cb));
            }
            //可执行对象置空
            co.reset();

            //++m_activeThreadCount;
            cb_coroutine->swapIn();
            --m_activeThreadCount;


            //从上面语句调回之后的处理 分为 还需要继续执行 和 需要挂起
            if(cb_coroutine->getState() == Coroutine::State::READY)
            {
                schedule(cb_coroutine);
                //智能指针置空
                cb_coroutine.reset();
            }
            else if(cb_coroutine->getState() == Coroutine::State::TERM ||
                    cb_coroutine->getState() == Coroutine::State::EXCEPTION)
            {
                //把执行任务置为空
                cb_coroutine->reset(nullptr);
            }
            else
            {
                //状态置为 HOLD
                cb_coroutine->setState(Coroutine::State::HOLD);

                //智能指针置空
                cb_coroutine.reset();
            }
        }
        else   //c.没有任务需要执行  去执行idle() --->代表空转
        {   
            if(is_work)
            {
                --m_activeThreadCount;
                continue;
            }

            //负责idle()的协程结束了 说明当前线程也结束了直接break
            if(idle_coroutine->getState() == Coroutine::State::TERM)
            {
                KIT_LOG_INFO(g_logger) << "idle_coroutine TERM!";
                break;
            }


           
            ++m_idleThreadCount;
            idle_coroutine->swapIn();
            --m_idleThreadCount;
        

            if(idle_coroutine->getState() != Coroutine::State::TERM &&
            idle_coroutine->getState() != Coroutine::State::EXCEPTION)
            {
                //状态置为 HOLD
                idle_coroutine->setState(Coroutine::State::HOLD);
            }

            
        }
 

    }



}

//唤醒函数
void Scheduler::tickle()
{
    KIT_LOG_INFO(g_logger) << "coroutine tickle";
}

//让子类有其他的清理功能
bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);

    return m_autoStop && m_stopping &&
        !m_coroutines.size() && m_activeThreadCount == 0;
}

//协程空转函数
void Scheduler::idle()
{
    
    while(!stopping())
    {
        //KIT_LOG_INFO(g_logger) << "coroutine idle";
        //当前协程中途让出执行时间 
        Coroutine::YieldToHold();
    }

    /*注意：退出循坏意味着 MainFunc/CallMainFunc 已经结束
    * 当前协程会被置为TERM状态
    */

}


//设置当前线程的调度器
void Scheduler::setThis()
{
    t_scheduler = this;
}

//获取到调度器
Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}


//获取创建了调度器的主协程  和线程里的母协程不是一个概念
Coroutine* Scheduler::GetMainCor()
{
    return t_sche_coroutine;
}




}