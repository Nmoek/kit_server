#include "socket.h"
#include "Log.h"
#include "iomanager.h"
#include "fdmanager.h"
#include "hook.h"
#include "macro.h"

#include <errno.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>


namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

//使用传入的地址 创建TCP 套接字
Socket::ptr Socket::CreateTCP(Address::ptr address)
{
    Socket::ptr sock(new Socket(address->getFamily(), Type::TCP, 0));
    return sock;
}

//使用传入的地址 创建UDP 套接字
Socket::ptr Socket::CreateUDP(Address::ptr address)
{
    Socket::ptr sock(new Socket(address->getFamily(), Type::UDP, 0));
    sock->newSocket();  
    sock->m_isConnectd = true;
    return sock;
}

//默认创建TCP 套接字 IPv4
Socket::ptr Socket::CreateTCPSocket()
{
    Socket::ptr sock(new Socket(Family::IPv4, Type::TCP, 0));
    return sock;
}

//默认创建UDP 套接字 IPv4
Socket::ptr Socket::CreateUDPSocket()
{
    Socket::ptr sock(new Socket(Family::IPv4, Type::UDP, 0));
    sock->newSocket();  
    sock->m_isConnectd = true;
    return sock;
}

//默认创建TCP 套接字 IPv6
Socket::ptr Socket::CreateTCPSocket6()
{
    Socket::ptr sock(new Socket(Family::IPv6, Type::TCP, 0));
    return sock;
}

//默认创建UDP 套接字 IPv6
Socket::ptr Socket::CreateUDPSocket6()
{
    Socket::ptr sock(new Socket(Family::IPv6, Type::UDP, 0));
    sock->newSocket();  
    sock->m_isConnectd = true;
    return sock;
}

//默认创建TCP 套接字 Unix域
Socket::ptr Socket::CreateUnixTCPSocket()
{
    Socket::ptr sock(new Socket(Family::Unix, Type::TCP, 0));
    return sock;
}

//默认创建UDP 套接字 Unix域
Socket::ptr Socket::CreateUnixUDPSocket()
{
    Socket::ptr sock(new Socket(Family::Unix, Type::UDP, 0));
    return sock;
}


Socket::Socket(int family, int type, int protocol)
    :m_fd(-1),
     m_family(family),
     m_type(type),
     m_protocol(protocol),
     m_isConnectd(false)
{

}

Socket::~Socket()
{
    close();
}

bool Socket::init(int fd)
{
    //新的fd加入到FdManager的管理中
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);

    //fd为socket 并且没被关闭 对其进行初始化
    if(ctx && ctx->isSocket() && !ctx->isClose())
    {
        m_fd = fd;
        m_isConnectd = true;
        initSocket();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }

    return false;
}

//获取发送超时时间
int64_t Socket::getSendTimeout() const
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_fd);

    if(ctx)
        return  ctx->getTimeout(SO_SNDTIMEO);
    
    return -1;
}


//设置发送超时时间
void Socket::setSendTimeout(int64_t val)
{
    struct timeval tv;
    tv.tv_sec = val / 1000;
    tv.tv_usec = val % 1000 * 1000;

    //设置套接字属性
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);

}

//获取接收超时时间
int64_t Socket::getRecvTimeout() const
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_fd);

    if(ctx)
        return  ctx->getTimeout(SO_RCVTIMEO);
    
    return -1;

}

//设置接收超时时间
void Socket::setRecvTimeout(int64_t val)
{
    struct timeval tv;
    tv.tv_sec = val / 1000;
    tv.tv_usec = val % 1000 * 1000;

    //设置套接字属性
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

//获取配置 利用HOOK方法
bool Socket::getOption(int level, int option, void* result, size_t* len)
{
    int ret = getsockopt(m_fd, level, option, result, (socklen_t*)len);
    if(ret)
    {
        KIT_LOG_ERROR(g_logger) << "getsockopt errno=" << errno 
            << ", is:" << strerror(errno)
            << ", fd=" << m_fd
            << ", level=" << level
            << ", option=" << option;
        return false;
    }
    
    return true;

}

//设置配置
bool Socket::setOption(int level, int option, const void* result, size_t len)
{
    int ret = setsockopt(m_fd, level, option, result, (socklen_t)len);
    if(ret)
    {
        KIT_LOG_ERROR(g_logger) << "setsockopt errno=" << errno 
            << ", is:" << strerror(errno)
            << ", fd=" << m_fd
            << ", level=" << level
            << ", option=" << option;
        return false;
    }
    
    return true;
}

//accept API
Socket::ptr Socket::accept()
{
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));

    int ac_fd = ::accept(m_fd, nullptr, nullptr);
    if(ac_fd < 0)
    {
        KIT_LOG_ERROR(g_logger) << "accept errno =" << errno << ", is:" <<strerror(errno);

        return nullptr;
    }

    if(sock->init(ac_fd))
        return sock;
    
    return nullptr;
}

//bind API
bool Socket::bind(const Address::ptr addr)
{
    //如果套接字无效
    if(KIT_UNLIKELY(!isValid()))
    {
        //创建新的套接字
        newSocket();
        if(KIT_UNLIKELY(!isValid()))
            return false;
    }

    if(KIT_UNLIKELY(addr->getFamily() != m_family))
    {
        KIT_LOG_ERROR(g_logger) << "bind socket family =" << m_family << ", addr family="
            << addr->getFamily() << ", is defferent!";
        return false;
    }

    //bind 没有被HOOK
    int ret = ::bind(m_fd, (struct sockaddr*)addr->getAddr(), addr->getAddrLen());
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "bind error, errno=" << errno << ", is:" << strerror(errno);
        return false;
    }

    //记录一下已经绑定好的本地地址 一般是服务器在bind
    getLocalAddress();
    return true;
}

//connect API 带超时
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
{
    if(KIT_UNLIKELY(!isValid()))
    {
        //创建新的套接字
        newSocket();
        if(KIT_UNLIKELY(!isValid()))
        {
            return false;
        }
    }

    
    if(KIT_UNLIKELY(addr->getFamily() != m_family))
    {
        KIT_LOG_ERROR(g_logger) << "connect socket family =" << m_family << ", addr family="
            << addr->getFamily() << ", is defferent!";
        return false;
    }

    if(timeout_ms == (uint64_t)-1)
    {
        //会使用默认超时时间
        int ret = ::connect(m_fd, addr->getAddr(), addr->getAddrLen());
        if(ret < 0)
        {
            KIT_LOG_ERROR(g_logger) << "connect error, errno=" << errno << ", is:" << strerror(errno);
            close();
            return false;
        }
    }
    else    //使用超时connect
    {
        int ret = ::connect_with_timeout(m_fd, addr->getAddr(), addr->getAddrLen(), timeout_ms);
        if(ret < 0)
        {
            KIT_LOG_ERROR(g_logger) << "connect error, errno=" << errno << ", is:" << strerror(errno);
            close();
            return false;
        }
    }

    //连接状态置为已连接
    m_isConnectd = true;
    //获取远端地址
    getRemoteAddress();
    //获取本地地址
    getLocalAddress();
    return true;
}

//listen API
bool Socket::listen(int backlog)
{
    if(KIT_UNLIKELY(!isValid()))
    {
        KIT_LOG_ERROR(g_logger) << "listen error sock fd = -1";
        return false;
    }

    if(::listen(m_fd, backlog) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "listen error, errno =" << errno << ",is:" << strerror(errno);
        return false;
    }

    return true;
}

//close API
bool Socket::close()
{
    if(!isConnected() && m_fd == -1)
    {
        return true;
    }

    m_isConnectd = false;
    if(m_fd != -1)
    {
        ::close(m_fd);
        m_fd = -1;
    }

    return false;
}

//send API
int Socket::send(const void *buffer, size_t len, int flags)
{
    if(isConnected())
    {
        return ::send(m_fd, buffer, len, flags);
    }

    return -1;
}

int Socket::send(const struct iovec* buffers, size_t len, int flags)
{
    if(isConnected())
    {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (struct iovec *)buffers;
        msg.msg_iovlen = len;
        return ::sendmsg(m_fd, &msg, flags);
    }

    return -1;
}

int Socket::sendTo(const void * buffer, size_t len, const Address::ptr addr_to, int flags)
{
    if(isConnected())
    {
        return ::sendto(m_fd, buffer, len, flags, addr_to->getAddr(), addr_to->getAddrLen());
    }

    return -1;
}

int Socket::sendTo(const struct iovec* buffers, size_t len, const Address::ptr addr_to, int flags)
{
    if(isConnected())
    {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (struct iovec *)buffers;
        msg.msg_iovlen = len;
        msg.msg_name = addr_to->getAddr();
        msg.msg_namelen = addr_to->getAddrLen();
        return ::sendmsg(m_fd, &msg, flags);
    }

    return -1;
}


//recv API
int Socket::recv(void *buffer, size_t len, int flags)
{
    if(isConnected())
    { 
        return ::recv(m_fd, buffer, len, flags);
    }

    return -1;
}

int Socket::recv(struct iovec* buffers, size_t len, int flags)
{
    if(isConnected())
    {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buffers;
        msg.msg_iovlen = len;
        return ::recvmsg(m_fd, &msg, flags);
    }

    return -1;
}

int Socket::recvFrom(void * buffer, size_t len, Address::ptr addr_from, int flags)
{
    if(isConnected())
    {
        socklen_t len = addr_from->getAddrLen();
        return ::recvfrom(m_fd, buffer, len, flags, (struct sockaddr*)addr_from->getAddr(), &len);
    }

    return -1;
}

int Socket::recvFrom(struct iovec* buffers, size_t len, Address::ptr addr_from, int flags)
{
    if(isConnected())
    {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (struct iovec *)buffers;
        msg.msg_iovlen = len;
        msg.msg_name = addr_from->getAddr();
        msg.msg_namelen = addr_from->getAddrLen();
        return ::recvmsg(m_fd, &msg, flags);
    }

    return -1;
}

//获取远端地址
Address::ptr Socket::getRemoteAddress()
{
    if(m_remoteAddr)
        return m_remoteAddr;
    
    Address::ptr result;
    switch(m_family)
    {
        case AF_INET: result.reset(new IPv4Address);break;
        case AF_INET6: result.reset(new IPv6Address);break;
        case AF_UNIX: result.reset(new UnixAddress);break;
        default: result.reset(new UnkonwAddress(m_family));break;
    }
    socklen_t new_len = result->getAddrLen();
    if(getpeername(m_fd, (struct sockaddr*)result->getAddr(), &new_len) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "getpeername error, errno=" << errno << ", is:" << strerror(errno);

        return Address::ptr(new UnkonwAddress(m_family));
    }
    
    //如果为Unix域通信地址需要单独设置一下结构体长度
    if(m_family == AF_UNIX)
    {
        UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(result);
        uaddr->setAddrLen(new_len);
    }

    m_remoteAddr = result;
    return m_remoteAddr;
}

//获取本地地址
Address::ptr Socket::getLocalAddress()
{
    if(m_localAddr)
        return m_localAddr;
    
    Address::ptr result;
    switch(m_family)
    {
        case AF_INET: result.reset(new IPv4Address);break;
        case AF_INET6: result.reset(new IPv6Address);break;
        case AF_UNIX: result.reset(new UnixAddress);break;
        default: result.reset(new UnkonwAddress(m_family));break;
    }

    socklen_t new_len = result->getAddrLen();
    if(getsockname(m_fd, (struct sockaddr*)result->getAddr(), &new_len) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "getsockname error, errno=" << errno << ", is:" << strerror(errno);

        return Address::ptr(new UnkonwAddress(m_family));
    }
    
    if(m_family == AF_UNIX)
    {
        UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(result);
        uaddr->setAddrLen(new_len);
    }

    
    m_localAddr = result;
    return m_localAddr;
}


//是否有效
bool Socket::isValid()
{
    return m_fd != -1;
}


//获取套接字上的错误
int Socket::getError()
{
    int error = 0;
    size_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
    {
        return -1;
    }

    return error;
}

//输出
std::ostream& Socket::dump(std::ostream& os) const
{
    os << "[Socket fd=" << m_fd 
        << ",connected=" << m_isConnectd
        << ",family=" << m_family
        << ",type=" << m_type
        << ",protocol=" << m_protocol;
    
    if(m_localAddr)
        os << ",local addr=" << m_localAddr->toString();
    if(m_remoteAddr)
        os << ", remote addr=" << m_remoteAddr->toString();
    
    os << "]";

    return os;

}

//字符串输出
std::string Socket::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


std::ostream& operator<<(std::ostream& os, const Socket& sock)
{
    return sock.dump(os);
}


bool Socket::cancelRead()
{
    return IOManager::GetThis()->cancelEvent(m_fd, IOManager::Event::READ);
}


bool Socket::cancelWrite()
{
    return IOManager::GetThis()->cancelEvent(m_fd, IOManager::Event::WRITE);
}
bool Socket::cancelAccept()
{
    return IOManager::GetThis()->cancelEvent(m_fd, IOManager::Event::READ);
}
bool Socket::cancelAll()
{
    return IOManager::GetThis()->cancelAll(m_fd);
}

//初始化套接字
void Socket::initSocket()
{
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_family != Family::Unix && m_type == SOCK_STREAM)   //禁用nagle算法
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
}

//创建新套接字
void Socket::newSocket()
{   
    m_fd = socket(m_family, m_type, m_protocol);
    if(KIT_LIKELY(m_fd != -1))
    {
        initSocket();
    }
    else
    {
        KIT_LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", " << m_protocol << ")errno=" << errno << ", is:" << strerror(errno);
    }

}

}