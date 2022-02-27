#include "../kit_server/tcp_server.h"
#include "../kit_server/Log.h"
#include "../kit_server/iomanager.h"
#include "../kit_server/socket.h"
#include "../kit_server/bytearray.h"
#include "../kit_server/uri.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <errno.h>
#include <memory>

static kit_server::Logger::ptr g_logger = KIT_LOG_ROOT();

class EchoServer: public kit_server::TcpServer
{
public:
    typedef std::shared_ptr<EchoServer> ptr;
    enum Type
    {
        UNKOWN,
        TEXT = 1,
        BIN = 2
    };

    //标识输出的内容是二进制还是文本
    EchoServer(Type type);

    //处理已经连接的客户端  完成数据交互通信
    void handleClient(kit_server::Socket::ptr client);

private:
    Type m_type = Type::UNKOWN;


};


EchoServer::EchoServer(Type type)
    :m_type(type)
{

}


//处理已经连接的客户端  完成数据交互通信
void EchoServer::handleClient(kit_server::Socket::ptr client)
{
    KIT_LOG_INFO(g_logger) << "handleClient start, client=" << *client;

    kit_server::ByteArray::ptr ba(new kit_server::ByteArray);
    while(1)
    {
        ba->clear();

        std::vector<struct iovec> iovs;
        ba->getWriteBuf(iovs, 1024);

        int ret = client->recv(&iovs[0], iovs.size());
        if(ret == 0)
        {
            KIT_LOG_INFO(g_logger) << "client closed:" << *client;
            break;
        }
        else if(ret < 0)
        {
            KIT_LOG_ERROR(g_logger) << "client error, errno=" << errno
                << ", is:" << strerror(errno);
                break;
        }

        ba->setPosition(ba->getPosition() + ret);
        ba->setPosition(0);
        if(m_type == Type::TEXT)
        {
            // KIT_LOG_INFO(g_logger) << "recv msg:" << ba->toString();
            std::cout << "recv msg:" << ba->toString() << std::endl;
        }
        else 
        {
            // KIT_LOG_INFO(g_logger) << "recv msg:" << ba->toHexString();
            std::cout << "recv msg:" << ba->toHexString() << std::endl;
        }

    }

}


EchoServer::Type type = EchoServer::Type::TEXT;

void run()
{
    EchoServer::ptr server(new EchoServer(type));

    auto addr = kit_server::Address::LookUpAny("0.0.0.0:8888");
    while(!server->bind(addr))
        sleep(1);

    server->start();

}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        KIT_LOG_INFO(g_logger) << "used as:" << argv[0] << "-t] or [" << argv[0] << "-b]";
        return 0;
    }

    if(strcmp(argv[1], "-b") == 0)
    {
        type = EchoServer::Type::BIN;
    }

    kit_server::IOManager iom("echo_server", 2);
    iom.schedule(&run);




    return 0;
}