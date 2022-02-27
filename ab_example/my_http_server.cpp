#include "../kit_server/http/http_server.h"
#include "../kit_server/Log.h"


static kit_server::Logger::ptr g_logger = KIT_LOG_ROOT();
static kit_server::Logger::ptr g_logger2 = KIT_LOG_NAME("system");

void run()
{

    kit_server::Address::ptr addr = kit_server::Address::LookUpAnyIPAddress("0.0.0.0:8888");
    if(!addr)
    {
        KIT_LOG_ERROR(g_logger) << "get addr error";
        return;
    }

    kit_server::http::HttpServer::ptr server(new kit_server::http::HttpServer(true));
    while(!server->bind(addr))
    {   
        sleep(1);
    }

    server->start();

}

int main(int argc, char* argv[])
{
    g_logger2->setLevel(kit_server::LogLevel::Level::INFO);

    kit_server::IOManager iom("http_server", 1);
    iom.schedule(&run);


    return 0;
}