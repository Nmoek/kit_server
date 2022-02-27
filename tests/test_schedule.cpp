#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"


using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();


void func1()
{
    KIT_LOG_DEBUG(g_logger) << "func1 work!!!!!!!!!";
    sleep(1);
    //KIT_LOG_DEBUG(g_logger) << "sleep out";
    static int i = 5;
    if(--i >= 0)
        Scheduler::GetThis()->schedule(&func1, GetThreadId());
   
}

void func2()
{
    KIT_LOG_DEBUG(g_logger) << "func2 work!!!!!!!!!";
    Scheduler::GetThis()->schedule(&func2);
    sleep(2);
}


void func3()
{
    KIT_LOG_DEBUG(g_logger) << "func3 work!!!!!!!!!";
    Scheduler::GetThis()->schedule(&func3);
    sleep(3);
}


int main()
{
    g_logger->addAppender(LogAppender::ptr(new FileLogAppender("schudeler.txt")));

    Scheduler sc("test", 3, false);

    //sc.schedule(&func3);
    

    sc.start();

    sc.schedule(&func1);
    // Coroutine::ptr c1(new Coroutine(&func2));
    // sc.schedule(c1);

    sc.stop();


    KIT_LOG_INFO(g_logger) << "test over";

    return 0;
}