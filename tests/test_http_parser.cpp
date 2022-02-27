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

using namespace std;
using namespace kit_server;

static Logger::ptr g_logger = KIT_LOG_ROOT();


char test_request_data[] = 
"POST / HTTP/1.1\r\n"
"host: www.baidu.com\r\n"
"content-leng";


void test_request()
{
    http::HttpRequestParser parser;
    std::string temp = test_request_data;
    size_t ret = parser.execute(&temp[0], temp.size());

    KIT_LOG_INFO(g_logger) << "execute ret = " << ret
        << ",has error=" << parser.hasError()
        << ",is finished=" << parser.isFinished()
        << ", total len=" << temp.size()
        << ", content length=" << parser.getContentLength(); 
    
    temp.resize(temp.size() - ret);
    KIT_LOG_INFO(g_logger) << "data=\n" << parser.getData()->toString();
    KIT_LOG_INFO(g_logger) << "re temp=" << temp;

}


char test_response_data[] = 
"HTTP/1.0 200 OK\r\n"
"Accept-Ranges: bytes\r\n"
"Cache-Control: no-cache\r\n"
"Content-Length: 9508\r\n"
"Content-Type: text/html\r\n"
"Date: Fri, 21 Jan 2022 09:10:09 GMT\r\n"
"P3p: CP=\" OTI DSP COR IVA OUR IND COM \"\r\n"
"P3p: CP=\" OTI DSP COR IVA OUR IND COM \"\r\n"
"Pragma: no-cache\r\n"
"Server: BWS/1.1\r\n"
"Set-Cookie: BAIDUID=724BAEA1537982702513F8E2FB225C4A:FG=1; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
"Set-Cookie: BIDUPSID=724BAEA1537982702513F8E2FB225C4A; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
"Set-Cookie: PSTM=1642758641; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
"Set-Cookie: BAIDUID=724BAEA153798270D2ABD4B41739368C:FG=1; max-age=31536000; expires=Sat, 21-Jan-23 09:50:41 GMT; domain=.baidu.com; path=/; version=1; comment=bd\r\n"
"Traceid: 164275864109089502828095481575910497583\r\n"
"Vary: Accept-Encoding\r\n"
"X-Frame-Options: sameorigin\r\n"
"X-Ua-Compatible: IE=Edge,chrome=1\r\n";

void test_response()
{
    http::HttpResponseParser parser;
    std::string temp = test_response_data;
    size_t ret = parser.execute(test_response_data, temp.size(), false);

    KIT_LOG_INFO(g_logger) << "execute ret = " << ret
        << ",has error=" << parser.hasError()
        << ",is finished=" << parser.isFinished()
        << ",total len=" << temp.size()
        << ",content length=" << parser.getContentLength(); 
    
    temp.resize(temp.size() - ret);
    KIT_LOG_INFO(g_logger) << "data=\n" << parser.getData()->toString();
    KIT_LOG_INFO(g_logger) << "re temp=" << temp;


}


int main()
{
    KIT_LOG_INFO(g_logger) << "test begin";

    test_request();
    std::cout << "---------------\n";
    test_response();

    KIT_LOG_INFO(g_logger) << "test end";


    return 0;
}