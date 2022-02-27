#ifndef _KIT_HTTP_SESSION_H_
#define _KIT_HTTP_SESSION_H_


#include <memory>

#include "../stream.h"
#include "../socket_stream.h"
#include "http.h"

namespace kit_server
{
namespace http
{

/**
 * @brief HTTP被动发起连接
 */
class HttpSession: public SocketStream
{
public:
    typedef std::shared_ptr<HttpSession> ptr;
    
    /**
     * @brief HTTP会话类构造函数
     * @param[in] sock 套接字对象智能指针
     * @param[in] owner 句柄关闭是否由本类来自动操作
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 接收HTTP请求报文
     * @return HttpRequest::ptr 
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief 发回HTTP响应报文
     * @param[in] rsp HTTP响应报文智能指针
     * @return int 
     */
    int sendResponse(HttpResponse::ptr rsp);
};

}
}

#endif