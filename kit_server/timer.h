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

/**
 * @brief 定时器类
 */
class Timer: public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    //取消定时器任务
    /**
     * @brief 取消定时器
     * @return true 取消成功
     * @return false 取消失败
     */
    bool cancel();

    /**
     * @brief 刷新定时器时间
     * @return true 刷新成功
     * @return false 刷新失败
     */
    bool refresh();

    /**
     * @brief 重新设定定时器间隔时间
     * @param[in] ms 间隔时间
     * @param[in] from_now 是否从当前时间点开始重新计时 默认是
     * @return true 重置成功
     * @return false 重置失败
     */
    bool reset(uint64_t ms, bool from_now = true);

private:
    //Timer的构造函数为私有意味着不能显式创建对象  必须由TimerManager来创建
    /**
     * @brief 定时器类构造函数 不能显式创建对象
     * @param[in] ms 间隔时长
     * @param[in] cb 执行的回调函数
     * @param[in] recurring 是否是循环定时器
     * @param[in] manager 哪一个定时器管理类
     */
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);

    /**
     * @brief 定时器类构造函数 不能显式创建对象
     * @param[in] next 到期时间点
     */
    Timer(uint64_t next);

    /**
     * @brief 构建仿函数 用于set比较
     */
    struct Comparator
    {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };

private:
    /// 定时器间隔时长
    uint64_t m_ms = 0;
    /// 执行的回调函数
    std::function<void()> m_cb;
    /// 当前定时器是否是循环定时器
    bool m_recurring = false;
    /// 精确的定时器超时时间点
    uint64_t m_next = 0;
    /// 定时器管理类指针
    TimerManager* m_manager = nullptr;

};

/**
 * @brief 定时器管理类
 */
class TimerManager
{
    friend class Timer;
public:
    typedef RWMutex MutexType;

    /**
     * @brief 定时器管理类构造函数
     */
    TimerManager();

    /**
     * @brief 定时器管理类析构函数
     */
    virtual ~TimerManager();

    /**
     * @brief 添加定时器
     * @param[in] ms 间隔时长
     * @param[in] cb 执行的回调函数
     * @param[in] recurring 是否是循环定时器 默认不是 
     * @return Timer::ptr 添加完成后会将创建的定时器返回
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /**
     * @brief 添加条件定时器
     * @param[in] ms 间隔时长
     * @param[in] cb 执行的回调函数
     * @param[in] weak_cond 判断条件是否还存在的弱指针
     * @param[in] recurring 是否是循环定时器 默认不是 
     * @return Timer::ptr 添加完成后会将创建的定时器返回
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

    /**
     * @brief 获取队头定时器的到期时间点
     * @return uint64_t 
     */
    uint64_t getNextTime();

    /**
     * @brief 找出已经到期的定时任务集合
     * @param[out] cbs 存储全部已经到期的任务
     */
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    /**
     * @brief 定时器队列是否为空
     * @return true 空
     * @return false 不为空
     */
    bool isTimersEmpty();

protected:
    /**
     * @brief 插入的定时器 在头部要执行这个函数 去唤醒epoll_wait
     */
    virtual void onTimerInsertedAtFront() = 0;

    /**
     * @brief 以智能指针形式 添加条件定时器
     * @param[in] p 定时器智能指针
     */
    void addTimer(Timer::ptr p);

private:
    /**
     * @brief 探测当前服务器时间是否改变 
     * @param[in] now_ms 当前的时间点 
     * @return true 已经被修改
     * @return false 没有被修改
     */
    bool checkClockChange(uint64_t now_ms);

private:
    /// 读写锁
    MutexType m_mutex;
    /// 定时器队列
    std::set<Timer::ptr, Timer::Comparator> m_timers; 
    /// 避免频繁唤醒的一个标识
    bool m_tickled = false;
    /// 上一次修改前的服务器的时间
    uint64_t m_previousTime = 0;

};

}

#endif