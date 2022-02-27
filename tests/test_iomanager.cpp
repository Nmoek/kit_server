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

using namespace kit_server;
using namespace std;


static Logger::ptr g_logger = KIT_LOG_ROOT();


#define PORT    8080


void func1()
{
    KIT_LOG_INFO(g_logger) << "func1 work!!!!!!!!!!";
}


void func2()
{
    KIT_LOG_INFO(g_logger) << "func2 work!!!!!!!!!!";
}

void netio_test()
{
    // Coroutine::ptr cor(new Coroutine(&func2));

    // iom.schedule(cor);

    /*设置一个socket IO 去测试异步触发*/

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0)
    {
        std::cout << "socket create error:" << strerror(errno);
        return;
    }


    struct sockaddr_in sockaddr;
    bzero(&sockaddr, sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(PORT);
    sockaddr.sin_addr.s_addr = inet_addr("192.168.77.1");  //连接百度


    IOManager::GetThis()->addEvent(sock_fd, IOManager::Event::READ, [](){
        KIT_LOG_INFO(g_logger) << "sock io  read event!!";
    });

    IOManager::GetThis()->addEvent(sock_fd, IOManager::Event::WRITE, [](){
        KIT_LOG_INFO(g_logger) << "sock io  wire event!!";
    });

   //iom.cancelEvent(sock_fd, IOManager::Event::WRITE); 

  
    if(connect(sock_fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr)) < 0)
    {
        std::cout << "connect error:" << strerror(errno) << std::endl;
    }

    //把socket 置为非阻塞
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    //iom.cancelAll(sock_fd);

    // IOManager::GetThis()->cancelAll(sock_fd);

    //取消之前设置的读事件
    //iom.cancelEvent(sock_fd, IOManager::Event::READ); 
    //iom.delEvent(sock_fd, IOManager::Event::READ);

    //

    // char buf[100];
    // recv(sock_fd, buf, sizeof(buf), 0);
    
    // KIT_LOG_DEBUG(g_logger) << buf;
    //while(1);

    send(sock_fd, "hello", 5, 0);
    KIT_LOG_DEBUG(g_logger) << "send end";

}


int main()
{
    Thread::_setName("test");
    KIT_LOG_INFO(g_logger) << "test begin";
    IOManager iom("test", 3, false);

    iom.schedule(&func1);
    iom.schedule(&func2);
    iom.schedule(&netio_test);

//     // Coroutine::ptr cor(new Coroutine(&func2));

//     // iom.schedule(cor);

//     /*设置一个socket IO 去测试异步触发*/

//     int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if(sock_fd < 0)
//     {
//         std::cout << "socket create error:" << strerror(errno);
//         return 0;
//     }


//     struct sockaddr_in sockaddr;
//     bzero(&sockaddr, sizeof(struct sockaddr_in));
//     sockaddr.sin_family = AF_INET;
//     sockaddr.sin_port = htons(PORT);
//     sockaddr.sin_addr.s_addr = inet_addr("192.168.77.1");  //连接百度


//     IOManager::GetThis()->addEvent(sock_fd, IOManager::Event::READ, [](){
//         KIT_LOG_INFO(g_logger) << "sock io  read event!!";
//     });

//     IOManager::GetThis()->addEvent(sock_fd, IOManager::Event::WRITE, [](){
//         KIT_LOG_INFO(g_logger) << "sock io  wire event!!";
//     });

//    //iom.cancelEvent(sock_fd, IOManager::Event::WRITE); 

  
//     if(connect(sock_fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr)) < 0)
//     {
//         std::cout << "connect error:" << strerror(errno) << std::endl;
//     }

//     //把socket 置为非阻塞
//     fcntl(sock_fd, F_SETFL, O_NONBLOCK);

//     //iom.cancelAll(sock_fd);

//     // IOManager::GetThis()->cancelAll(sock_fd);

//     //取消之前设置的读事件
//     //iom.cancelEvent(sock_fd, IOManager::Event::READ); 
//     //iom.delEvent(sock_fd, IOManager::Event::READ);

//     //

//     // char buf[100];
//     // recv(sock_fd, buf, sizeof(buf), 0);
    
//     // KIT_LOG_DEBUG(g_logger) << buf;
//     //while(1);

//     send(sock_fd, "hello", 5, 0);
//     KIT_LOG_DEBUG(g_logger) << "send end";





    
    KIT_LOG_INFO(g_logger) << "test end";


    

    return 0;
}