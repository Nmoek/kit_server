#include "../kit_server/address.h"
#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"
#include "../kit_server/iomanager.h"
#include "../kit_server/hook.h"
#include "../kit_server/socket.h"

using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();


void test_socket()
{
    IPAddress::ptr addr = Address::LookUpAnyIPAddress("www.baidu.com");
    
    if(addr)
    {
        KIT_LOG_INFO(g_logger) << "get address=" << addr->toString();
    }
    else
    {
        KIT_LOG_ERROR(g_logger) << "get addr fail";
    }
    
    //创建一个指定地址的TCP连接
    Socket::ptr sock = Socket::CreateTCP(addr);
    addr->setPort(80);
    if(!sock->connect(addr))
    {
        KIT_LOG_ERROR(g_logger) << "connect:" << addr->toString() << " fail";
    }
    else
    {
        KIT_LOG_INFO(g_logger) << "connect:" << addr->toString() << " success";
    }

    const char buf[] = 
    "GET / HTTP/1.0\r\n"
    "host: www.baidu.com\r\n"
    "\r\n";
    int ret = sock->send(buf, sizeof(buf));
    if(ret <= 0)
    {
        KIT_LOG_ERROR(g_logger) << "send fail ret = " << ret;
        return;
    }

    std::string temp;
    temp.resize(10 * 1024);

    ret = sock->recv(&temp[0], temp.size());
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "recv fail ret = " << ret;
        return;
    }


    // temp.resize(ret);
    std::cout << "size:" << ret << std::endl;
    //KIT_LOG_INFO(g_logger) << "recv msg:" << temp;
    std::cout << "recv msg:\n" << temp << std::endl;
}


int main()
{
    KIT_LOG_DEBUG(g_logger) << "test begin";
    IOManager iom;
    iom.schedule(&test_socket);

    KIT_LOG_DEBUG(g_logger) << "test end";

    return 0;
}