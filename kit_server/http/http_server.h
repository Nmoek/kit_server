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

/**
 * @brief HTTP服务器类
 */
class HttpServer: public TcpServer
{
public:
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief HTTP服务器类构造函数
     * @param[in] keepalive 是否是长连接 默认否
     * @param[in] worker 服务执行调度器 默认是当前线程持有的调度器
     * @param[in] accept_worker 接收连接调度器 默认是当前线程持有的调度器
     */
    HttpServer(bool keepalive = false, IOManager* worker = IOManager::GetThis(), 
                IOManager* accept_worker = IOManager::GetThis());
    
    /**
     * @brief 获取长连接状态
     * @return true 处于长连接
     * @return false 处于短连接
     */
    bool getKeepalive() const {return m_isKeepalive;}

    /**
     * @brief 获取服务分发器
     * @return ServletDispatch::ptr 
     */
    ServletDispatch::ptr getServletDispatch() const {return m_dispatch;}

    /**
     * @brief 设置服务分发器
     * @param[in] v 
     */
    void setServletDispatch(ServletDispatch::ptr v) {m_dispatch = v;}

    /**
     * @brief 设置服务器名称
     * @param[in] v 
     */
    virtual void setName(const std::string& v) override;

protected:
    /**
     * @brief 处理已经连接的客户端  完成数据交互通信
     * @param[in] client 和客户端通信的Socket对象智能指针
     */
    virtual void handleClient(Socket::ptr client) override;

private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// 服务分发器
    ServletDispatch::ptr m_dispatch;
};


}
}

#endif 