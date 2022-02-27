#ifndef _KIT_SOCKET_H_
#define _KIT_SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>

#include "noncopyable.h"
#include "address.h"

namespace kit_server
{

class Socket: public std::enable_shared_from_this<Socket>, Noncopyable
{
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    /**
     * @brief 套接字类型
     */
    enum Type
    {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };


    /**
     * @brief 地址族类型
     */
    enum Family
    {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        Unix = AF_UNIX
    };


    /**
     * @brief 使用传入的地址 创建TCP套接字对象
     * @param[in] address 传入指定的通信地址对象智能指针
     * @return Socket::ptr 
     */
    static Socket::ptr CreateTCP(Address::ptr address);

    /**
     * @brief 使用传入的地址 创建UDP套接字对象
     * @param[in] address 传入指定的通信地址对象智能指针
     * @return Socket::ptr 
     */
    static Socket::ptr CreateUDP(Address::ptr address);
    
    /**
     * @brief 默认创建TCP IPv4 套接字
     * @return Socket::ptr 
     */
    static Socket::ptr CreateTCPSocket();

    /**
     * @brief 默认创建UDP IPv4 套接字
     * @return Socket::ptr 
     */
    static Socket::ptr CreateUDPSocket();

    /**
     * @brief 默认创建TCP IPv6 套接字
     * @return Socket::ptr 
     */
    static Socket::ptr CreateTCPSocket6();
    /**
     * @brief 默认创建UDP IPv6 套接字
     * @return Socket::ptr 
     */
    static Socket::ptr CreateUDPSocket6();

    /**
     * @brief 默认创建TCP UNIX域 套接字
     * @return Socket::ptr 
     */
    static Socket::ptr CreateUnixTCPSocket();

    /**
     * @brief 默认创建UDP UNIX域 套接字
     * @return Socket::ptr 
     */
    static Socket::ptr CreateUnixUDPSocket();

public:

    /**
     * @brief Socket类构造函数
     * @param[in] family 地址族
     * @param[in] type 套接字类型
     * @param[in] protocol 协议类型
     */
    Socket(int family, int type, int protocol = 0);

    /**
     * @brief Socket类析构函数
     */
    ~Socket();


    /**
     * @brief 获取发送超时时间 
     * @return 返回超时时间 单位ms
     */
    int64_t getSendTimeout() const;

    /**
     * @brief 设置发送超时时间
     * @param[in] val 传入设置的具体的时间 单位ms
     */
    void setSendTimeout(int64_t val);

    /**
     * @brief 获取接收超时时间
     * @return 返回超时时间 单位ms
     */
    int64_t getRecvTimeout() const;

    /**
     * @brief 设置接收超时时间
     * @param[in] val 传入设置的具体的时间 单位ms
     */
    void setRecvTimeout(int64_t val);

 
    /**
     * @brief 获取套接字配置信息
     * @param[in] level 在哪一个层级句柄获取
     * @param[in] option 获取的具体配置
     * @param[in] result 获取的结果
     * @param[in] len 参数大小
     * @return true 获取成功
     * @return false 获取失败
     */
    bool getOption(int level, int option, void* result, size_t* len);

    /**
     * @brief 获取套接字配置信息重载模板函数
     */
    template<class T>
    bool getOption(int level, int option, T& result)
    {
        socklen_t len = sizeof(T);
        return getOption(level, option, &result, &len);
    }

    /**
     * @brief 设置套接字配置信息
     * @param[in] level 在哪一个层级句柄设置
     * @param[in] option 设置的具体配置
     * @param[in] result 设置的结果
     * @param[in] len 参数大小
     * @return true 设置成功
     * @return false 设置失败
     */
    bool setOption(int level, int option, const void* result, size_t len);

    /**
     * @brief 设置套接字配置信息重载模板函数
     */
    template<class T>
    bool setOption(int level, int option, const T& result)
    {
        return setOption(level, option, &result, (socklen_t)sizeof(T));
    }

    /**
     * @brief 接收客户端连接
     * @return 返回新的通信套接字智能指针
     */
    Socket::ptr accept();

    /**
     * @brief 套接字绑定地址
     * @param[in] addr 传入的通信地址智能指针
     * @return 绑定成功 
     * @return 绑定失败
     */
    bool bind(const Address::ptr addr);
    
    /**
     * @brief 连接远端地址，带有超时功能
     * @param[in] addr  传入的通信地址智能指针
     * @param[in] timeout_ms 连接的超时时间 
     * @return 连接成功
     * @return 连接失败
     */
    bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    /**
     * @brief 服务器监听
     * @param[in] backlog 监听最大数值 
     * @return 监听成功 
     * @return 监听失败
     */
    bool listen(int backlog = SOMAXCONN);
    
    /**
     * @brief 关闭套接字
     * @return true 已经处于关闭
     * @return false 还没处于关闭刚刚关闭
     */
    bool close();
    
    //send API
    int send(const void *buffer, size_t len, int flags = 0);
    int send(const struct iovec* buffers, size_t len, int flags = 0);
    int sendTo(const void * buffer, size_t len, const Address::ptr addr_to, int flags = 0);
    int sendTo(const  struct iovec* buffers, size_t len, const Address::ptr addr_to, int flags = 0);


    //recv API
    int recv(void *buffer, size_t len, int flags = 0);
    int recv(struct iovec* buffers, size_t len, int flags = 0);
    int recvFrom(void * buffer, size_t len, Address::ptr addr_from, int flags = 0);
    int recvFrom(struct iovec* buffers, size_t len, Address::ptr addr_from, int flags = 0);

    /**
     * @brief 获取远端地址
     * @return 返回通信地址对象智能指针
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本地地址
     * @return 返回通信地址对象智能指针
     */
    Address::ptr getLocalAddress();

 
    /**
     * @brief 获取地址族
     * @return int 
     */
    int getFamily() const {return m_family;}

    /**
     * @brief 获取套接字类型
     * @return int 
     */
    int getType() const {return m_type;}

    /**
     * @brief 获取协议类型
     * @return int 
     */
    int getProtocol() const {return m_protocol;}

    /**
     * @brief 套接字是否处于连接状态
     * @return true 连接成功
     * @return false 连接失败
     */
    bool isConnected() const {return m_isConnectd;}

    //是否有效
    /**
     * @brief 套接字是否有效
     * @return true 有效
     * @return false 无效
     */
    bool isValid();

    /**
     * @brief 获取套接字上的错误
     * @return int 返回错误码
     */
    int getError();

    /**
     * @brief 套接字的信息放入标准输出流中
     * @param os 标准输出流
     * @return std::ostream& 
     */
    std::ostream& dump(std::ostream& os) const;


    /**
     * @brief 套接字的信息以字符串输出
     * @return std::string 
     */
    std::string toString() const;

    /**
     * @brief 获取套接字句柄fd
     * @return int 
     */
    int getFd() const {return m_fd;}

    /**
     * @brief 取消并即刻激活套接字上的读事件
     * @return true 
     * @return false 
     */
    bool cancelRead();

    /**
     * @brief 取消并即刻激活套接字上的写事件
     * @return true 
     * @return false 
     */
    bool cancelWrite();

    /**
     * @brief 取消并即刻激活监听套接字上的读事件
     * @return true 
     * @return false 
     */
    bool cancelAccept();

    /**
     * @brief 取消并即刻激活套接字上的所有读写事件
     * @return true 
     * @return false 
     */
    bool cancelAll();

private:
    /**
     * @brief 初始化fd
     * @param[in] fd 传入文件句柄
     * @return 初始化成功
     * @return 初始化失败
     */
    bool init(int fd);

    /**
     * @brief 初始化套接字
     */
    void initSocket();

    /**
     * @brief 创建新的套接字
     */
    void newSocket();

private:   
    //套接字句柄
    int m_fd;
    //地址族类型
    int m_family;
    //套接字类型
    int m_type;
    //套接字协议
    int m_protocol;
    //套接字连接状态
    bool m_isConnectd;
    //本地地址对象智能指针
    Address::ptr m_localAddr;
    //远端地址对象智能指针
    Address::ptr m_remoteAddr;

};

//重载输出
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}


#endif 