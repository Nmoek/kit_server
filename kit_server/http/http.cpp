#include "http.h"
#include "../Log.h"

namespace kit_server
{
namespace http
{
//字符串转枚举
HttpMethod StringToHttpMethod(const std::string &method)
{
#define XX(num, name, string)\
    if(strcmp(#string, method.c_str()) == 0)\
        return HttpMethod::name;\

    HTTP_METHOD_MAP(XX);
#undef XX

    return HttpMethod::INVALID_METHOD;

}

HttpMethod CharsToHttpMethod(const char *method)
{
#define XX(num, name, string)\
    if(strncmp(#string, method, strlen(#string)) == 0)\
        return HttpMethod::name;\

    HTTP_METHOD_MAP(XX)
#undef XX

    return HttpMethod::INVALID_METHOD;
}

/**
 * @brief 辅助数组，将HTTP请求方法由枚举转为数组方便访问
 */ 
static const char* s_method_string[] = {
#define XX(num, name, string)   #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};


//枚举转字符串
const char* HttpMethodToString(const HttpMethod &method)
{
    uint32_t index = (uint32_t)method;
    if(index >= sizeof(s_method_string) / sizeof(s_method_string[0]))
    {
        return "<unknown>";
    }

    return s_method_string[index];
}

const char* HttpStatusToString(const HttpStatus &status)
{
    switch(status)
    {
#define XX(code, name, string)\
        case HttpStatus::name: \
            return #string;

        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}


bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const
{
    //忽视字母大小写比较
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

/****************************************HttpRequest**************************************/
HttpRequest::HttpRequest(uint8_t version, bool close)
    :m_method(HttpMethod::GET)
    ,m_version(version)
    ,m_close(close)
    ,m_path("/")    //默认是根路径
{
    
}

static Logger::ptr g_logger = KIT_LOG_NAME("system");
void HttpRequest::setHeader(const std::string& key, const std::string& val)
{
    m_headers[key] = val;
}

std::string HttpRequest::getHeadr(const std::string& key, const std::string& def) const
{
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpRequest::delHeader(const std::string& key)
{
    m_headers.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key, std::string * val)
{
    auto it = m_headers.find(key);
    if(it == m_headers.end())
        return false;
    
    if(val)
        *val = it->second;

    return true;
}

void HttpRequest::setParam(const std::string& key, const std::string& val)
{
    m_params[key] = val;
}

std::string HttpRequest::getParam(const std::string& key, const std::string& def) const
{
    auto it = m_params.find(key);
    return it == m_params.end() ? def : it->second;
}

void HttpRequest::delParam(const std::string& key)
{
    m_params.erase(key);
}

bool HttpRequest::hasParam(const std::string& key, std::string * val)
{
    auto it = m_params.find(key);
    if(it == m_params.end())
        return false;
    
    if(val)
        *val = it->second;
        
    return true;
}

void HttpRequest::setCookie(const std::string& key, const std::string& val)
{
    m_cookies[key] = val;
}

std::string HttpRequest::getCookie(const std::string& key, const std::string& def) const
{
    auto it = m_cookies.find(key);
    return it == m_cookies.end() ? def : it->second;
}

void HttpRequest::delCookie(const std::string& key)
{
    m_cookies.erase(key);
}
bool HttpRequest::hasCookie(const std::string& key, std::string * val)
{
    auto it = m_cookies.find(key);
    if(it == m_cookies.end())
        return false;
    
    if(val)
        *val = it->second;
        
    return true;

}

//报文封装
std::ostream& HttpRequest::dump(std::ostream& os) const
{
    // GET /uri HTTP/1.1
    //Host: www.baidu.com
    //空行CR+LF
    //实体
    
    //请求行封装
    os << HttpMethodToString(m_method) << " "
        << m_path
        << (m_query.size() ? "?" : "")
        << m_query
        << (m_fragment.size() ? "#" : "")
        << m_fragment
        << " HTTP/"
        << ((uint32_t)(m_version >> 4))     //把低4位移走 只保留高4位
        << "."
        << ((uint32_t)(m_version & 0x0F))   //把高4位置0  只保留低4位
        << "\r\n";


    //连接状态单独列举
    os << "connection:" << (m_close ? "close" : "keep-alive") << "\r\n";

    //请求首部封装
    for(auto &x : m_headers)
    {
        if(strcasecmp(x.first.c_str(), "connection") == 0)
            continue;

        os << x.first << ":" << x.second << "\r\n";
    }


    //报文实体封装
    if(m_body.size())
        os << "content-length: " << m_body.size() << "\r\n\r\n"
            << m_body;
    else
        os << "\r\n";
    

    return os;
}


std::string HttpRequest::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


void HttpRequest::init()
{
    std::string connection = getHeadr("connection");
    if(connection.size())
    {
        if(strcasecmp(connection.c_str(), "keep-alive") == 0)
            m_close = false;
        else
            m_close = true;
    }
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req)
{
    return req.dump(os);
}





/****************************************HttpResponse**************************************/
HttpResponse::HttpResponse(uint8_t version, bool close)
    :m_status(HttpStatus::OK)
    ,m_version(version)
    ,m_close(close)
{

}

void HttpResponse::setHeader(const std::string& key, const std::string& val)
{
    m_headers[key] = val;
}

std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const
{
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpResponse::delHeader(const std::string& key)
{
    m_headers.erase(key);
}

//报文封装
std::ostream& HttpResponse::dump(std::ostream& os) const
{
    // HTTP/1.1 200 OK

    //响应行封装
    os << "HTTP/"
        << ((uint32_t)(m_version >> 4))
        << "."
        << ((uint32_t)(m_version & 0x0F))
        << " "
        << (uint32_t)m_status
        << " "
        << (m_reason.size() ? m_reason : HttpStatusToString(m_status))
        << "\r\n";


    //响应首部封装
    for(auto &x : m_headers)
    {
        if(strcasecmp(x.first.c_str(), "connection") == 0)
            continue;
        os << x.first << ":" << x.second << "\r\n";
    }

    //单独封装连接状态
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

    //报文主体封装
    if(m_body.size())
        os << "content-length: " << m_body.size() << "\r\n\r\n"
            << m_body;
    else
        os << "\r\n";
    
    return os;
    
}

std::string HttpResponse::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp)
{
    return rsp.dump(os);
}

}
}