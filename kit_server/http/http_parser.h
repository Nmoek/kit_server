#ifndef _KIT_HTTP_PARSER_H_
#define _KIT_HTTP_PARSER_H_


#include "../Log.h"
#include "http11_parser.h"
#include "httpclient_parser.h"
#include "http.h"


namespace kit_server
{
namespace http
{

/**
 * @brief HTTP请求解析类
 */
class HttpRequestParser
{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;

    /**
     * @brief HTTP请求解析类构造函数
     */
    HttpRequestParser();

    /**
     * @brief 执行解析动作
     * @param[in] data 需要解析的具体数据
     * @param[in] len 解析数据的长度
     * @return size_t 1:解析成功 -1:解析有问题  >0已经处理的字节数
     */
    size_t execute(char* data, size_t len);

    /**
     * @brief 报文解析是否结束
     * @return int 
     */
    int isFinished();

    /**
     * @brief 解析是否出错
     * @return int 
     */
    int hasError();

    /**
     * @brief 获取HTTP请求报文
     * @return HttpRequest::ptr 
     */
    HttpRequest::ptr getData() const {return m_request;}

    /**
     * @brief 设置解析时的错误码
     * @param[in] v  具体错误码
     */
    void setError(int v) {m_error = v;}

    /**
     * @brief 获取报文主体字段"content-length"中的长度 并转换为uint64_t
     * @return uint64_t 
     */
    uint64_t getContentLength();

    /**
     * @brief 获取解析请求报文的结构体
     * @return const http_parser& 
     */
    const http_parser& getParser() const {return m_parser;}

public:
    /**
     * @brief 获取请求报文头部最大缓冲区大小
     * @return uint64_t 
     */
    static uint64_t GetHttpRequestBufferSize();

    /**
     * @brief 获取请求报文主体最大大小
     * @return uint64_t 
     */
    static uint64_t GetHttpMaxBodySize();


private:
    //解析请求报文的结构体
    http_parser m_parser;
    //请求报文对象智能指针
    HttpRequest::ptr m_request;
    //1000: invalid method
    //1001: invalid version
    //1002: invalid field
    int m_error;    //错误码
};

std::ostream& operator<<(std::ostream& os, const HttpRequestParser& req);


/**
 * @brief HTTP响应解析类
 */
class HttpResponseParser
{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;

    /**
     * @brief HTTP响应解析类构造函数
     */
    HttpResponseParser();

    /**
     * @brief 执行解析动作
     * @param[in] data 需要解析的具体数据
     * @param[in] len 解析数据的长度
     * @param[in] chunck 标志位 判断是否是chunck类型
     * @return size_t 1:解析成功 -1:解析有问题  >0已经处理的字节数
     */
    size_t execute(char* data, size_t len, bool chunck);

    /**
     * @brief 报文解析是否结束
     * @return int 
     */
    int isFinished();
    
    /**
     * @brief 解析是否出错
     * @return int 
     */
    int hasError();

    /**
     * @brief 获取HTTP响应报文
     * @return HttpRequest::ptr 
     */
    HttpResponse::ptr getData() const {return m_response;}


    /**
     * @brief 设置解析时的错误码
     * @param[in] v  具体错误码
     */
    void setError(int v) {m_error = v;}

    /**
     * @brief 获取报文主体字段"content-length"中的长度 并转换为uint64_t
     * @return uint64_t 
     */
    uint64_t getContentLength();

    /**
     * @brief 获取解析响应报文的结构体
     * @return const http_parser& 
     */
    const httpclient_parser& getParser() const {return m_parser;}


public:
    /**
     * @brief 获取响应报文头部最大缓冲区大小
     * @return uint64_t 
     */
    static uint64_t GetHttpResponseBufferSize();

    /**
     * @brief 获取响应报文主体最大大小
     * @return uint64_t 
     */
    static uint64_t GetHttpMaxBodySize();

private:
    //解析响应报文的结构体
    httpclient_parser m_parser;
    //响应报文对象
    HttpResponse::ptr m_response;
    //错误码
    int m_error;
};
std::ostream& operator<<(std::ostream& os, const HttpResponseParser& rsp);

}
}

#endif