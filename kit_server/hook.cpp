#include "hook.h"
#include "scheduler.h"
#include "iomanager.h"
#include "coroutine.h"
#include "fdmanager.h"
#include "config.h"
#include "macro.h"

#include <sys/types.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <string.h>



namespace kit_server
{

static thread_local bool t_hook_enable = false;
static Logger::ptr g_logger = KIT_LOG_NAME("system");

//约定一个tcp connect 超时配置 5000ms
static ConfigVar<int>::ptr g_tcp_connect_timeout = 
    Config::LookUp("tcp.connect.timeout", 5000, "tcp connect timeout");



#define HOOK_FUNC(XX) \
    XX(sleep)\
    XX(usleep)\
    XX(nanosleep)\
    XX(socket)\
    XX(connect)\
    XX(accept)\
    XX(read)\
    XX(readv)\
    XX(recv)\
    XX(recvfrom)\
    XX(recvmsg)\
    XX(write)\
    XX(writev)\
    XX(send)\
    XX(sendto)\
    XX(sendmsg)\
    XX(close)\
    XX(fcntl)\
    XX(ioctl)\
    XX(setsockopt)\
    XX(getsockopt)



//HOOK初始化
void hook_init()
{
    static bool is_inited = false;

    if(is_inited)
        return;

#define XX(name)    name ## _f = (name ## _func)dlsym(RTLD_NEXT, #name);
    HOOK_FUNC(XX)
#undef XX
}


//connect 超时时间
static uint64_t s_connect_timeout = -1;

//在main() 函数之前执行hook初始化
struct _HookIniter
{
    _HookIniter()
    {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        //为这个超时配置项 绑定一个 修改回调，发送修改时，自动适配
        g_tcp_connect_timeout->addListener([](const int &old_value, const int &new_value){
            KIT_LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value 
                << "to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};

static struct _HookIniter _hook_initer;
// void hook_init() __attribute__((constructor));


//检查当前线程是否被HOOK
bool IsHookEnable()
{
    return t_hook_enable;
}

//设置当前线程hook状态
void SetHookEnable(bool flag)
{
    t_hook_enable = flag;
}


//超时条件结构体
struct timer_info
{
    int canceled = 0;
};


//核心 用一个模板兼容我们要hook的IO操作函数
template<class OriginFunc, typename... Args>
static ssize_t do_io(int fd, OriginFunc func, const char *hook_func_name, 
    uint32_t event, int timeout_so, Args&&... args)
{
    if(!t_hook_enable)
        return func(fd, std::forward<Args>(args)...);
    

    /*1.获取fd对应的FdCtx*/
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);

    //不存在认为不是socket
    if(!ctx)       
        return func(fd, std::forward<Args>(args)...);
    
    //socket 已经关闭
    if(ctx->isClose())
    {
        errno = EBADF;
        return -1;
    }

    //不为socket 且 是用户式非阻塞
    if(!ctx->isSocket() || ctx->getUserNonblock())
    {
        return func(fd, std::forward<Args>(args)...);
    }
    
    //KIT_LOG_DEBUG(g_logger) << "do_io func start hook=" << hook_func_name;
    
    /*2.获取IO的超时时间  设置shared_ptr的超时条件*/
    uint64_t timeout = ctx->getTimeout(timeout_so);
    //设置超时条件
    std::shared_ptr<timer_info> tinfo(new timer_info);

//重试标志位
RETRY:

    /*3.直接执行一次IO操作*/
    ssize_t n = func(fd, std::forward<Args>(args)...);

    // if(strncmp(hook_func_name, "recv", 4) == 0)
    //     KIT_LOG_DEBUG(g_logger) << "recv size:" << n;

    /*(1).IO被中断要重试*/
    while(n == -1 && errno == EINTR)
    {
        n = func(fd, std::forward<Args>(args)...);
    }

    /*(2).socket IO处于阻塞状态了 对它进行hook操作*/
    if(n == -1 && errno == EAGAIN)
    {

        IOManager* iom = IOManager::GetThis();
        //添加条件定时器
        Timer::ptr timer;
        //使用弱指针包装条件
        std::weak_ptr<timer_info> winfo(tinfo);


        /*a.不为-1(立即返回) 根据超时时间添加条件定时器*/
        if(timeout != (uint64_t)-1)
        {
            timer = iom->addConditionTimer(timeout, [winfo, fd, iom, event](){
                auto t = winfo.lock();
                //条件失效 或 被取消了
                if(!t || t->canceled)
                    return;
                t->canceled = ETIMEDOUT;
                //取消事件强制执行
                iom->cancelEvent(fd, (IOManager::Event)event);

            }, winfo);
        }


        /*b.为对应的fd添加异步回调 */
        int ret = iom->addEvent(fd, (IOManager::Event)event);
        if(KIT_UNLIKELY(ret < 0))
        {
            // if(c)
            //     KIT_LOG_ERROR(g_logger) << hook_func_name << " do_io: addEvent(" << fd << "," << event << ")" << "error, again count = " << again_count << ", used time=" << GetCurrentUs() - now;

            KIT_LOG_ERROR(g_logger) <<  hook_func_name << " do_io: addEvent("  << fd << "," << event << ")" << " error";
            
            //添加事件失败就 取消定时器
            if(timer)
                timer->cancel();

            return -1;
        }


        /*c.挂起当前协程*/
        Coroutine::YieldToHold();
        /*切回来的两个时间点：
            1.IO没有数据到达，条件定时器超时强制唤醒 
            2.定时器还没有超时，IO活跃有数据到达触发回调
        */

        //先把定时器 取消
        if(timer) 
            timer->cancel();
        
    
        //超时条件不空 说明是由定时器超时切回的
        if(tinfo->canceled) //释放了就是0
        {
            errno = tinfo->canceled;
            return -1;
        }
        
        //再一次进行IO操作
        goto RETRY;
        
    }

    return n;
    
}


}


/*----------------------------------------------------------*/

using namespace kit_server;

extern "C"
{

#define XX(name) name ## _func name ## _f = nullptr;
    HOOK_FUNC(XX)
#undef XX



/***********************************sleep***********************************/

unsigned int sleep(unsigned int secends)
{
    //如果当前线程没有被hook就返回旧的系统调用执行
    if(!kit_server::t_hook_enable)
        return sleep_f(secends);
    
    kit_server::Coroutine::ptr cor = kit_server::Coroutine::GetThis();
    kit_server::IOManager* iom = kit_server::IOManager::GetThis();

    //bind 模板函数时候 需要把对应的参数类型设定好 默认参数也需要显示指定
    iom->addTimer(secends * 1000, std::bind((void(kit_server::Scheduler::*)(kit_server::Coroutine::ptr, int))&kit_server::IOManager::schedule, iom, cor, -1));

    // iom->addTimer(secends * 1000, [iom, cor](){
    //     iom->schedule(cor);
    // });

    kit_server::Coroutine::YieldToHold();

    return 0;
}

int usleep(useconds_t usec)
{
    //如果当前线程没有被hook就返回旧的系统调用执行
    if(!kit_server::t_hook_enable)
        return usleep_f(usec);
    
    kit_server::Coroutine::ptr cor = kit_server::Coroutine::GetThis();
    kit_server::IOManager* iom = kit_server::IOManager::GetThis();

    iom->addTimer(usec / 1000, std::bind((void(kit_server::Scheduler::*)(kit_server::Coroutine::ptr, int))&kit_server::IOManager::schedule, iom, cor, -1));

    // iom->addTimer(usec / 1000, [iom, cor](){
    //     iom->schedule(cor);
    // });

    kit_server::Coroutine::YieldToHold();

    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if(!kit_server::t_hook_enable)
        return nanosleep_f(req, rem);
    

    kit_server::Coroutine::ptr cor = kit_server::Coroutine::GetThis();
    kit_server::IOManager* iom = kit_server::IOManager::GetThis();

    uint64_t ms = 0;
    if(errno == EINTR)
    {
        if(rem != nullptr)
            ms = rem->tv_sec * 1000 + rem->tv_nsec / 1000 / 1000; 
    }
    else 
        ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;

    iom->addTimer(ms, std::bind((void(kit_server::Scheduler::*)(kit_server::Coroutine::ptr, int))&kit_server::IOManager::schedule, iom, cor, -1));

    // iom->addTimer(ms, [iom, cor](){
    //     iom->schedule(cor);
    // });

    kit_server::Coroutine::YieldToHold();

    return 0;
}



/***********************************scoket***********************************/
//socket
int socket(int domain, int type, int protocol)
{
    if(!kit_server::t_hook_enable)
        return socket_f(domain, type, protocol);

    KIT_LOG_DEBUG(g_logger) << "hook socket start";
    int fd = socket_f(domain, type, protocol);
    if(fd < 0)
        return fd;
    
    //由FdManager创建一个fd
    kit_server::FdMgr::GetInstance()->get(fd, true);

    return fd;
    
}


//带超时功能的connect
int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms)
{
    if(!kit_server::t_hook_enable)
        return connect_f(sockfd, addr, addrlen);
    
    KIT_LOG_DEBUG(g_logger) << "hook connect start";

    kit_server::FdCtx::ptr ctx = kit_server::FdMgr::GetInstance()->get(sockfd);
    if(!ctx || ctx->isClose())
    {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket())
        return connect_f(sockfd, addr, addrlen);
    
    if(ctx->getUserNonblock())
        return connect_f(sockfd, addr, addrlen);
    

    //创建socket时候已经设置为 非阻塞的 因此这里不会阻塞
    int n = connect_f(sockfd, addr, addrlen);
    if(n == 0)
    {
        return 0;
    }
    else if(n != -1 || errno != EINPROGRESS)
    {
        return n;
    }
    
    kit_server::IOManager* iom = kit_server::IOManager::GetThis();
    kit_server::Timer::ptr timer;
    std::shared_ptr<kit_server::timer_info> tinfo(new kit_server::timer_info);
    std::weak_ptr<kit_server::timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1)
    {

        timer = iom->addConditionTimer(timeout_ms, [iom, winfo, sockfd](){
            auto t = winfo.lock();
            if(!t || t->canceled)
                return;
            
            t->canceled = ETIMEDOUT;
            iom->cancelEvent(sockfd, kit_server::IOManager::Event::WRITE);

        }, winfo);
    }

    //添加写事件是因为 connect成功后马上可写
    int ret = iom->addEvent(sockfd, kit_server::IOManager::Event::WRITE);
    if(ret == 0)    //如果添加写事件成功
    {
        //切出
        kit_server::Coroutine::YieldToHold();

        //切回
        if(timer)
            timer->cancel();

        if(tinfo->canceled)
        {
            errno = tinfo->canceled;
            return -1;
        }
    }
    else
    {
        if(timer)
            timer->cancel();
        
        KIT_LOG_ERROR(g_logger) << "connect: addEvent(" << sockfd << ", WRITE) error=" << errno << "is:" << strerror(errno); 
    }

    //协程切回后 检查一下socket上是否有错误  才能最终判断连接是否建立
    int m_error = 0;
    socklen_t len = sizeof(int);

    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &m_error, &len) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "connect with timeout getsockopt error";
        return -1;
    }

    //检测sockfd 上是否有错误发生 有就通过int error变量带回
    //没有错误才是真的建立连接成功
    errno = m_error;
    if(!m_error)
    {
        return 0;
    }
    else 
    {

        return -1;
    }

}


//connect  较为复杂！
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return connect_with_timeout(sockfd, addr, addrlen, kit_server::s_connect_timeout);
}


//accept
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int fd = do_io(sockfd, accept_f, "accept", kit_server::IOManager::Event::READ, 
        SO_RCVTIMEO, addr, addrlen);

    if(fd >= 0)
    {
        //把新建立的通信套接字加入到FdManager中去管理
        kit_server::FdMgr::GetInstance()->get(fd, true);
    }

    return fd;
}


//close 要做一些清理工作
int close(int fd)
{
    if(!kit_server::t_hook_enable)
        return close_f(fd);
    

    KIT_LOG_DEBUG(g_logger) << "hook close start";

    kit_server::FdCtx::ptr ctx = kit_server::FdMgr::GetInstance()->get(fd);


    //如果是socket 
    if(ctx)
    {
        //KIT_LOG_DEBUG(g_logger) << "get ctx fd=" << ctx->getFd();
        auto iom = kit_server::IOManager::GetThis();
        if(iom)
        {
            iom->cancelAll(fd);
        }

        kit_server::FdMgr::GetInstance()->del(fd);
    }

    return close_f(fd);
}


//fcntl  用户级nonblock
int fcntl(int fd, int cmd, ... /* arg */ )
{
    va_list va;
    va_start(va, cmd);
    switch (cmd)
    {
    //设置文件状态
    case F_SETFL:
    {
        int arg = va_arg(va, int);
        va_end(va);

        kit_server::FdCtx::ptr ctx = kit_server::FdMgr::GetInstance()->get(fd);
        if(!ctx || ctx->isClose() || !ctx->isSocket())
            return fcntl_f(fd, cmd, arg);
        
        ctx->setUserNonblock(arg & O_NONBLOCK);

        if(ctx->getSysNonblock())
        {
            arg |= O_NONBLOCK;
        }
        else
        {
            arg &= ~O_NONBLOCK;
        }

        return fcntl_f(fd, cmd, arg);
    }
    break;

    //获取文件状态
    case F_GETFL:
    {
        va_end(va);
        int arg = fcntl_f(fd, cmd);
        kit_server::FdCtx::ptr ctx = kit_server::FdMgr::GetInstance()->get(fd);
        if(!ctx || ctx->isClose() || !ctx->isSocket())
            return arg;

        if(ctx->getUserNonblock())
        {
            arg |= O_NONBLOCK;
        }
        else
        {
            arg &= ~O_NONBLOCK;
        }

        return arg;
        
    }
    break;
    /*int*/
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
    case F_SETPIPE_SZ:
    {
        int arg = va_arg(va, int);
        va_end(va);
        return fcntl_f(fd, cmd, arg);

    }
    break;
    /*void*/
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
    case F_GETPIPE_SZ:
    {
        va_end(va);
        return fcntl_f(fd, cmd);
    }
    break;

    /*锁*/
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:
    case F_OFD_SETLK:
    case F_OFD_SETLKW:
    case F_OFD_GETLK:
    {
        struct flock* arg = va_arg(va, struct flock*);
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    }
    break;

    /*进程组*/
    case F_GETOWN_EX:
    case F_SETOWN_EX:
    {
        struct f_owner_ex* arg = va_arg(va, struct f_owner_ex*);
        va_end(va);
        return fcntl_f(fd, cmd, arg);
    }
    break;

    default:
        va_end(va);
        return fcntl_f(fd, cmd);
    }
}

//ioctl
int ioctl(int fd, unsigned long request, ...)
{
    va_list va;
    va_start(va, request);
    void *arg = va_arg(va, void *);
    va_end(va);

    if(FIONBIO == request)
    {
        bool user_nonblock = *(int*)arg;

        kit_server::FdCtx::ptr ctx = kit_server::FdMgr::GetInstance()->get(fd);

        if(!ctx || ctx->isClose() || !ctx->isSocket())
            return ioctl_f(fd, request, arg);     

        ctx->setUserNonblock(user_nonblock);

    }

    return ioctl_f(fd, request, arg);
}

//getsockopt 不需要hook
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

//setsockopt
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    if(!kit_server::t_hook_enable)
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    
    if(level == SOL_SOCKET)
    {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
        {
            kit_server::FdCtx::ptr ctx = kit_server::FdMgr::GetInstance()->get(sockfd);
            if(ctx)
            {
                const struct timeval *tv = (const struct timeval*)(optval);
                ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }

    return setsockopt_f(sockfd, level, optname, optval, optlen);
}


/***********************************read***********************************/
//read
ssize_t read(int fd, void *buf, size_t count)
{
    return do_io(fd, read_f, "read", kit_server::IOManager::Event::READ, 
        SO_RCVTIMEO, buf, count);
}


//readv
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    return do_io(fd, readv_f, "readv", kit_server::IOManager::Event::READ, 
        SO_RCVTIMEO, iov, iovcnt);
}

//recv
ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    return do_io(sockfd, recv_f, "recv", kit_server::IOManager::Event::READ, 
        SO_RCVTIMEO, buf, len, flags);
}


//recvfrom
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, 
    struct sockaddr *src_addr, socklen_t *addrlen)
{
    return do_io(sockfd, recvfrom_f, "recvfrom", kit_server::IOManager::Event::READ, 
        SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}  

//recvmsg
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    return do_io(sockfd, recvmsg_f, "recvmsg", kit_server::IOManager::Event::READ, 
        SO_RCVTIMEO, msg, flags);
}

/***********************************write***********************************/
//write
ssize_t write(int fd, const void *buf, size_t count)
{
    return do_io(fd, write_f, "write", kit_server::IOManager::Event::WRITE, 
        SO_SNDTIMEO, buf, count);
}


//writev
ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    return do_io(fd, writev_f, "writev", kit_server::IOManager::Event::WRITE, 
        SO_SNDTIMEO, iov, iovcnt);
}

//send
ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
    return do_io(sockfd, send_f, "send", kit_server::IOManager::Event::WRITE, 
        SO_SNDTIMEO, buf, len, flags);
}

//sendto
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen)
{
    return do_io(sockfd, sendto_f, "sendto", kit_server::IOManager::Event::WRITE, 
        SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}
 
//sendmsg
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
    return do_io(sockfd, sendmsg_f, "sendmsg", kit_server::IOManager::Event::WRITE, 
        SO_SNDTIMEO, msg, flags);
}


}