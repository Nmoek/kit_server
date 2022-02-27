#ifndef _KIT_IOMANAGER_H_
#define _KIT_IOMANAGER_H_


#include "scheduler.h"
#include "timer.h"
#include <memory>


namespace kit_server
{

class IOManager: public Scheduler, public TimerManager
{
public:
    // 直接对标 EPOLL里的事件赋值
    enum Event{
        NONE =  0x0,    //无事件
        READ =  0x1,    //读事件   EPOLLIN
        WRITE = 0x4     //写事件   EPOLLOUT
    };
    
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex MutexType;

    IOManager(const std::string& name = "", size_t threads_size = 1, bool use_caller = true);

    ~IOManager();

    //为句柄添加事件 0成功  -1出错
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    
    //为句柄删除事件
    bool delEvent(int fd, Event event);

    //为句柄取消事件 找到对应事件强制触发执行 不等待条件满足
    bool cancelEvent(int fd, Event event);

    //为句柄取消所有事件
    bool cancelAll(int fd);

public:
    //获取当前调度器的指针
    static IOManager * GetThis();


protected:
    //唤醒协程
    void tickle() override;

    //协程调度终止
    bool stopping() override;

    //协程没有任务可做 要执行epoll监听
    void idle() override;

    //句柄对象初始化
    void contextResize(size_t size);

    //实现TimerManger中的纯虚函数 唤醒epol_wait重新设置超时时间
    void onTimerInsertedAtFront() override;

private:
    //句柄对象
    struct FdContext
    {
        //互斥锁  锁句柄资源
        typedef Mutex MutexType;

        //事件对象
        struct EventContext
        {
            //事件所属的调度器
            Scheduler *scheduler = nullptr;
            /*以下二选一供任务调度使用*/
            //事件所属协程
            Coroutine::ptr coroutine;
            //事件所属回调函数
            std::function<void()> cb;

        };

        //句柄/文件描述符
        int fd = 0; 
        //句柄上的读事件对象
        EventContext read_event;
        //句柄上的写事件对象
        EventContext write_event;
        //句柄上人为注册好的事件
        Event events = NONE;
        //互斥锁
        MutexType mutex;

        //获取句柄对象上的事件对象
        EventContext& getEventContext(Event event);

        //重置事件对象
        void resetEventContext(struct EventContext& event_ctx);

        //主动触发事件
        void triggerEvent(Event event);

    };

private:
    //epoll句柄
    int m_epfd; 
    //管道描述符 收发消息使用
    int m_tickleFds[2];
    //待办事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    //读写锁
    MutexType m_mutex;
    //句柄对象指针数组
    std::vector<FdContext*> m_fdContexts;
};

}



#endif