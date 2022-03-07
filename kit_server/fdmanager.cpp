#include "fdmanager.h"
#include "Log.h"
#include "hook.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 

namespace kit_server
{
static Logger::ptr g_logger = KIT_LOG_NAME("system");

/********************************FdCtx***************************************/

FdCtx::FdCtx(int fd)
    :m_isInit(false),
     m_isSocket(false),
     m_userNonblock(false),
     m_sysNonblock(false),
     m_isClosed(false),
     m_fd(fd),
     m_recvTimeout(-1),
     m_sendTimeout(-1)
{
    init();
}

FdCtx::~FdCtx()
{
}

//句柄初始化 判断是否是socket  是就设置为非阻塞
bool FdCtx::init()
{
    //句柄初始化过了 就返回
    if(m_isInit)
        return true;
    
    //struct stat 获取当前系统文件句柄的状态
    struct stat fd_stat;
    if(fstat(m_fd, &fd_stat) < 0)
    {
        m_isInit = false;
        m_isSocket = false;
        KIT_LOG_ERROR(g_logger) << "FdCtx init(): fstat() error";

    }
    else
    {
        m_isInit = true;
        //取出状态位 判断句柄类型
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    
    //如果是socket 句柄 设置为非阻塞
    if(m_isSocket)
    {
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        //如果句柄阻塞 要设置为非阻塞
        if(!(flags & O_NONBLOCK))
        {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }

        m_sysNonblock = true;
    }
    else
    {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    
    return m_isInit;

}


//设置socket 超时时间
void FdCtx::setTimeout(int type, uint64_t time)
{
    if(type == SO_RCVTIMEO)
        m_recvTimeout = time;
    else 
        m_sendTimeout = time;
}

//获取socket 超时时间
uint64_t FdCtx::getTimeout(int type)
{
    if(type == SO_RCVTIMEO)
        return m_recvTimeout;
    else 
        return m_sendTimeout;
}

/********************************FdManager***************************************/


FdManager::FdManager()
{
    m_fds.resize(64);
}

//获取句柄  不存在就创建
FdCtx::ptr FdManager::get(int fd, bool auto_create)
{
    if(fd == -1)
        return nullptr;
        
    //加读锁
    MutexType::ReadLock lock(m_mutex);

    //如果fd越界说明容量不够 
    if((int)m_fds.size() <= fd)
    {
        if(!auto_create)
            return nullptr;

    }
    else    //fd没有越界
    {
        if(m_fds[fd] || !auto_create)
            return m_fds[fd];

    }
    lock.unlock();

    //加写锁
    MutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd >= (int)m_fds.size())
        m_fds.resize(fd * 1.5);

    m_fds[fd] = ctx;
    return ctx;
    
}

//删除句柄
void FdManager::del(int fd)
{
    MutexType::WriteLock lock(m_mutex);

    if((int)m_fds.size() <= fd)
        return;

    m_fds[fd].reset();

}


}