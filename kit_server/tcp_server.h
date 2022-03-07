#ifndef _KIT_TCP_SERVER_H_
#define _KIT_TCP_SERVER_H_


#include <memory>
#include <functional>
#include <vector>

#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "noncopyable.h"


namespace kit_server
{

/**
 * @brief TCP服务器
 */
class TcpServer: public std::enable_shared_from_this<TcpServer>, Noncopyable
{
public:
    typedef std::shared_ptr<TcpServer> ptr;

    /**
     * @brief Tcp服务器类构造函数
     * @param[in] worker 负责和客户端通信的线程池 
     * @param[in] accept_worker 负责接收客户端新连接的线程池
     */
    TcpServer(IOManager* worker = IOManager::GetThis(), 
                IOManager* accept_worker = IOManager::GetThis());

    /**
     * @brief Tcp服务器类析构函数
     */
    virtual ~TcpServer();

    /**
     * @brief 绑定一个地址
     * @param[in] addr 地址对象智能指针
     * @return true 绑定成功
     * @return false 绑定失败
     */
    virtual bool bind(Address::ptr addr);

    /**
     * @brief 绑定多个地址
     * @param[in] in_addrs 传入要绑定的地址数组
     * @param[out] out_addrs 传出绑定失败的地址数组
     * @return true 全部绑定成功
     * @return false 全部绑定失败
     */
    virtual bool bind(const std::vector<Address::ptr> &in_addrs, std::vector<Address::ptr> &out_addrs);

    /**
     * @brief 服务器启动
     * @return true 启动成功
     * @return false 启动失败
     */
    virtual bool start();

    /**
     * @brief 服务器停止
     * @return true 停止成功
     * @return false 停止失败
     */
    virtual bool stop();

    /**
     * @brief 设置读超时时间
     * @param[in] v 具体超时时间
     */
    void setRecvTimeout(uint64_t v)   {m_recvTimeout = v;}

    /**
     * @brief 获取读超时时间
     * @return uint64_t 
     */
    uint64_t getRecvTimeout() const {return  m_recvTimeout;}

    /**
     * @brief 设置服务器名称
     * @param[in] v 具体名称
     */
    virtual void setName(const std::string& v) {m_name = v;}

    /**
     * @brief 获取服务器名称
     * @return std::string 
     */
    std::string getName() const {return m_name;}

    /**
     * @brief 服务器工作状态
     * @return true 已经停止
     * @return false 还在运行
     */
    bool isStop() const {return m_stopped;}

protected:
    /**
     * @brief 处理已经连接的客户端  完成数据交互通信
     * @param[in] client 
     */
    virtual void handleClient(Socket::ptr client);

    /**
     * @brief 监听客户端 处理客户端新连接
     * @param sock 
     */
    virtual void startAccept(Socket::ptr sock);

private:
    //存储多个监听socket 可能支持多协议 可能存在多个网卡  可能监听多个地址  
    std::vector<Socket::ptr> m_listenSockets;
    //作为一个线程池  专门负责已经接收连接的客户端的调度
    IOManager* m_worker;
    //作为一个线程池  专门负责服务器监听套接字执行accept的调度
    IOManager* m_acceptWorker;
    //读取超时时间    防止攻击  防止死的客户端来浪费资源
    uint64_t m_recvTimeout;
    //用一个名字来进行区分 socket所属的server 也作为一个服务器版本号方便更迭
    std::string m_name;
    //服务器当前的工作状态
    bool m_stopped;


};



}


#endif