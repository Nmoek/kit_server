#include "iomanager.h"
#include "Log.h"
#include "config.h"
#include "macro.h"
#include "mutex.h"


#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <algorithm>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

//获取具体事件对象
IOManager::FdContext::EventContext& IOManager::FdContext::getEventContext(IOManager::Event event)
{
    switch(event)
    {
        case IOManager::READ: return read_event;break;
        case IOManager::WRITE: return write_event;break;
        default:
            KIT_ASSERT2(false, "getContext error");
    }
    
    throw std::invalid_argument("getContext invalid event!!");
}

//重置事件对象
void IOManager::FdContext::resetEventContext(IOManager::FdContext::EventContext& event_ctx)
{
    event_ctx.scheduler = nullptr;
    event_ctx.coroutine.reset();
    event_ctx.cb = nullptr;
}


//主动触发事件
void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
    KIT_ASSERT(events & event);

    //把触发后的事件去除掉
    events = (Event)(events & ~event);

    EventContext& ctx = getEventContext(event);

    //KIT_ASSERT(ctx.cb || ctx.coroutine);

    if(ctx.cb)
    {
        //KIT_LOG_DEBUG(g_logger) << "触发函数";
        ctx.scheduler->schedule(&ctx.cb);
    }
    else if(ctx.coroutine)
    {
        //KIT_LOG_DEBUG(g_logger) << "触发协程";
        ctx.scheduler->schedule(&ctx.coroutine);
    }
 

    ctx.scheduler = nullptr;
}





IOManager::IOManager(const std::string& name, size_t threads_size, bool use_caller)
    :Scheduler(name, threads_size, use_caller)
{
    //创建epoll句柄
    m_epfd = epoll_create(1);
    if(m_epfd < 0)
    {
        KIT_LOG_ERROR(g_logger) << "IOManager: epoll_create error";
        KIT_ASSERT2(false, "epoll_create error");
    }

    //创建管道句柄
    int ret = pipe(m_tickleFds);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "IOManager: pipe create error";
        KIT_ASSERT2(false, "pipe create error");
    }

    //初始化epoll事件
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));

    //设置为 读事件触发 以及 边缘触发
    event.events = EPOLLIN | EPOLLET;
    // [0]为读管道 [1]为写管道
    event.data.fd = m_tickleFds[0];    

    //设置句柄属性  将读fd 设置为非阻塞
    ret = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "IOManager: fcntl error";
        KIT_ASSERT2(false, "fcntl error");
    }

    //将当前的事件添加到epoll中
    ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "IOManager: epoll_ctl";
        KIT_ASSERT2(false, "epoll_ctl error");
    }

    //默认为32个事件信息
    contextResize(256);

    //初始化后直接启动
    start();
}

IOManager::~IOManager()
{
    stop();
    //把句柄都关闭
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    //删除为事件上下文分配的空间
    for(size_t i = 0;i < m_fdContexts.size();++i)
    {
        if(m_fdContexts[i])
            delete m_fdContexts[i];
    }
}

//句柄对象初始化
void IOManager::contextResize(size_t size)
{
    m_fdContexts.resize(size);

    for(size_t i = 0;i < m_fdContexts.size();++i)
    {
        if(!m_fdContexts[i])
        {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }

}


//添加事件 0成功 -1出错
int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
    FdContext *fd_ctx = nullptr;

    /*拿到对应的 句柄对象  没有就创建*/
    MutexType::ReadLock lock(m_mutex); 
    if((int)m_fdContexts.size() > fd)
    {
        //KIT_LOG_DEBUG(g_logger) << "存在并取出 fd = " << fd;
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }
    else  //事件信息 容器扩容
    {
        lock.unlock();
        
        MutexType::WriteLock lock2(m_mutex);
        
        KIT_LOG_DEBUG(g_logger) << "不存在并扩容 fd = " << fd;
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock _lock(fd_ctx->mutex);

    /*设置句柄对象的信息*/
    //同一个句柄不能在上面加相同的事件
    //如果有这种情况出现 说明有多个线程在操作同一个句柄
    if(KIT_UNLIKELY(fd_ctx->events & event))   
    {
        KIT_LOG_ERROR(g_logger) << "addEvent: event exists, fd= " << fd
            << ", event=" << event
            << ";exist event=" << fd_ctx->events;
        KIT_ASSERT(!(fd_ctx->events & event));
    }

    //判断事件 是需要修改还是新增
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    struct epoll_event ev;
    ev.events = EPOLLET | fd_ctx->events | event;
    ev.data.ptr = fd_ctx;

    //将事件添加/修改到epoll
    int ret = epoll_ctl(m_epfd, op, fd, &ev);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "\naddEvent: epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << ");\n"
        << " error:" << ret << "(" << errno << "," << strerror(errno) << ")"; 

        return -1;
    }

    //待处理事件自增
    ++m_pendingEventCount;

    //将句柄上的事件叠加
    fd_ctx->events = (Event)(fd_ctx->events | event);

    //构建对应的添加的读/写 事件对象 设置相关信息
    //要加 读事件就返回 read_event;要加写事件 就返回write_event
    FdContext::EventContext& event_contex = fd_ctx->getEventContext(event);

    KIT_ASSERT(!event_contex.scheduler && !event_contex.coroutine && !event_contex.cb);

    //设置调度器
    event_contex.scheduler = Scheduler::GetThis();

    //设置回调函数
    if(cb)
        event_contex.cb.swap(cb);
    else    //没有设置回调  下一次就继续执行当前协程
    {

        event_contex.coroutine = Coroutine::GetThis();

        //给事件对象绑定协程时候 协程应该是运行的
        KIT_ASSERT2(event_contex.coroutine->getState() == Coroutine::State::EXEC,  "thread id=" << GetThreadId() << ",coroutine id=" << event_contex.coroutine->getID() << ",state=" << event_contex.coroutine->getState());
    }

    return 0;

}

//为句柄删除事件
bool IOManager::delEvent(int fd, Event event)
{
    MutexType::ReadLock lock(m_mutex);
    //句柄不存在不用删除
    if((int)m_fdContexts.size() <= fd)
    {
        return false;
    }   

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //该句柄上没有对应事件 不用删除
    if(!(fd_ctx->events & event))
    {
        return false;
    }
    
    //取反运算+与运算 就是去掉该事件event
    Event left_events = (Event)(fd_ctx->events & ~event);

    //去掉之后看句柄上还是否有剩余的事件  有就修改epoll 没有了就从epoll删除
    int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;


    //构造epoll_event事件
    struct epoll_event ev;
    ev.events = EPOLLET | left_events;
    ev.data.ptr = fd_ctx;

    //将事件添加/修改到epoll
    int ret = epoll_ctl(m_epfd, op, fd, &ev);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "\ndelEvent: epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << ");\n"
        << " error:" << ret << "(" << errno << "," << strerror(errno) << ")"; 

        return false;
    }


    //更新句柄上的事件
    fd_ctx->events = left_events;

    //待处理事件对象自减
    --m_pendingEventCount;
    
    //把fdContext句柄对象 中旧的 事件对象EventContext拿出来重置
    FdContext::EventContext& event_context = fd_ctx->getEventContext(event);
    fd_ctx->resetEventContext(event_context);

    return true;

}

//取消事件  找到对应事件强制触发执行 不等待条件满足
bool IOManager::cancelEvent(int fd, Event event)
{
    MutexType::ReadLock lock(m_mutex);
    //句柄不存在不用删除
    if((int)m_fdContexts.size() <= fd)
    {
        return false;
    }   

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //该句柄上没有对应事件 不用删除
    if(!(fd_ctx->events & event))
    {
        return false;
    }

    // KIT_LOG_DEBUG(g_logger) << "fd_ctx->events=" << fd_ctx->events;

    //取反运算 + 与运算 就是去掉该事件
    Event left_events = (Event)(fd_ctx->events & ~event);
    
    //去掉之后看句柄上还是否有剩余的事件  有就修改epoll 没有了就从epoll删除
    int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    //构造epoll_event事件
    struct epoll_event ev;
    ev.events = EPOLLET | left_events;
    ev.data.ptr = fd_ctx;

    //将事件添加/修改到epoll
    int ret = epoll_ctl(m_epfd, op, fd, &ev);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "\ndelEvent: epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << ");\n"
        << " error:" << ret << "(" << errno << "," << strerror(errno) << ")"; 

        return false;
    }


    //事件对象 主动触发事件
    fd_ctx->triggerEvent(event);

    //待处理事件对象自减
    --m_pendingEventCount;

    return true;


}

//为句柄取消所有事件
bool IOManager::cancelAll(int fd)
{
    MutexType::ReadLock lock(m_mutex);
    //句柄不存在不用删除
    if((int)m_fdContexts.size() <= fd)
    {
        return false;
    }   

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //该句柄上没有对应事件 不用删除
    if(!fd_ctx->events)
    {
        return false;
    }


    int op = EPOLL_CTL_DEL;

    //构造epoll_event事件
    struct epoll_event ev;
    ev.events = 0;
    ev.data.ptr = fd_ctx;

    //将事件删除到epoll
    int ret = epoll_ctl(m_epfd, op, fd, &ev);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "\ndelEvent: epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << ");\n"
        << " error:" << ret << "(" << errno << "," << strerror(errno) << ")"; 

        return false;
    }

 

    if(fd_ctx->events & READ)
    {

        //事件对象 主动触发事件
        fd_ctx->triggerEvent(READ);

        //待处理事件对象自减
        --m_pendingEventCount;
    }
    
    if(fd_ctx->events & WRITE)
    {

        //事件对象 主动触发事件
        fd_ctx->triggerEvent(WRITE);

        //待处理事件对象自减
        --m_pendingEventCount;

    }

    //将注册事件置0
    KIT_ASSERT(fd_ctx->events == 0);

    return true;


}

//获取当前调度器的指针 这里是不是不太规范？？？ 向下转型。
//这里使用向下转型的目的：是去访问线程局部变量存储的Scheduler调度器的指针 进而访问到实体
IOManager* IOManager::GetThis()
{
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}


//唤醒协程
void IOManager::tickle()
{
    //没有空闲线程就不用发消息了
    if(!isIdleThreads())
        return;

    //写入一个消息 以唤醒
    int ret = write(m_tickleFds[1], "T", 1);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "tickle: write error";

        KIT_ASSERT2(false, "write error");
    }


}

//协程调度终止
bool IOManager::stopping()
{
    return m_pendingEventCount == 0  && 
        isTimersEmpty() && Scheduler::stopping();
}

//协程没有任务可做 要执行epoll监听
void IOManager::idle()
{
    KIT_LOG_DEBUG(g_logger) << "idle start";

    const uint64_t MAX_EVENTS = 256;
    //4个一组 取出已经就绪的IO
    struct epoll_event *events = new epoll_event[MAX_EVENTS];

    //小技巧：借助智能指针的指定析构函数  自动释放数组
    std::shared_ptr<struct epoll_event> evs_sp(events, [](struct epoll_event* p){
        delete[] p;
    });

    while(1)
    {

             
        /*1.如果调度器关闭了 就退出该函数*/
        if(stopping())
        {
        
            KIT_LOG_INFO(g_logger) << "iomanager name= " << getName() << " is stopping, idle func exit";

            return;
            
        }

        //获取下一次定时器的执行时间
        uint64_t next_timeout = getNextTime();


        /*2.通过epoll_wait 带回已经就绪的IO*/
        int n_ready = 0;
        do{
            //最大超时时间 3000ms
            static const int MAXTIMEOUT = 3000;

            //下一次定时器执行时间
            if(next_timeout != ~0ull)
            {
                next_timeout = next_timeout > MAXTIMEOUT ? MAXTIMEOUT : next_timeout;
            }
            else
            {
                next_timeout = MAXTIMEOUT;
            }

            n_ready = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
            //KIT_LOG_DEBUG(g_logger) << "epoll_wait n_ready=" <<  n_ready;
            if(n_ready < 0 && errno == EINTR)
                continue;       //重新尝试等待wait
            else            //拿到了返回epoll_event 或者 已经超时就break
                break;


        }while(1);

        /*3. 检查定时器队列 将所有到时定时器任务进行调度*/
        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if(cbs.size())
        {
            KIT_LOG_DEBUG(g_logger) << "定时器到时调度";
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }


        /*4.依次处理已经就绪的IO*/
        for(int i = 0;i < n_ready;++i)
        {
            struct epoll_event& event = events[i];

            //过滤管道读端被消息唤醒  跳过
            if(event.data.fd == m_tickleFds[0])
            {
                //KIT_LOG_DEBUG(g_logger) << "读管道活跃";
                uint8_t temp[256];
                //循环的目的 把缓冲区全部读干净
                while(read(m_tickleFds[0], temp, sizeof(temp)) > 0);

                continue;
            }

            //处理剩下的真正就绪的IO
            FdContext* fd_ctx = (FdContext*)event.data.ptr;

            FdContext::MutexType::Lock lock(fd_ctx->mutex);

            //如果是错误或者中断 导致的活动  将IO事件置为可读可写
            if(event.events & (EPOLLERR | EPOLLHUP))
            {
                KIT_LOG_DEBUG(g_logger) << "socket error";
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }

            //开一个变量转换 从epoll_event的事件----->自定义的事件Event
            int real_events = Event::NONE;
            if(event.events & EPOLLIN)
            {
                real_events |= READ;
            }

            if(event.events & EPOLLOUT)
            {
                real_events |= WRITE;
            }

            //和当前IO上的事件对比 不符合就说明没有事件触发 跳过
            if((fd_ctx->events & real_events) == Event::NONE)
                continue;
            
            //把下面准备主动触发的事件 去除掉 剩余的事件放回epoll中
            //BUG点:自定义的Event 和 
            int left_events = (fd_ctx->events & ~real_events);
            
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

            //复用event
            event.events = EPOLLET | left_events;
            
            int ret = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(ret < 0)
            {
                KIT_LOG_ERROR(g_logger) << "\nidel: epoll_ctl(" << m_epfd << ", " << op << ", " << fd_ctx->fd << ", " << fd_ctx->events << ");\n"
                << " error:" << ret << "(" << errno << "," << strerror(errno) << ")"; 

                continue;
            }

            //把需要触发的读事件 主动触发
            if(real_events & READ)
            {
                KIT_LOG_DEBUG(g_logger) << "idle 读事件触发";
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }

            //把需要触发的写事件 主动触发
            if(real_events & WRITE)
            {
                KIT_LOG_DEBUG(g_logger) << "idle 写事件触发";
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }

 
            
        }
        

        /*5.处理完就绪的IO  让出当前协程的执行权 到Scheduler::run中去*/
        Coroutine::ptr cur = Coroutine::GetThis();
        auto p = cur.get();
        cur.reset();

        //从这切回 又会 从这切进
        p->swapOut();

    }

}

//实现TimerManger中的纯虚函数 唤醒epol_wait重新设置超时时间
void IOManager::onTimerInsertedAtFront()
{
    //唤醒一下 在epoll_wait的线程
    tickle();
}



}