#include "../kit_server/iomanager.h"
#include "../kit_server/socket.h"
#include "../kit_server/Log.h"
#include "../kit_server/address.h"

#include <iostream>
#include <string.h>
#include <string>

static kit_server::Logger::ptr g_logger = KIT_LOG_ROOT();


const char *ip = nullptr;
uint16_t port = 0;



void run()
{
    kit_server::IPAddress::ptr addr = kit_server::Address::LookUpAnyIPAddress(ip);
    if(!addr)
    {
        KIT_LOG_ERROR(g_logger) << "addr parser fail! ip =" << ip;
        return;
    }
    addr->setPort(port);

    kit_server::Socket::ptr sock = kit_server::Socket::CreateUDP(addr);
    

    kit_server::IOManager::GetThis()->schedule([addr, sock](){

        KIT_LOG_INFO(g_logger) << "udp client begin recv";
        char buf[1024];
        while(1)
        {
            memset(buf, 0, sizeof(buf));
            int ret = sock->recvFrom(buf, 1024, addr);
            if(ret < 0)
            {
                int err = sock->getError();
                KIT_LOG_ERROR(g_logger) << "recvfrom error!"
                    << " error=" << err
                    << ", is:" << strerror(err)
                    << ", addr" << *addr
                    << ", sock=" << *sock;

                return;
            } 

            KIT_LOG_INFO(g_logger) << "client recv msg:" << buf << "  len=" << ret;
        }


    });

    sleep(2);

    while(1)
    {
        std::string msg;
        std::cout << "input msg> ";

        getline(std::cin, msg);
        if(msg.size())
        {
            int ret = sock->sendTo(&msg[0], msg.size(), addr);
            if(ret < 0)
            {
                KIT_LOG_ERROR(g_logger) << "sendto fail! errno=" << errno 
                    << ",is:" << strerror(errno) << ", addr=" << *addr;
                break;
            }

        }


    }

}

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        std::cout << "usage:./echo_udp_server [ip] [port]" << std::endl;
        return 0;
    }

    ip = argv[1];
    port = atoi(argv[2]);

    kit_server::IOManager iom("udp client", 2);
    iom.schedule(&run);

    return 0;

}