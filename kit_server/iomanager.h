#ifndef _KIT_IOMANAGER_H_
#define _KIT_IOMANAGER_H_


#include "scheduler.h"
#include "timer.h"
#include <memory>


namespace kit_server
{

/**
 * @brief IO调度器类
 */
class IOManager: public Scheduler, public TimerManager
{
public:
    /**
     * @brief 事件枚举类型 直接对标epoll里的事件赋值
     */
    enum Event{
        NONE =  0x0,    //无事件
        READ =  0x1,    //读事件   EPOLLIN
        WRITE = 0x4     //写事件   EPOLLOUT
    };
    
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex MutexType;

    /**
     * @brief IO调度器类构造函数
     * @param[in] name 调度器名称
     * @param[in] threads_size 初始线程数量
     * @param[in] use_caller 当前线程是否作为调度线程
     */
    IOManager(const std::string& name = "", size_t threads_size = 1, bool use_caller = true);

    /**
     * @brief IO调度器类析构函数
     */
    ~IOManager();

    /**
     * @brief 为句柄添加事件 0成功  -1出错
     * @param[in] fd 给哪一个句柄fd添加
     * @param[in] event 读/写事件
     * @param[in] cb 要执行的回调函数
     * @return 
     *      @retval 0  添加成功
     *      @retval -1 添加失败
     * 
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    
    /**
     * @brief 为句柄删除单个事件
     * @param[in] fd 给哪一个句柄fd删除
     * @param[in] event 读/写事件
     * @return true 删除成功
     * @return false 删除失败
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 为句柄取消单个事件，找到对应事件强制触发执行，不等待条件满足
     * @param[in] fd 给哪一个句柄fd取消
     * @param[in] event 读/写事件
     * @return true 取消并触发成功
     * @return false 取消并触发失败
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief 为句柄取消所有事件，所有事件强制触发执行，不等待条件满足
     * @param[in] fd 给哪一个句柄fd取消
     * @return true 取消并触发成功
     * @return false 取消并触发失败
     */
    bool cancelAll(int fd);

public:
    /**
     * @brief 获取当前线程运行的调度器this指针
     * @return IOManager* 
     */
    static IOManager* GetThis();

protected:
    /**
     * @brief 线程唤醒函数
     */
    void tickle() override;

    /**
     * @brief 调度器停止的判断条件
     * @return true 停止成功
     * @return false 停止失败
     */
    bool stopping() override;

    /**
     * @brief 执行协程空转的回调函数,执行epoll监听
     */
    void idle() override;

    /**
     * @brief 修改句柄对象数量
     * @param[in] size 要修改到的数量
     */

    void contextResize(size_t size);

    /**
     * @brief 定时器队列队头插入对象后进行epoll_wait超时更新
     */
    void onTimerInsertedAtFront() override;

private:
    /**
     * @brief 套接字句柄对象结构体
     */
    struct FdContext
    {
        typedef Mutex MutexType;

        /**
         * @brief 事件对象结构体
         */
        struct EventContext
        {
            ///被调度的调度器
            Scheduler *scheduler = nullptr;
            //事件绑定的协程
            Coroutine::ptr coroutine;
            //事件绑定的函数
            std::function<void()> cb;

        };

        /// 套接字句柄/文件描述符
        int fd = 0; 
        /// 套接字句柄上的读事件对象
        EventContext read_event;
        /// 套接字句柄上的写事件对象
        EventContext write_event;
        /// 套接字句柄上注册好的事件
        Event events = NONE;
        /// 互斥锁
        MutexType mutex;

        /**
         * @brief 获取套接字句柄对象上的事件对象
         * @param[in] event 读/写事件
         * @return EventContext& 
         */
        EventContext& getEventContext(Event event);

        /**
         * @brief 清空套接字句柄对象上的事件对象
         * @param[in] event_ctx 读/写事件对象
         */
        void resetEventContext(struct EventContext& event_ctx);

        /**
         * @brief 主动触发事件，执行事件对象上的回调函数
         * @param[in] event 读/写事件 
         */
        void triggerEvent(Event event);

    };

private:
    /// epoll句柄
    int m_epfd; 
    /// 管道描述符 作为唤醒工具
    int m_tickleFds[2];
    /// 待处理事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    /// 读写锁
    MutexType m_mutex;
    /// 套接字句柄对象指针数组
    std::vector<FdContext*> m_fdContexts;
};

}



#endif