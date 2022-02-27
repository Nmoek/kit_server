#include "tcp_server.h"
#include "Log.h"
#include "config.h"

#include <vector>
#include <errno.h>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


//Tcp Server 读超时默认超时项 2min(单位ms) 
static ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = 
    Config::LookUp("tcp_server.read_timeout", (uint64_t)(60 * 2 * 1000), "tcp server read timeout");


TcpServer::TcpServer(IOManager* worker, IOManager* accept_worker)
    :m_worker(worker)
    ,m_acceptWorker(accept_worker)
    ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
    ,m_name("kit/1.0.0")    //服务器的版本号
    ,m_stopped(true)
{

}


TcpServer::~TcpServer()
{
    //关闭所有的监听套接字
    for(auto &x : m_listenSockets)
    {
        x->close();
    }

    m_listenSockets.clear();
}


//支持一个地址bind  直接复用多个地址的情况 只是容器里只有一个地址
bool TcpServer::bind(Address::ptr addr)
{
    std::vector<Address::ptr> in_addrs, out_addrs;
    in_addrs.push_back(addr);
    return bind(in_addrs, out_addrs);
}

//支持多个地址bind  
bool TcpServer::bind(const std::vector<Address::ptr> &in_addrs, std::vector<Address::ptr> &out_addrs)
{
    //KIT_LOG_DEBUG(g_logger) << "TcpServer::bind";

    for(auto &x : in_addrs)
    {
        //创建协议不确定 但是传输类型是TCP的socket
        Socket::ptr sock = Socket::CreateTCP(x);
        //bind绑定地址
        if(!sock->bind(x))
        {
            KIT_LOG_ERROR(g_logger) << "bind error, errno=" << errno
                << ", is:" << strerror(errno)
                << ", addr=[" << x->toString() << "]";
            
            out_addrs.push_back(x);
            continue;
        }

        //listen监听地址
        if(!sock->listen())
        {
            KIT_LOG_ERROR(g_logger) << "listen error, errno=" << errno
                << ", is:" << strerror(errno)
                << ", addr=[" << x->toString() << "]";
            

            out_addrs.push_back(x);
            continue;
        }

        //成功监听
        m_listenSockets.push_back(sock);
    }

    //如果存在监听失败的套接字 要将成功监听那部分清除 
    if(out_addrs.size())
    {
        m_listenSockets.clear();
        return false;
    }

    for(auto &x : m_listenSockets)
    {
        //利用重载<<符号 打印一下Socket的内容
        KIT_LOG_INFO(g_logger) << "sock bind success: " << *x;
    }

    return true;


}

//服务器启动 让每一个套接字去accept
bool TcpServer::start()
{
   // KIT_LOG_DEBUG(g_logger) << "TcpServer::start";

    if(!m_stopped) 
        return true;
        
    m_stopped = false;

    for(auto &x : m_listenSockets)
    {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), x));
    }

    return true;
}

//服务器停止
bool TcpServer::stop()
{
    if(m_stopped)
        return true;
    m_stopped = true;

    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self](){
        
        //唤醒所有线程 进行退出
        for(auto &x : self->m_listenSockets)
        {   
            x->cancelAll();
        }

        self->m_listenSockets.clear();
    });

    return true;
}


//处理连接上的客户端 
void TcpServer::handleClient(Socket::ptr client)
{
    KIT_LOG_INFO(g_logger) << "client handle start" << ", clien=" << *client;
}

//处理监听到的客户端 处理accept
void TcpServer::startAccept(Socket::ptr sock)
{
    KIT_LOG_DEBUG(g_logger) << "TcpServer startAccept";

    //循环 只要不停止就要一直去accept客户端
    while(!m_stopped)
    {
        Socket::ptr client = sock->accept();
        if(client)
        {
            //给每一通信套接字设置读超时
            client->setRecvTimeout(m_recvTimeout);

            //将通信套接字加入线程池管理 
            m_worker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        }
        else 
        {
            KIT_LOG_ERROR(g_logger) << "accept error, client=" << client.get();
        }
    }

}




}