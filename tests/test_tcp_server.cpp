#include "../kit_server/address.h"
#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"
#include "../kit_server/iomanager.h"
#include "../kit_server/hook.h"
#include "../kit_server/socket.h"
#include "../kit_server/tcp_server.h"

using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();

void run()
{
    auto addr = Address::LookUpAny("0.0.0.0:8888");
    auto addr2 = UnixAddress::ptr(new UnixAddress("./tmp/unix_addr"));
    KIT_LOG_INFO(g_logger) << "addr=" << addr->toString();
   //KIT_LOG_INFO(g_logger) << "addr=" << *addr;
    KIT_LOG_INFO(g_logger) << "addr2=" << addr2->toString();

    std::vector<Address::ptr> in_addrs, out_addrs;
    in_addrs.push_back(addr);
    //in_addrs.push_back(addr2);
    TcpServer::ptr server(new TcpServer);
    while(!server->bind(in_addrs, out_addrs))
        sleep(2);
   
    server->start();
}


int main()
{
    IOManager iom("tcp_server");

    iom.schedule(&run);


    return 0;
}