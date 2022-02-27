#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"
#include "../kit_server/iomanager.h"
#include "../kit_server/hook.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <sstream>

using namespace std;
using namespace kit_server;

static Logger::ptr g_logger = KIT_LOG_ROOT();

#define PORT  80
#define ADDR   "183.232.231.172"


void func1()
{
    sleep(2);
}

void func2()
{
    sleep(3);
}

void test_hook()
{
        IOManager iom("test", 1);

    auto f1 = [](){
        KIT_LOG_DEBUG(g_logger) << "f1 start";
        sleep(2);
        KIT_LOG_DEBUG(g_logger) << "f1 end";
    };

    auto f2 = [](){
        KIT_LOG_DEBUG(g_logger) << "f2 start";
        sleep(3);
        KIT_LOG_DEBUG(g_logger) << "f2 end";
    };


    iom.schedule(f1);
    iom.schedule(f2);
}

void test_sock()
{

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        KIT_LOG_ERROR(g_logger) << "socket error";
        return;
    }

    // IOManager::GetThis()->addEvent(fd, IOManager::Event::READ, [](){
    //     KIT_LOG_INFO(g_logger) << "sock fd read evenr trigger";
    // });

    // IOManager::GetThis()->addEvent(fd, IOManager::Event::WRITE, [](){
    //     KIT_LOG_INFO(g_logger) << "sock fd write evenr trigger";
    // });

    struct sockaddr_in sockaddr;

    bzero(&sockaddr, sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(PORT);
    sockaddr.sin_addr.s_addr = inet_addr(ADDR);

    int ret = connect(fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr));
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "connect error";
        return;
    }

    KIT_LOG_DEBUG(g_logger) << "connect ret = " << ret << ", errno = " << errno << ", is" << strerror(errno);

    std::stringstream p;
    p << "GET / HTTP/1.0\r\n\r\n";
    ret = write(fd, (void *)p.str().c_str(), p.str().size());

    std::string msg;
    msg.resize(1024*1024);
    ret = read(fd, &msg[0], msg.size());

    KIT_LOG_DEBUG(g_logger) << "recv msg: " << msg;
    //close(fd);
}


int main()
{  
    KIT_LOG_DEBUG(g_logger) << "test begin";
    IOManager iom("test_sock", 1);
    iom.schedule(&test_sock);
   // test_hook();
   //test_sock();

    KIT_LOG_DEBUG(g_logger) << "test end";

    return 0;
}