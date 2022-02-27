#ifndef _KIT_SOCKET_STREAM_H_
#define _KIT_SOCKET_STREAM_H_


#include "stream.h"
#include "socket.h"
#include <memory>


namespace kit_server
{

class SocketStream: public Stream
{
public:
    typedef std::shared_ptr<SocketStream> ptr;

    /**
     * @brief SocketStream类构造函数
     * @param[in] sock 套接字对象智能指针
     * @param[in] owner 句柄关闭是否由本类来自动操作
     */
    SocketStream(Socket::ptr sock, bool owner = true);

    /**
     * @brief SocketStream类析构函数 owner=true就close()
     */
    ~SocketStream();

    virtual int read(void *buffer, size_t len) override;

    virtual int read(ByteArray::ptr ba, size_t len) override;

    virtual int write(const void* buffer, size_t len) override;

    virtual int write(ByteArray::ptr ba, size_t len) override;

    virtual void close() override;

    /**
     * @brief 获取套接字对象指针
     * @return Socket::ptr 
     */
    Socket::ptr getSocket() const {return m_sock;}

    /**
     * @brief 套接字是否处于连接状态
     * @return true 处于连接
     * @return false 处于不连接
     */
    bool isConnected() const;

protected:
    //Socket对象智能指针
    Socket::ptr m_sock;
    //句柄全权管理标志位
    bool m_owner;

};





}


#endif