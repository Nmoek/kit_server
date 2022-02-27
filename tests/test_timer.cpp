#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"
#include "../kit_server/iomanager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>


using namespace std;
using namespace kit_server;

static Logger::ptr g_logger = KIT_LOG_NAME("root");




int main()
{
    KIT_LOG_DEBUG(g_logger) << "test begin";
    IOManager iom("test", 2);

    // auto f1 = [=](){
    //     KIT_LOG_INFO(g_logger) << "hello timer!!";
    // };

    // auto f2 = [=](){
    //     KIT_LOG_INFO(g_logger) << "hello recurring timer!!";
    // };

    //创建一个循环定时器
    static Timer::ptr t = iom.addTimer(1000, [](){
        static int i = 0;
        KIT_LOG_INFO(g_logger) << "hello timer!!, i = " << i;
        
        if(++i == 3)
            t->refresh();
            //t->reset(3000, true);
            //t->cancel();
 
    }, true);

    KIT_LOG_DEBUG(g_logger) << "test end";



    return 0;
}