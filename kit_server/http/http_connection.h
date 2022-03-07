#ifndef _KIT_HTTP_CONNECTION_H_
#define _KIT_HTTP_CONNECTION_H_


#include <memory>
#include <list>
#include <atomic>

#include "../stream.h"
#include "../socket_stream.h"
#include "http.h"
#include "../uri.h"
#include "../mutex.h"

namespace kit_server
{
namespace http
{

struct HttpResult
{
    typedef std::shared_ptr<HttpResult> ptr;

    /**
     * @brief 错误码枚举类型
     */
    enum class Error
    {
        //成功
        OK,
        //非法URL
        INVALID_URL,
        //非法地址
        INVALID_ADDR,
        //非法套接字
        INVALID_SOCK,
        //连接失败
        CONNECT_FAIL,
        //发送请求失败
        SEND_REQ_FAIL,
        //接收响应超时
        RECV_RSP_TIMEOUT,
        //连接池取出连接失败
        POOL_GET_CONNECT_FAIL,
        //连接池非法sokcet
        POOL_INVALID_SOCK,

    };

    /**
     * @brief HTTP报文处理结果结构体构造函数
     * @param[in] _result 错误码
     * @param[in] _response HTTP响应报文对象智能指针
     * @param[in] _error 错误原因短语
     */
    HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error)
        :result(_result)
        ,response(_response)
        ,error(_error) 
    {

    }

    /**
     * @brief HTTP报文处理结果以字符串形式输出
     * @return std::string 
     */
    std::string toString() const;

    //错误码
    int result;
    //HTTP响应报文对象智能指针
    HttpResponse::ptr response;
    //错误原因短语
    std::string error;
};

/**
 * @brief HTTP主动发起连接类
 */
class HttpConnection: public SocketStream
{
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    
    /**
     * @brief HTTP主动发起连接类构造函数
     * @param[in] sock 套接字对象智能指针
     * @param[in] owner 句柄关闭是否由本类来自动操作
     */
    HttpConnection(Socket::ptr sock, bool owner = true);

    /**
     * @brief HTTP主动发起连接类析构函数
     */
    ~HttpConnection();


    /**
     * @brief 接收HTTP响应报文
     * @return HttpResponse::ptr 
     */
    HttpResponse::ptr recvResponse();

    /**
     * @brief 发出HTTP请求报文
     * @param[in] rsp 指定HTTP请求报文智能指针
     * @return 
     *     @retval >0 返回实际发出字节数
     *     @retval <0 错误
     */
    int sendRequest(HttpRequest::ptr rsp);

    /**
     * @brief 设置连接创建时间
     * @param[in] v 具体创建时间 单位ms
     */
    void setCreateTime(uint64_t v) {m_createTime = v;}

    /**
     * @brief 获取连接创建时间
     * @return uint64_t 单位ms
     */
    uint64_t getCreateTime() const {return m_createTime;}
    
    /**
     * @brief HTTP连接上请求数+1
     */
    void addRequestCount() {++m_requestCount;}
    
    /**
     * @brief 获取HTTP连接上请求数
     * @return uint64_t 
     */
    uint64_t getRequestCount() const {return m_requestCount;}

public:
    /*便捷的创建HTTP请求*/
    /**
     * @brief 用GET方法将HTTP请求发给目标服务器
     * @param[in] url 指定访问的网址
     * @param[in] timeout_ms 接收超时时间 单位ms
     * @param[in] headers 指定的首部字段组
     * @param[in] body 指定的报文主体内容
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoGet(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /**
     * @brief 用POST方法将HTTP请求发给目标服务器
     * @param[in] url 指定访问的网址
     * @param[in] timeout_ms 接收超时时间 单位ms
     * @param[in] headers 指定的首部字段组
     * @param[in] body 指定的报文主体内容
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoPost(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /**
     * @brief 用GET方法将HTTP请求发给目标服务器
     * @param[in] uri 指定URI对象智能指针
     * @param[in] timeout_ms 接收超时时间 单位ms
     * @param[in] headers 指定的首部字段组
     * @param[in] body 指定的报文主体内容
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoGet(Uri::ptr uri, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /**
     * @brief 用POST方法将HTTP请求发给目标服务器
     * @param[in] uri 指定URI对象智能指针
     * @param[in] timeout_ms 接收超时时间 单位ms
     * @param[in] headers 指定的首部字段组
     * @param[in] body 指定的报文主体内容
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoPost(Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    
    /**
     * @brief 负责将HTTP请求发给目标服务器
     * @param[in] method HTTP请求方法
     * @param[in] url 指定访问的网址
     * @param[in] timeout_ms 接收超时时间 单位ms
     * @param[in] headers 指定的首部字段组
     * @param[in] body 指定的报文主体内容
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    

    /**
     * @brief 负责将HTTP请求发给目标服务器
     * @param[in] method HTTP请求方法
     * @param[in] uri 指定URI对象智能指针
     * @param[in] timeout_ms 接收超时时间 单位ms
     * @param[in] headers 指定的首部字段组
     * @param[in] body 指定的报文主体内容
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    
    /**
     * @brief 负责将HTTP请求发给目标服务器
     * @param[in] req 指定的HTTP请求智能指针
     * @param[in] uri 指定的URI智能指针
     * @param[in] timeout_ms 接收超时时间 单位ms
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms);

private:
    //连接创建时间
    uint64_t m_createTime;
    //连接上的请求数
    uint64_t m_requestCount;

};


/**
 * @brief HTTP 连接池
 */
class HttpConnectionPool
{
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;
    typedef Mutex MutexType;    //互斥锁

    /**
     * @brief HTTP连接池类构造函数
     * @param[in] host 域名
     * @param[in] vhost 备用域名
     * @param[in] port 端口号
     * @param[in] max_size 最大连接数
     * @param[in] maxAliveTime 每条连接最大存活时间
     * @param[in] maxRequest 每条连接最大请求数
     */
    HttpConnectionPool(const std::string& host, const std::string& vhost, 
        uint16_t port, uint32_t max_size, uint32_t maxAliveTime, uint32_t maxRequest);

    /**
     * @brief 从连接池中获取连接，没有可用资源要创建后返回
     * @return HttpConnection::ptr 
     */
    HttpConnection::ptr getConnection();

    /**
     * @brief 负责将HTTP请求发给目标服务器
     * @param[in] req HTTP请求报文智能指针
     * @param[in] timeout_ms 接收响应报文超时时间 
     * @return HttpResult::ptr 返回HTTP连接上操作结果
     */
    HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);

public:
    /**
     * @brief 使用GET方法将HTTP请求发给目标服务器
     * @param[in] url 指定的目标网址
     * @param[in] timeout_ms 接收超时时间
     * @param[in] headers 指定的首部字段
     * @param[in] body 指定的报文主体
     * @return HttpResult::ptr 返回HTTP连接上操作结果
     */
    HttpResult::ptr doGet(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /**
     * @brief 使用POST方法将HTTP请求发给目标服务器
     * @param[in] url 指定的目标网址
     * @param[in] timeout_ms 接收超时时间
     * @param[in] headers 指定的首部字段
     * @param[in] body 指定的报文主体
     * @return HttpResult::ptr 返回HTTP连接上操作结果
     */
    HttpResult::ptr doPost(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /**
     * @brief 使用GET方法将HTTP请求发给目标服务器
     * @param[in] uri 指定的URI对象智能指针 
     * @param[in] timeout_ms 接收超时时间
     * @param[in] headers 指定的首部字段
     * @param[in] body 指定的报文主体
     * @return HttpResult::ptr 返回HTTP连接上操作结果
     */
    HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /**
     * @brief 使用POST方法将HTTP请求发给目标服务器
     * @param[in] uri 指定的URI对象智能指针 
     * @param[in] timeout_ms 接收超时时间
     * @param[in] headers 指定的首部字段
     * @param[in] body 指定的报文主体
     * @return HttpResult::ptr 返回HTTP连接上操作结果
     */
    HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    

    /**
     * @brief 负责将HTTP请求发给目标服务器
     * @param[in] method 指定HTTP请求方法
     * @param[in] url 指定的目标网址
     * @param[in] timeout_ms 接收超时时间
     * @param[in] headers 指定的首部字段
     * @param[in] body 指定的报文主体
     * @return HttpResult::ptr 返回HTTP连接上操作结果
     */
    HttpResult::ptr doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    

    /**
     * @brief 负责将HTTP请求发给目标服务器
     * @param[in] method 指定HTTP请求方法
     * @param[in] uri 指定的URI对象智能指针 
     * @param[in] timeout_ms 接收超时时间
     * @param[in] headers 指定的首部字段
     * @param[in] body 指定的报文主体
     * @return HttpResult::ptr 返回HTTP连接上操作结果
     */
    HttpResult::ptr doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

private:
    /**
     * @brief 释放连接资源
     * @param[in] ptr http连接指针
     * @param[in] pool http连接池指针
     */
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

private:
    /// 要连接的主机域名
    std::string m_host;
    /// 备用主机域名
    std::string m_vhost;
    /// 端口号
    uint16_t m_port;
    /// 最大连接数
    uint32_t m_maxSize;
    /// 每条连接最大存活时间
    uint32_t m_maxAliveTime;
    /// 每条连接上最大请求数量
    uint32_t m_maxRequest;
    /// 互斥锁 读写频次相当 不用读写锁
    MutexType m_mutex;
    /// 连接存储容器  list增删方便
    std::list<HttpConnection* > m_connections;
    /// 当前连接数量 可以突破最大连接数 但是使用完毕放回后若超限就要马上释放
    std::atomic<int32_t> m_total = {0};

};

}
}

#endif