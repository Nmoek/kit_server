#include "Log.h"
#include "timer.h"
#include "util.h"

#include <vector>

namespace kit_server
{

static Logger::ptr g_logger  = KIT_LOG_NAME("system");

/*********************************Timer********************************/
Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
    :m_ms(ms), m_cb(cb), m_recurring(recurring), m_manager(manager)
{
    m_next = GetCurrentMs() + m_ms;

}

Timer::Timer(uint64_t next)
    :m_next(next)
{

}

//取消定时器任务
bool Timer::cancel()
{
    TimerManager::MutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb)
    {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end())
        {
            KIT_LOG_ERROR(g_logger) << "Timer::cancel get timer error";
            return false;
        }
        m_manager->m_timers.erase(it);

        return true;
    }

    return false;

}

//刷新定时器时间
bool Timer::refresh()
{
    TimerManager::MutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb)
        return false;

    auto it = m_manager->m_timers.find(shared_from_this());

    if(it == m_manager->m_timers.end())
    {
        return false;
    }

    //注意：由于使用的set 要先移除再进行重置加入 不能直接在原来的定时器上直接修改时间
    m_manager->m_timers.erase(it);
    m_next = GetCurrentMs() + m_ms;
    m_manager->m_timers.insert(shared_from_this());

    return true;

}

//重新设定定时器到期时间
bool Timer::reset(uint64_t ms, bool from_now)
{
    if(m_ms == ms && !from_now)
        return true;

    TimerManager::MutexType::WriteLock lock(m_manager->m_mutex);

    if(!m_cb)   //没有任务就要返回
        return false;


    auto it = m_manager->m_timers.find(shared_from_this());

    if(it == m_manager->m_timers.end())
    {
        return false;
    }
    //注意：由于使用的set 要先移除再进行重置加入
    m_manager->m_timers.erase(it);

    uint64_t start = 0;
    //重新从现在开始计时
    if(from_now)
        start = GetCurrentMs();
    else //继续上一次的计时
        start = m_next - m_ms;
    
    m_ms = ms;
    m_next = start + m_ms;

    //写锁解锁
    lock.unlock();

    //因为reset可能出现执行时间变成最小的可能(放在队头) 会有一次唤醒
    m_manager->addTimer(shared_from_this());

    return true;
}



bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
{
    /*判断地址*/
    if(!lhs && !rhs)
        return false;
    
    if(!lhs)
        return true;
    
    if(!rhs)
        return false;
    
    /*判断到期时间*/
    if(lhs->m_next < rhs->m_next)
        return true;

    if(lhs->m_next > rhs->m_next)
        return false;
        
    //到期时间相等的话 地址小的优先调度
    return lhs.get() < rhs.get();
    
}




/*********************************TimerManager********************************/
TimerManager::TimerManager()
{
    m_previousTime = GetCurrentMs();
}

TimerManager::~TimerManager()
{

}

//创建并添加定时器
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
{
    //在TimerManager 中构造Timer
    Timer::ptr timer(new Timer(ms, cb, recurring, this));

    // MutexType::WriteLock lock(m_mutex);
    addTimer(timer);

    return timer;
}

//添加定时器
void TimerManager::addTimer(Timer::ptr p)
{
    MutexType::WriteLock lock(m_mutex);
    //insert返回两个值 插入后的迭代器位置/插入是否成功
    //判断插入后是否是在队头 最小的到时时间
    auto it = m_timers.insert(p).first;
    bool is_front = (it == m_timers.begin()) && !m_tickled;
    //频繁修改时候 避免总是去唤醒
    if(is_front)
        m_tickled = true;

    lock.unlock();

    //当有比之前更小的 定时器任务插入  就要通知epoll_wait那边去修改为更小的等待时间
    //实际上：将挂起的协程唤醒，重新走一遍流程idle()---->run()--->idle()
    if(is_front)
        onTimerInsertedAtFront();
    
}


//添加条件定时器辅助函数
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{   
    //利用弱指针weak_ptr 来判断条件是否存在
    std::shared_ptr<void> temp = weak_cond.lock();
    if(temp)
    {
        cb();
    }

}

//添加条件定时器
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring)
{
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

//获取队头定时器的到期时间
uint64_t TimerManager::getNextTime()
{
    MutexType::ReadLock lock(m_mutex);
    m_tickled = false;

    if(!m_timers.size())    //没有任务执行返回最大值
        return ~0ull; 

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = GetCurrentMs();
    if(now_ms >= next->m_next)  //现在获取的时间 已经晚于预计要触发的时间点 马上执行
        return 0;
    else    //还没到触发时间点就返回剩余时间间隔
        return next->m_next - now_ms;
}

//已经到期的定时任务集合
void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs)
{
    uint64_t now_ms = GetCurrentMs();
    std::vector<std::shared_ptr<Timer> > expired;

    {
        MutexType::ReadLock lock(m_mutex);
        if(!m_timers.size())
            return;
    }

    MutexType::WriteLock lock(m_mutex);
    if(!m_timers.size())
        return;

    bool changed = checkClockChange(now_ms);
    //服务器时间没有发生改变 且不存在超时定时器 就退出函数
    if(!changed && (*m_timers.begin())->m_next > now_ms)
        return;

    //构造一个装有当前时间的Timer为了应用lower_bound算法函数
    Timer::ptr now_timer(new Timer(now_ms));

    //如果服务器时间发生了变动 就返回end() 会将整个m_timers全部清理 重新加入队列
    //否则 将找出大于等于当前时间的第一个Timer
    auto it = changed ? m_timers.end() : m_timers.lower_bound(now_timer);

    //找到还没到时的第一个定时器的位置后 停下
    while(it != m_timers.end() && (*it)->m_next == now_ms)
        ++it;
    
    //将当前位置之前 到队头 所有已经到时的定时器全部拿出 到expired队列中
    expired.insert(expired.begin(), m_timers.begin(), it);
    //将原队列前部的定时器任务全部移除
    m_timers.erase(m_timers.begin(), it);

    //扩充可执行任务的队列
    cbs.reserve(expired.size());
    for(auto &x : expired)
    {
        cbs.emplace_back(x->m_cb);
        //循环定时器就放回 set中
        if(x->m_recurring)
        {
            x->m_next = now_ms + x->m_ms;
            m_timers.insert(x);
        }
        else
            x->m_cb = nullptr;
        
    }

}


//探测当前服务器时间是否改变  策略有 往前调/往后调
bool TimerManager::checkClockChange(uint64_t now_ms)
{
    bool changed = false;
    //当前时间小于上一次时间且比上一次时间一小时前还要小 就认为服务器时间被修改
    if(now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000))
        changed = true;
    
    m_previousTime = now_ms;
    return changed;
}

//定时器队列是否为空
bool TimerManager::isTimersEmpty()
{
    MutexType::ReadLock lock(m_mutex);
    return m_timers.empty();
}


}