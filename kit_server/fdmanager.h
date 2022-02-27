#ifndef _KIT_FDMANAGER_H_
#define _KIT_FDMANAGER_H_

#include "mutex.h"
#include "iomanager.h"
#include "single.h"

#include <memory>
#include <vector>


namespace kit_server
{

class FdCtx: public std::enable_shared_from_this<FdCtx>
{
public:
    typedef std::shared_ptr<FdCtx> ptr;
    typedef Mutex MutexType;

    FdCtx(int fd);
    ~FdCtx();

    bool init();

    //是否已经初始化
    bool isInit() const {return m_isInit;}
    //是否是socket套接字
    bool isSocket() const {return m_isSocket;}
    //是否已经关闭
    bool isClose() const {return m_isClosed;}

    //设定用户级 非阻塞状态
    void setUserNonblock(bool flag) {m_userNonblock = flag;}
    //获取用户级 非阻塞状态
    bool getUserNonblock() const {return m_userNonblock;}

    //设定系统级 非阻塞状态
    void setSysNonblock(bool flag) {m_sysNonblock = flag;}
    //获取系统级 非阻塞状态
    bool getSysNonblock() const {return m_sysNonblock;}

    //设定IO超时时间
    void setTimeout(int type, uint64_t time);
    //获取IO超时时间
    uint64_t getTimeout(int typr);

    int getFd() const {return m_fd;}

private:
    //采用位域 来记录 fd上的一些状态
    bool m_isInit: 1;
    bool m_isSocket: 1;
    bool m_userNonblock: 1;
    bool m_sysNonblock: 1;
    bool m_isClosed: 1;

    //套接字
    int m_fd;

    //套接字 接收超时时间
    uint64_t m_recvTimeout = 0;
    //套接字 发送超时时间
    uint64_t m_sendTimeout = 0;


};

class FdManager
{
public:
    typedef RWMutex MutexType;

    FdManager();

    //获取句柄  不存在就创建
    FdCtx::ptr get(int fd, bool auto_create = false);
    //删除句柄
    void del(int fd);



private:
    //读写锁
    MutexType m_mutex;
    //套接字对象集合
    std::vector<FdCtx::ptr> m_fds;

};
//置为单例
typedef Single<FdManager> FdMgr;

}


#endif