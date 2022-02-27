#include "socket_stream.h"
#include "Log.h"

#include <sys/types.h>
#include <sys/socket.h>

namespace kit_server
{


//owner表示是否将Socket对象交由全权管理
//句柄关闭是否由本类来操作
SocketStream::SocketStream(Socket::ptr sock, bool owner)
    :m_sock(sock)
    ,m_owner(owner)
{

}

SocketStream::~SocketStream()
{
    if(m_owner && m_sock)
        m_sock->close();
    
}

int SocketStream::read(void *buffer, size_t len)
{
    if(!isConnected())
        return -1;
    
    return m_sock->recv(buffer, len);
}


//从当前ByteArray的position 向内存池写入 
int SocketStream::read(ByteArray::ptr ba, size_t len)
{
    if(!isConnected())
        return -1;

    //通过iovec和ByteArray内存空间建立映射
    //获取到即将写入的空间
    std::vector<struct iovec> iovs;
    ba->getWriteBuf(iovs, len);

    //接收数据 直接就写入到了指向ByteArray的内存空间中
    int ret = m_sock->recv(&iovs[0], iovs.size());
    //由于getWriteBuf() 并不会修改内存指针位置 手动修改m_position
    if(ret > 0)
    {
        ba->setPosition(ba->getPosition() + ret);
    }

    return ret; 
}

int SocketStream::write(const void* buffer, size_t len)
{
    if(!isConnected())
        return -1;
    
    return m_sock->send(buffer, len);
}



//从当前ByteArray的position 向外输出 
int SocketStream::write(ByteArray::ptr ba, size_t len)
{
    if(!isConnected())
        return -1;

    std::vector<struct iovec> iovs;
    ba->getReadBuf(iovs, len);

    int ret = m_sock->send(&iovs[0], iovs.size());
    if(ret > 0)
    {
        ba->setPosition(ba->getPosition() + ret);
    }

    return ret;
}

void SocketStream::close()
{
    if(m_sock && isConnected())
        m_sock->close();
}

//套接字是否处于连接状态
bool SocketStream::isConnected() const
{
    return m_sock && m_sock->isConnected();
}







}