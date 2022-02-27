#ifndef _KIT_TIMER_H_ 
#define _KIT_TIMER_H_


#include <memory>
#include <functional>
#include <set>
#include <vector>

#include "mutex.h"


namespace kit_server
{

class TimerManager;

class Timer: public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    //取消定时器任务
    bool cancel();
    //刷新定时器时间
    bool refresh();
    //重新设定定时器间隔时间
    bool reset(uint64_t ms, bool from_now = true);

private:
    //Timer的构造函数为私有意味着不能显示创建对象  必须由TimerManager来创建
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);

    Timer(uint64_t next);

    //构建可执行对象 用于set比较
    struct Comparator
    {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };



private:
    //间隔多长时间后执行
    uint64_t m_ms = 0;
    //执行的回调函数
    std::function<void()> m_cb;
    //是否是循环定时器
    bool m_recurring = false;
    //精确的定时器超时时间点
    uint64_t m_next = 0;
    //定时器管理对象
    TimerManager* m_manager = nullptr;


};

class TimerManager
{
    friend class Timer;
public:
    typedef RWMutex MutexType;

    TimerManager();
    virtual ~TimerManager();

    //添加定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
    //添加条件定时器
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);
    //获取队头定时器的到期时间
    uint64_t getNextTime();
    //已经到期的定时任务集合
    void listExpiredCb(std::vector<std::function<void()> >& cbs);
    //定时器队列是否为空
    bool isTimersEmpty();

protected:
    //当插入的定时器 在头部要执行这个函数 去唤醒epoll_wait
    //或者允许我们能够做一些额外的操作
    virtual void onTimerInsertedAtFront() = 0;

    void addTimer(Timer::ptr p);

private:
    //探测当前服务器时间是否改变  策略有往前调/往后调
    bool checkClockChange(uint64_t now_ms);

private:
    //读写锁
    MutexType m_mutex;
    //定时器队列
    std::set<Timer::ptr, Timer::Comparator> m_timers; 
    //避免频繁修改的一个ticked标记
    bool m_tickled = false;
    //旧的服务器的时间
    uint64_t m_previousTime = 0;

};

}

#endif