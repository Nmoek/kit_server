#ifndef _KIT_FDMANAGER_H_
#define _KIT_FDMANAGER_H_

#include "mutex.h"
#include "iomanager.h"
#include "single.h"

#include <memory>
#include <vector>


namespace kit_server
{

/**
 * @brief 文件句柄类
 * @details 管理文件句柄的类型、阻塞状态、关闭状态、读/写超时时间
 */
class FdCtx: public std::enable_shared_from_this<FdCtx>
{
public:
    typedef std::shared_ptr<FdCtx> ptr;
    
    /**
     * @brief 文件句柄类构造函数
     * @param[in] fd 传入fd文件句柄 
     */
    FdCtx(int fd);

    /**
     * @brief 文件句柄类析构函数
     */
    ~FdCtx();

    /**
     * @brief 文件句柄是否已经初始化
     * @return true 已经初始化
     * @return false 没有初始化
     */
    bool isInit() const {return m_isInit;}

    /**
     * @brief 文件句柄是否是套接字socket类型
     * @return true 是socket
     * @return false 不是socket
     */
    bool isSocket() const {return m_isSocket;}

    /**
     * @brief 文件句柄是否已经关闭
     * @return true 已经关闭
     * @return false 没有关闭
     */
    bool isClose() const {return m_isClosed;}

    //设定用户级 非阻塞状态
    /**
     * @brief 设定是否是用户设置非阻塞状态
     * @param[in] flag 
     */
    void setUserNonblock(bool flag) {m_userNonblock = flag;}

    /**
     * @brief 获取是否是用户设置非阻塞状态
     * @return true 是
     * @return false 不是
     */
    bool getUserNonblock() const {return m_userNonblock;}

    /**
     * @brief 设置系统级 非阻塞状态
     * @param[in] flag 
     */
    void setSysNonblock(bool flag) {m_sysNonblock = flag;}

    /**
     * @brief 获取系统级 非阻塞状态
     * @return true 是
     * @return false 不是
     */
    bool getSysNonblock() const {return m_sysNonblock;}

    /**
     * @brief 设置IO超时时间
     * @param[in] type 读(SO_REVTIMEO)/写(SO_SNDTIMEO)超时类型
     * @param[in] time 超时时间，单位ms
     */
    void setTimeout(int type, uint64_t time);

    /**
     * @brief 获取IO超时时间
     * @param[in] type 读(SO_REVTIMEO)/写(SO_SNDTIMEO)超时类型
     * @return uint64_t 
     */
    uint64_t getTimeout(int type);

    /**
     * @brief 获取文件句柄
     * @return int 
     */
    int getFd() const {return m_fd;}

private:
    /**
     * @brief 文件句柄初始化，如果为socket套接字句柄fd置为非阻塞
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool init();

private:
    /*采用位域 来记录文件句柄上的一些状态*/
    /// 是否初始化
    bool m_isInit: 1;
    /// 是否是套接字socket类型
    bool m_isSocket: 1;
    /// 是否是用户主动设置为非阻塞
    bool m_userNonblock: 1;
    /// 是否hook非阻塞
    bool m_sysNonblock: 1;
    /// 是否关闭
    bool m_isClosed: 1;
    /// 文件句柄
    int m_fd;
    /// 读超时时间
    uint64_t m_recvTimeout = 0;
    /// 写超时时间
    uint64_t m_sendTimeout = 0;

};

/**
 * @brief 文件句柄管理类
 */
class FdManager
{
public:
    typedef RWMutex MutexType;

    /**
     * @brief 文件句柄管理类构造函数
     */
    FdManager();

    /**
     * @brief 获取文件句柄对象
     * @param[in] fd 文件句柄 
     * @param[in] auto_create 是否自动创建文件句柄对象 
     * @return FdCtx::ptr 
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    /**
     * @brief 删除文件句柄对象
     * @param fd 
     */
    void del(int fd);

private:
    /// 读写锁
    MutexType m_mutex;
    /// 文件句柄对象队列
    std::vector<FdCtx::ptr> m_fds;

};
//置为单例
typedef Single<FdManager> FdMgr;

}


#endif