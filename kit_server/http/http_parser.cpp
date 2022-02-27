#include "http_parser.h"
#include "../Log.h"
#include "../config.h"
#include "http.h"

#include <stdint.h>
#include <string.h>


namespace kit_server
{
namespace http
{

/*****************request error*******************/
static const int INVALID_METHOD = 1000;
static const int INVALID_VERSION = 1001;
static const int INVALID_FIELD = 1002;

static kit_server::Logger::ptr g_logger = KIT_LOG_NAME("system");

//使用一个配置项 规定一个首部字段数据长度阈值默认4KB  来规避大数据发包攻击 
static ConfigVar<uint64_t>::ptr g_http_request_buffer_size = 
    Config::LookUp("http.request.buffer_size", (uint64_t)(4 * 1024), "http request buffer size");


//使用一个配置项 规定一个报文实体数据长度阈值默认64MB 
static ConfigVar<uint64_t>::ptr g_http_request_max_body_size = 
    Config::LookUp("http.request.max_body_size", (uint64_t)(64 *1024 * 1024), "http request max body size");

//使用一个配置项 规定一个首部字段数据长度阈值默认4KB  来规避大数据发包攻击 
static ConfigVar<uint64_t>::ptr g_http_response_buffer_size = 
    Config::LookUp("http.response.buffer_size", (uint64_t)(4 * 1024), "http response buffer size");


//使用一个配置项 规定一个报文实体数据长度阈值默认64MB 
static ConfigVar<uint64_t>::ptr g_http_response_max_body_size = 
    Config::LookUp("http.response.max_body_size", (uint64_t)(64 *1024 * 1024), "http response max body size");


static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;


//初始化结构放在匿名空间 防止污染
namespace {
struct _RequestSizeIniter
{
    //初始化 并设置回调
    _RequestSizeIniter()
    {
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();

        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();

        g_http_request_buffer_size->addListener([](const uint64_t &old_value, const uint64_t &new_value){

            s_http_request_buffer_size = new_value;
        });

        g_http_response_max_body_size->addListener([](const uint64_t &old_value, const uint64_t &new_value){

            s_http_response_max_body_size = new_value;
        });

        g_http_response_buffer_size->addListener([](const uint64_t &old_value, const uint64_t &new_value){

            s_http_response_buffer_size = new_value;
        });

        g_http_request_max_body_size->addListener([](const uint64_t &old_value, const uint64_t &new_value){

            s_http_request_max_body_size = new_value;
        });
    }
};
static _RequestSizeIniter _initer;
}

/**************************************request回调************************************/

/**
 * @brief 解析HTTP请求方法回调函数
 */
void on_request_method(void *data, const char *at, size_t length)
{
    //拿到this指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod method = CharsToHttpMethod(at);

    if(method == HttpMethod::INVALID_METHOD)
    {
        KIT_LOG_WARN(g_logger) << "http request invaild method:"
             << std::string(at, length);
        
        parser->setError(INVALID_METHOD);
        return;
    }

    parser->getData()->setMethod(method);

}

/**
 * @brief 解析URI回调函数 要自定义URI的解析故不使用
 */
void on_request_uri(void *data, const char *at, size_t length)
{

}

/**
 * @brief 解析分段标识符回调函数
 */
void on_request_fragment(void *data, const char *at, size_t length)
{
    //拿到this指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at, length));
}

/**
 * @brief 解析资源路径回调函数
 */
void on_request_path(void *data, const char *at, size_t length)
{
    //拿到this指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}

/**
 * @brief 解析查询参数回调函数
 */
void on_request_query_string(void *data, const char *at, size_t length)
{
    //拿到this指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}

/**
 * @brief 解析HTTP协议版本回调函数
 */
void on_request_http_version(void *data, const char *at, size_t length)
{
    //拿到this指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0)
    {
        v = 0x11;
    }
    else if(strncmp(at, "HTTP/1.0", length) == 0)
    {
        v = 0x10;
    }
    else
    {
        KIT_LOG_WARN(g_logger) << "http request version invaild:"
            << std::string(at, length);

        parser->setError(INVALID_VERSION);
        return;
    }

    parser->getData()->setVersion(v);
}

void on_request_header_done(void *data, const char *at, size_t length)
{

}

/**
 * @brief 解析一系列首部字段的回调函数
 */
void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen)
{
    //拿到this指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);

    // KIT_LOG_DEBUG(g_logger) << "request parser:\n" << value;

    if(flen == 0)
    {
        KIT_LOG_WARN(g_logger) << "http request field length=" << flen;
        /*不作为错误 处理 否则会返回nullptr*/
       // parser->setError(INVALID_FIELD);
        return;
    }

    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

/**************************************response回调************************************/

/**
 * @brief 解析状态原因短语回调函数
 */
void on_response_reason_phrase(void *data, const char *at, size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(std::string(at, length));
}

/**
 * @brief 解析状态码回调函数
 */
void on_response_status_code(void *data, const char *at, size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at));
    parser->getData()->setStatus(status);
}


void on_response_chunk_size(void *data, const char *at, size_t length)
{

}

/**
 * @brief 解析HTTP协议版本回调函数
 */
void on_response_http_version(void *data, const char *at, size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0)
    {
        v = 0x11;
    }
    else if(strncmp(at, "HTTP/1.0", length) == 0)
    {
        v = 0x10;
    }
    else
    {
        KIT_LOG_WARN(g_logger) << "http response version invaild:"
            << std::string(at, length);

        parser->setError(INVALID_VERSION);
        return;
    }

    parser->getData()->setVersion(v);
}


void on_response_header_done(void *data, const char *at, size_t length)
{

}

void on_response_last_chunk(void *data, const char *at, size_t length)
{

}


/**
 * @brief 解析一系列首部字段回调函数
 */
void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen)
{
    HttpResponseParser *parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0)
    {
        KIT_LOG_WARN(g_logger) << "http response field length=" << flen << "invaild: ";
        //parser->setError(INVALID_FIELD);
        return;
    }

    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}


/*************************************HttpRequestParser********************************/
HttpRequestParser::HttpRequestParser()
    :m_error(0)
{
    m_request.reset(new HttpRequest);
    // 调用初始化API
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query_string;
    m_parser.http_version = on_request_http_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;   //this指针放入
}

//执行解析动作 核心  1:解析成功 -1:解析有问题  >0已经处理的字节数 且data有效数据为len - v
size_t HttpRequestParser::execute(char* data, size_t len)
{
    size_t ret = http_parser_execute(&m_parser, data, len, 0);

    //先将解析过的空间挪走 防止缓存不够 但是仍然有数据位解析完成的情况
    memmove(data, data + ret, (len - ret));

    //返回实际解析过的字节数
    return ret;

}

//解析是否结束
int HttpRequestParser::isFinished()
{
    return http_parser_finish(&m_parser);
}

//解析是否出错
int HttpRequestParser::hasError()
{
    return m_error || http_parser_has_error(&m_parser);
}


uint64_t HttpRequestParser::getContentLength() 
{
    uint64_t v = 0;
    return m_request->GetHeaderAs<uint64_t>("content-length", v);
}

uint64_t HttpRequestParser::GetHttpRequestBufferSize()
{
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpMaxBodySize()
{
    return s_http_request_max_body_size;
}


/*************************************HttpResponseParser********************************/


HttpResponseParser::HttpResponseParser()
    :m_error(0)
{
    m_response.reset(new HttpResponse);

    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason_phrase;
    m_parser.status_code = on_response_status_code;
    m_parser.chunk_size = on_response_chunk_size;
    m_parser.http_version = on_response_http_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;   //this指针放入
}

//执行解析动作
size_t HttpResponseParser::execute(char* data, size_t len, bool chunck)
{
    //如果为chunck包需要重新初始化一下解析包
    if(chunck)
        httpclient_parser_init(&m_parser);

    //每一次都是从头开始解析
    size_t ret = httpclient_parser_execute(&m_parser, data, len, 0);
    //先将解析过的空间挪走 
    memmove((void *)data, data + ret, (len - ret));
    
    //返回实际解析过的字节数
    return ret;
}

//解析是否结束
int HttpResponseParser::isFinished()
{
    return httpclient_parser_finish(&m_parser);
}

//解析是否有错误
int HttpResponseParser::hasError()
{
    return m_error || httpclient_parser_has_error(&m_parser);
}

uint64_t HttpResponseParser::getContentLength() 
{
    uint64_t v = 0;
    return m_response->GetHeaderAs<uint64_t>("content-length", v);

}

uint64_t HttpResponseParser::GetHttpResponseBufferSize()
{
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpMaxBodySize()
{
    return s_http_response_max_body_size;
}

}
}