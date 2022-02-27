#ifndef _KIT_HTTP_H_
#define _KIT_HTTP_H_

#include <memory>
#include <string>
#include <string.h>
#include <map>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <stdint.h>




#include "http11_parser.h"
#include "httpclient_parser.h"

namespace kit_server
{
namespace http
{


 /* Request Methods 请求方法*/
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \




/* Status Codes  响应状态码*/
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

enum class HttpMethod
{
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

enum class HttpStatus
{
#define XX(code, name, string) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 * @brief 字符串转为HTTP请求方法枚举型
 * @param[in] method HTTP请求方法string
 * @return HttpMethod 
 */
HttpMethod StringToHttpMethod(const std::string &method);

/**
 * @brief 字符串转为HTTP请求方法枚举型
 * @param[in] method HTTP请求方法 char*
 * @return HttpMethod 
 */
HttpMethod CharsToHttpMethod(const char *method);

/**
 * @brief HTTP请求方法枚举型转为字符型
 * @param[in] method HTTP请求方法枚举型具体值
 * @return const char* 
 */
const char* HttpMethodToString(const HttpMethod &method);

/**
 * @brief HTTP响应码枚举型转为字符型
 * @param[in] status HTTP响应码枚举型具体值
 * @return const char* 
 */
const char* HttpStatusToString(const HttpStatus &status);

//无关大小写仿函数
struct CaseInsensitiveLess
{
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};


/**
 * @brief 用于对存储字段的map容器进行查询和获取以及类型转换
 * @tparam 具体的map类型
 * @tparam[in] T 期待转换的类型
 * @param[in] key 查询的首部字段 
 * @param[out] val 转换后的传出值
 * @param[in] def 默认转换类型
 * @return true 存在并转换为T
 * @return false 不存在并转换为默认类型
 */
template<class MapType, class T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T())
{
    std::string str;
    auto it = m.find(key);
    if(it == m.end())
    {
        val = def;
        return false;
    }

    try
    {
        //万能转换
        val = boost::lexical_cast<T>(it->second);
        return true;
    }
    catch(...)
    {
        val = def;
    }

    return false;
    
}

/**
 * @brief 主要用于对存储字段的map容器进行获取以及类型转换
 * @tparam 具体的map类型
 * @tparam[in] T 期待转换的类型
 * @param[in] key 查询的字段 
 * @param[in] def 默认转换类型
 * @return T 
 */
template<class MapType, class T>
T getAs(const MapType& m, const std::string& key, const T& def = T())
{
    auto it = m.find(key);
    if(it == m.end())
        return def;
    
    try
    {
        return boost::lexical_cast<T>(it->second);
    }
    catch(...)
    {
    }

    return def;
    
}



//HTTP请求类
class HttpRequest
{
public:
    typedef std::shared_ptr<HttpRequest> ptr;

    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief HTTP请求类构造函数
     * @param[in] version http协议版本，默认为1.1
     * @param[in] close 是否支持长连接，默认不支持 
     */
    HttpRequest(uint8_t version = 0x11, bool close = true);

    /**
     * @brief 设置请求方法
     * @param[in] m 具体请求方法
     */
    void setMethod(HttpMethod m) {m_method = m;}

    /**
     * @brief 获取请求方法
     * @return HttpMethod 
     */
    HttpMethod getMethod() const {return m_method;}

    /**
     * @brief 设置协议版本
     * @param[in] v 具体版本号
     */
    void setVersion(uint8_t v) {m_version = v;}

    /**
     * @brief 获取协议版本
     * @return uint8_t 
     */
    uint8_t getVersion() const {return m_version;}

    /**
     * @brief 设置资源路径
     * @param[in] s 具体资源路径
     */
    void setPath(const std::string& s) {m_path = s;}

    /**
     * @brief 获取资源路径
     * @return const std::string& 
     */
    const std::string& getPath() const {return m_path;}

    /**
     * @brief 设置查询字符串
     * @param[in] s 具体查询字符串
     */
    void setQuery(const std::string& s) {m_query = s;}

    /**
     * @brief 获取查询字符串 
     * @return const std::string& 
     */
    const std::string& getQuery() const {return m_query;}

    /**
     * @brief 设置片段标识符
     * @param[in] s 具体片段标识 
     */
    void setFragment(const std::string& s) {m_fragment = s;}

    /**
     * @brief 获取片段标识符
     * @return const std::string& 
     */
    const std::string& getFragment() const {return m_fragment;}

    /**
     * @brief 设置报文主体
     * @param[in] s 设置具体主体内容 
     */
    void setBody(const std::string& s) {m_body = s;}

    /**
     * @brief 获取报文主体
     * @return const std::string& 
     */
    const std::string& getBody() const {return m_body;}

    /**
     * @brief 设置报文首部字段组
     * @param[in] v 具体map容器 
     */
    void setHeaders(const MapType& v) {m_headers = v;}

    /**
     * @brief 获取报文首部字段组
     * @return const MapType& 
     */
    const MapType& getHeaders() const {return m_headers;}

    /**
     * @brief 设置参数组
     * @param[in] v 具体map容器 
     */
    void setParams(const MapType& v) {m_params = v;}

    /**
     * @brief 获取参数组
     * @return const MapType& 
     */
    const MapType& getParams() const {return m_params;}

    /**
     * @brief 设置cookie字段组
     * @param[in] v 具体map容器  
     */
    void setCookies(const MapType& v) {m_cookies = v;}

    /**
     * @brief 获取cookie字段组
     * @return const MapType& 
     */
    const MapType& getCookies() const {return m_cookies;}

    /**
     * @brief  获取连接状态
     * @return true 属于短连接
     * @return false 属于长连接
     */
    bool isClose() const {return m_close;}

    /**
     * @brief 设置连接状态
     * @param[in] v  true = 短连接 false = 长连接
     */
    void setClose(bool v) {m_close = v;}

    //设置/获取/删除/查询具体首部字段
    void setHeader(const std::string& key, const std::string& val);
    std::string getHeadr(const std::string& key, const std::string& def = "") const;
    void delHeader(const std::string& key);
    bool hasHeader(const std::string& key, std::string * val = nullptr);
    
    //设置/获取/删除/查询具体参数
    void setParam(const std::string& key, const std::string& val);
    std::string getParam(const std::string& key, const std::string& def = "") const;
    void delParam(const std::string& key);
    bool hasParam(const std::string& key, std::string * val = nullptr);

    //设置/获取/删除/查询具体cookie
    void setCookie(const std::string& key, const std::string& val);
    std::string getCookie(const std::string& key, const std::string& def = "") const;
    void delCookie(const std::string& key);
    bool hasCookie(const std::string& key, std::string * val = nullptr);
    
    /**
     * @brief 查询并将存在的首部字段转换  sting---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的首部字段 
     * @param[out] val 转换后的传出值
     * @param[in] def 默认转换类型
     * @return true 存在并转换为T
     * @return false 不存在并转换为默认类型
     */
    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(m_headers, key, val, def);
    }

    /**
     * @brief 将存在的首部字段转换 string---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的首部字符 
     * @param[in] def 默认转换类型
     * @return T 返回转换为T类型的数据
     */
    template<class T>
    T GetHeaderAs(const std::string& key, const T& def = T())
    {
        return getAs(m_headers, key, def);
    }

    /**
     * @brief 查询并将存在的参数字段转换  sting---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的参数字段 
     * @param[out] val 转换后的传出值
     * @param[in] def 默认转换类型
     * @return true 存在并转换为T
     * @return false 不存在并转换为默认类型
     */
    template<class T>
    bool checkGetParamAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(m_params, key, val, def);
    }

    /**
     * @brief 将存在的参数字段转换 string---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的参数字段 
     * @param[in] def 默认转换类型
     * @return T 返回转换为T类型的数据
     */
    template<class T>
    T GetParamAs(const std::string& key, const T& def = T())
    {
        return getAs(m_params, key, def);
    }

    /**
     * @brief 查询并将存在的cookie字段转换  sting---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的cookie字段
     * @param[out] val 转换后的传出值
     * @param[in] def 默认转换类型
     * @return true 存在并转换为T
     * @return false 不存在并转换为默认类型
     */
    template<class T>
    bool checkGetCookieAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(m_params, key, val, def);
    }

    /**
     * @brief 将存在的cookie字段段转换 string---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的cookie字段 
     * @param[in] def 默认转换类型
     * @return T 返回转换为T类型的数据
     */
    template<class T>
    T GetCookieAs(const std::string& key, const T& def = T())
    {
        return getAs(m_params, key, def);
    }

    /**
     * @brief 将变量信息以流的形式重新组装为HTTP请求报文
     * @param[in] os 标准输出流 
     * @return std::ostream& 
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 将HTTP请求报文以字符串形式输出
     * @return std::string 
     */
    std::string toString() const;

    void init();

private:
    //请求方法
    HttpMethod m_method;
    //协议版本 0x11----http1.1  0x10-----http1.0
    uint8_t m_version;  
    //是否处于长连接
    bool m_close;

    /*URI 资源定位符*/
    //资源路径
    std::string m_path;
    //查询字符串
    std::string m_query;
    //片段标识
    std::string m_fragment;

    //报文主体
    std::string m_body;

    //首部字段
    std::map<std::string, std::string, CaseInsensitiveLess> m_headers;
    //参数字段
    std::map<std::string, std::string, CaseInsensitiveLess> m_params;
    //cookie字段
    std::map<std::string, std::string, CaseInsensitiveLess> m_cookies;

};
std::ostream& operator<<(std::ostream& os, const HttpRequest& req);

//HTTP响应类
class HttpResponse
{
public:
    typedef std::shared_ptr<HttpResponse> ptr;
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief HTTP响应类构造函数
     * @param[in] version http协议版本，默认为1.1
     * @param[in] close 是否支持长连接，默认不支持 
     */
    HttpResponse(uint8_t version = 0x11, bool close = true);

    /**
     * @brief 设置响应状态码
     * @param[in] v 具体响应状态码
     */
    void setStatus(HttpStatus v) {m_status = v;}

    /**
     * @brief 获取响应状态码
     * @return HttpStatus 
     */
    HttpStatus getStatus() const {return m_status;}
    
    /**
     * @brief 设置HTTP协议版本
     * @param[in] v 具体版本号
     */
    void setVersion(uint8_t v) {m_version = v;}

    /**
     * @brief 获取HTTP协议版本
     * @return uint8_t 
     */
    uint8_t getVersion() const {return m_version;}

    /**
     * @brief 设置响应报文主体
     * @param[in] v 具体报文内容
     */
    void setBody(const std::string& v) {m_body = v;}

    /**
     * @brief 获取响应报文主体
     * @return const std::string& 
     */
    const std::string& getBody() const {return m_body;}

    /**
     * @brief 设置响应原因短语
     * @param[in] v 具体短语内容
     */
    void setReason(const std::string& v) {m_reason = v;}

    /**
     * @brief 获取响应原因短语
     * @return const std::string& 
     */
    const std::string& getReason() const {return m_reason;}

    /**
     * @brief 设置响应首部字段组
     * @param[in] v 具体map容器
     */
    void setHeaders(const MapType& v) {m_headers = v;}

    /**
     * @brief 获取响应首部字段组
     * @return const MapType& 
     */
    const MapType& getHeaders() const {return m_headers;}

    /**
     * @brief 获取连接状态
     * @return true 属于短连接
     * @return false 属于长连接
     */
    bool isClose() const {return m_close;}

    /**
     * @brief 设置连接状态
     * @param[in] v  true = 短连接  false = 长连接
     */
    void setClose(bool v) {m_close = v;}

    void setHeader(const std::string& key, const std::string& val);
    std::string getHeader(const std::string& key, const std::string& def = "") const;
    void delHeader(const std::string& key);

    /**
     * @brief 查询并将存在的首部字段转换  sting---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的首部字段 
     * @param[out] val 转换后的传出值
     * @param[in] def 默认转换类型
     * @return true 存在并转换为T
     * @return false 不存在并转换为默认类型
     */
    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(m_headers, key, val, def);
    }

    /**
     * @brief 将存在的首部字段转换 string---->T
     * @tparam[in] T 期待转换的类型
     * @param[in] key 查询的首部字符 
     * @param[in] def 默认转换类型
     * @return T 返回转换为T类型的数据
     */
    template<class T>
    T GetHeaderAs(const std::string& key, T& val, const T& def = T())
    {
        return getAs(m_headers, key, def);
    }

    /**
     * @brief 将变量信息以流的形式重新组装为HTTP响应报文
     * @param[in] os 标准输出流 
     * @return std::ostream& 
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 将HTTP响应报文以字符串形式输出
     * @return std::string 
     */
    std::string toString() const;

private:
    //响应状态码
    HttpStatus m_status;
    //协议版本  0x10----HTTP1.0  0x11-----HTTP1.1
    uint8_t m_version;
    //是否为长连接 
    bool m_close;

    //响应报文主体
    std::string m_body;
    //响应状态原因短语
    std::string m_reason;
    //响应首部字段
    std::map<std::string, std::string, CaseInsensitiveLess> m_headers;
};

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);




}
}


#endif