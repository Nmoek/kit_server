#include "http_server.h"
#include "../Log.h"


namespace kit_server
{
namespace http
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


HttpServer::HttpServer(bool keepalive, IOManager* worker, IOManager* accept_worker)
    :TcpServer(worker, accept_worker)
    ,m_isKeepalive(keepalive)
{
    m_dispatch.reset(new ServletDispatch);

}


//处理已经连接的客户端  完成数据交互通信
void HttpServer::handleClient(Socket::ptr client)
{
    KIT_LOG_DEBUG(g_logger) << "HttpServer::handleClient, client=" << *client;
    HttpSession::ptr session(new HttpSession(client));

    do
    {
        // 接收请求报文
        auto req = session->recvRequest();
        if(!req)
        {
            KIT_LOG_WARN(g_logger) << "recv http request fail, errno=" << errno
                << ", is:" << strerror(errno)
                << ", client=" << *client;
            break;
        }

        //回复响应报文
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);

        // rsp->setBody(std::string("hello kit!"));

        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose())
            break;

    } while (1);
    
    KIT_LOG_DEBUG(g_logger) << "session close!!!";

    session->close();
}

void HttpServer::setName(const std::string& v)
{
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}



}
}