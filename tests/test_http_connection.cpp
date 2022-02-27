#include "../kit_server/address.h"
#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"
#include "../kit_server/iomanager.h"
#include "../kit_server/hook.h"
#include "../kit_server/socket.h"
#include "../kit_server/macro.h"
#include "../kit_server/http/http.h"
#include "../kit_server/http/http_parser.h"
#include "../kit_server/http/http_server.h"
#include "../kit_server/http/servlet.h"
#include "../kit_server/http/http_connection.h"


using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();
static Logger::ptr g_logger2 = KIT_LOG_NAME("connect");
static Logger::ptr g_logger3 = KIT_LOG_NAME("connect2");


void test_pool()
{
    http::HttpConnectionPool::ptr pool(new http::HttpConnectionPool("www.sylar.top", "", 80, 10, 1000 * 30, 5));

    //添加一个循环定时器 每隔1s触发一次 发送一个GET请求
    IOManager::GetThis()->addTimer(1000, [pool](){
        auto result = pool->doGet("/", 500);

        KIT_LOG_INFO(g_logger) << "result=\n" << result->toString();

    }, true);


}

void run()
{

    g_logger2->addAppender(LogAppender::ptr(new FileLogAppender("./connect.dat")));
    g_logger3->addAppender(LogAppender::ptr(new FileLogAppender("./connect2.dat")));

    Address::ptr addr = Address::LookUpAny("www.sylar.top:80");
    KIT_LOG_DEBUG(g_logger) << "addr:" << *addr;
    if(!addr)
    {
        KIT_LOG_ERROR(g_logger) << "get addr error";
        return;
    }

    Socket::ptr sock = Socket::CreateTCP(addr);
    bool ret = sock->connect(addr);
    if(!ret)
    {
        KIT_LOG_ERROR(g_logger) << "connect " << *addr << "error";
        return;
    }

    http::HttpConnection::ptr con(new http::HttpConnection(sock));

    http::HttpRequest::ptr req(new http::HttpRequest);
    req->setHeader("host", "www.sylar.top");
    req->setPath("/blog/");
    KIT_LOG_INFO(g_logger) << "req:\n" << *req;
    //req->setMethod(http::HttpMethod::POST);
    con->sendRequest(req);


    auto rsp  = con->recvResponse();
    if(!rsp)
    {
        KIT_LOG_ERROR(g_logger) << "recv response error";
        return;
    }
    
    KIT_LOG_INFO(g_logger2) << "recv response:\n" << *rsp;
    KIT_LOG_INFO(g_logger) << "body size:" << rsp->getBody().size();

    std::cout << "----------------------------------------------" << std::endl;

    auto result = http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);

    KIT_LOG_INFO(g_logger) << "result=" << result->result
        << ",error str=" << result->error;

    KIT_LOG_INFO(g_logger3) << "response:\n" << (result->response ? result->response->toString() : "");

    std::cout << "=================================================" << std::endl;

    test_pool();
}


int main()
{
    IOManager iom("test connection", 2);
    iom.schedule(&run);


    return 0;
}