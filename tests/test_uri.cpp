#include "../kit_server/uri.h"
#include "../kit_server/Log.h"


using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();

int main()
{
    KIT_LOG_DEBUG(g_logger) << "test begin";

    Uri::ptr uri = Uri::Create("http://www.baidu.com/test/uri?a=100&name=kit#fra中文");


    KIT_LOG_INFO(g_logger) << "uri:\n" << uri->toString();  
    auto addr = uri->createAddress();
    KIT_LOG_INFO(g_logger) << "addr:\n" << *addr;


    KIT_LOG_DEBUG(g_logger) << "test end";

    return 0;
}