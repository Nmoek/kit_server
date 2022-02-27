#include "../kit_server/daemon.h"
#include "../kit_server/iomanager.h"


static kit_server::Logger::ptr g_logger = KIT_LOG_ROOT();

static kit_server::Timer::ptr t;
int server_main(int argc, char* argv[])
{
    KIT_LOG_INFO(g_logger) << kit_server::ProcessInfoMgr::GetInstance()->toString();
    kit_server::IOManager iom("test daemon", 1);
    t = iom.addTimer(1000, [](){
        KIT_LOG_INFO(g_logger) << "server_main timer tun!";
        static int count = 0;
        if(++count > 10)
        {
            t->cancel();
        }
    },true);


    return 0;
}

int main(int argc, char* argv[])
{
    return kit_server::start_daemon(argc, argv, server_main, argc != 1);
}