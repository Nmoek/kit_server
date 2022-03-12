#include "../kit_server/iomanager.h"
#include "../kit_server/socket.h"
#include "../kit_server/Log.h"
#include "../kit_server/address.h"
#include "../kit_server/bytearray.h"


#include <vector>
#include <sys/un.h>
#include <errno.h>

static kit_server::Logger::ptr g_logger = KIT_LOG_ROOT();

void run()
{
    kit_server::IPAddress::ptr addr = kit_server::Address::LookUpAnyIPAddress("192.168.77.136:5555");
    if(!addr)
    {
        KIT_LOG_ERROR(g_logger) << "udp addr get fail!";
        return;
    }

    kit_server::Socket::ptr sock = kit_server::Socket::CreateUDP(addr);
    if(sock->bind(addr))
    {
        KIT_LOG_INFO(g_logger) << "udp bind addr sucess:" << *addr;
    }
    else
    {
        KIT_LOG_ERROR(g_logger) << "udp bind addr fail:" << *addr;
        return;
    }
    

    while(1)
    {
        kit_server::Address::ptr from_addr(new kit_server::IPv4Address);
        char buf[1024];
        int ret = sock->recvFrom(buf, 1024, from_addr);
        if(ret < 0)
        {
            KIT_LOG_ERROR(g_logger) << "recvfrom error! errno = " << errno << ",is:" << strerror(errno);
            break;
        }
        else if(ret > 0)
        {
            
            buf[ret] = '\0';
            KIT_LOG_INFO(g_logger) << "recv msg:" << buf;

            ret = sock->sendTo(buf, ret, from_addr);
            if(ret < 0)
            {
                KIT_LOG_ERROR(g_logger) << "sendto error! errno=" << errno << ",is:" << strerror(errno);
                break;
            }

        }

    }

}




int main()
{
    kit_server::IOManager iom("udp_server", 1);
    iom.schedule(&run);

    return 0;
}