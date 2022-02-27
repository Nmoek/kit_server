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

using namespace std;
using namespace kit_server;

static Logger::ptr g_logger = KIT_LOG_ROOT();



void test_request()
{

    http::HttpRequest::ptr req(new http::HttpRequest);
    req->setMethod(http::HttpMethod::POST);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");

    req->dump(std::cout) << std::endl;

}


void test_response()
{
    http::HttpResponse::ptr rsp(new http::HttpResponse);
    rsp->setStatus(http::HttpStatus::MOVED_PERMANENTLY);
    rsp->setHeader("X-X", "kit");
    rsp->setBody("hello kit");
    rsp->setClose(false);
    rsp->dump(std::cout) << std::endl;
}


int main()
{
    KIT_LOG_INFO(g_logger) << "test begin";
    
    test_request();
    std::cout << "------------------\n";
    test_response();

    KIT_LOG_INFO(g_logger) << "test end";


    return 0;
}