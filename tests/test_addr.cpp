#include "../kit_server/address.h"
#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"
#include "../kit_server/iomanager.h"
#include "../kit_server/hook.h"

using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();


void test()
{
    std::vector<Address::ptr> addrs;

    bool val = Address::LookUp(addrs, "www.baidu.com:ftp");
    if(!val)
    {
        KIT_LOG_ERROR(g_logger) << "Address::LookUp error!";
        return;
    }

    //打印解析获取到的地址
    for(size_t i = 0; i < addrs.size();++i)
    {
        KIT_LOG_INFO(g_logger) << i << "--" << addrs[i]->toString();
    }
}

void test2()
{
    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > mp;

    if(!Address::GetInertfaceAddresses(mp))
    {
        KIT_LOG_ERROR(g_logger) << "Address::GetInertfaceAddresses error";
        return;
    }

    for(auto &x : mp)
    {
        KIT_LOG_INFO(g_logger) << x.first << "--" << x.second.first->toString() << "/" << x.second.second;
    }

    // for(auto &x : mp)
    // {
    //     auto addr = std::dynamic_pointer_cast<IPAddress>(x.second.first);
    //     KIT_LOG_INFO(g_logger) << x.first << "--" << x.second.first->toString() << "/" << x.second.second
    //         << ", 子网地址:" << addr->subnetAddress(x.second.second)->toString();
        

    // }
}


void test_ipv4()
{
    // auto addr = IPAddress::CreateFromText("www.sylar.top");

    // if(addr)
    // {
    //     KIT_LOG_INFO(g_logger) << addr->toString();
    // }
    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > mp;

    if(!Address::GetInertfaceAddresses(mp))
    {
        KIT_LOG_ERROR(g_logger) << "Address::GetInertfaceAddresses error";
        return;
    }


    for(auto &x : mp)
    {

        auto addr = std::dynamic_pointer_cast<IPAddress>(x.second.first);
        if(addr->getAddr()->sa_family == AF_INET)
            KIT_LOG_INFO(g_logger) << x.first << "--" << x.second.first->toString() << "/" << x.second.second
            << ", 广播地址:" << addr->broadcastAddress(x.second.second)->toString() << ", 子网地址:" << addr->subnetAddress(x.second.second)->toString();
    }

    
}


int main()
{
    KIT_LOG_INFO(g_logger) << "test begin";
    test_ipv4();
    // test2();

    KIT_LOG_INFO(g_logger) << "test end";

    return 0;
}