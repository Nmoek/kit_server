#ifndef _KIT_HTTP_SERVR_H_
#define _KIT_HTTP_SERVR_H_

#include "http_session.h"
#include "http.h"
#include "../tcp_server.h"
#include "../iomanager.h"
#include "servlet.h"

#include <memory>

namespace kit_server
{
namespace http
{

class HttpServer: public TcpServer
{
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = false, IOManager* worker = IOManager::GetThis(), 
                IOManager* accept_worker = IOManager::GetThis());
    
    bool getKeepalive() const {return m_isKeepalive;}

    ServletDispatch::ptr getServletDispatch() const {return m_dispatch;}
    void setServletDispatch(ServletDispatch::ptr v) {m_dispatch = v;}

    virtual void setName(const std::string& v) override;

protected:
    //处理已经连接的客户端  完成数据交互通信
    virtual void handleClient(Socket::ptr client) override;

private:
    //是否支持长连接
    bool m_isKeepalive;

    ServletDispatch::ptr m_dispatch;
};


}
}

#endif 