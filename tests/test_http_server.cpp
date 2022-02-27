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

using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();


void run()
{
    http::HttpServer::ptr server(new http::HttpServer);

    auto addr = Address::LookUpAny("0.0.0.0:8888");
    while(!server->bind(addr))
        sleep(1);

    auto sd = server->getServletDispatch();
    sd->addServlet("/kit_server/aaa", [](http::HttpRequest::ptr req, 
                                    http::HttpResponse::ptr rsp,
                                    http::HttpSession::ptr s){
        rsp->setBody(req->toString());
        return 0;                   

    });


    sd->addGlobServlet("/kit_server/*", [](http::HttpRequest::ptr req, 
                                    http::HttpResponse::ptr rsp,
                                    http::HttpSession::ptr s){
        rsp->setBody("Glob:\r\n" + req->toString());

        return 0;                    

    });
    server->start();

}

int main(int argc, char *argv[])
{
    IOManager iom("http server", 2);
    iom.schedule(&run);



    return 0;
}