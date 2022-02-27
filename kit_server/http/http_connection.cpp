#include "http_connection.h"
#include "http_parser.h"

#include <errno.h>

namespace kit_server
{
namespace http
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


std::string HttpResult::toString() const
{
    std::stringstream ss;
    ss << "[HttpResult result=" << result
        << ", error_str=]" << error 
        << ", response=\n" << (response ? response->toString() : "nullptr");
    
    return ss.str();
}


/************************************HttpConnection**********************************************/


HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner)
    ,m_createTime(-1)
    ,m_requestCount(0)
{


}

HttpConnection:: ~HttpConnection()
{
    KIT_LOG_INFO(g_logger) << "HttpConnection:: ~HttpConnection work";
}

//收到HTTP响应报文
HttpResponse::ptr HttpConnection::recvResponse()
{
    HttpResponseParser::ptr parser(new HttpResponseParser);
    //使用某一时刻的值即可 不需要实时的值
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();

    //创建一个接受响应报文的缓冲区 指定析构
    std::shared_ptr<char> buffer(new char[buff_size + 1], [](char *ptr){
        delete[] ptr;
    }); 

    char *data = buffer.get();
    int offset = 0;
    
    //一边读一遍解析
    while(1)
    {
        int ret = read(data + offset, buff_size - offset);
        if(ret <= 0)
        {
            KIT_LOG_ERROR(g_logger) << "HttpConnection::recvResponse read error, errno=" << errno
                << ", is:" << strerror(errno);
            close();
            return nullptr;
        }

       //KIT_LOG_DEBUG(g_logger) << "读取的原始数据, size=" << strlen(data) << "\n" << data;

        ret += offset;
        data[ret] = '\0';
        size_t n = parser->execute(data, ret, false);
        //KIT_LOG_DEBUG(g_logger) << "解析后的数据:\n" <<  data;
        // KIT_LOG_DEBUG(g_logger) << "execute n= " << n
        // << ",has error=" << parser->hasError()
        // << ",is finished=" << parser->isFinished()
        // << ", total len=" << ret
        // << ", content length=" << parser->getContenLength()
        // << ", data=\n" << parser->getData()->toString(); 

        if(parser->hasError())
        {   
            KIT_LOG_ERROR(g_logger) << "HttpConnection::recvResponse parser error";
            close();
            return nullptr;
        }

        
        offset = ret - n;
        if(offset == (int)buff_size)
        {
            KIT_LOG_WARN(g_logger) << "HttpConnection::recvResponse http response buffer out of  range";
            close();
            return nullptr;
        }

        //如果解析已经结束
        if(parser->isFinished())
            break;
    
    }

   // KIT_LOG_DEBUG(g_logger) << "已经解析得到的报文:\n" << parser->getData()->toString();

    
    auto& client_parser = parser->getParser();
    //如果发送方式为chunked 分块发送 就需要分块接收
    if(client_parser.chunked)
    {   
        std::string body;
        //拿到剩余报文的长度
        int len = offset;
        
        do
        {
            do
            {
                //尝试读取一次 剩余的缓存空间大小的数据 尽可能的多读出主体
                int ret = read(data + len, buff_size - len);
                if(ret <= 0)
                {
                    KIT_LOG_ERROR(g_logger) << "HttpConnection::recvResponse read error, errno=" << errno
                        << ", is:" << strerror(errno);
                    close();
                    return nullptr;
                }
            

                // KIT_LOG_DEBUG(g_logger) << "读取的原始数据, size=" << strlen(data) << "\n" << data;

                len += ret;
                data[len] = '\0';

                //开启分块解析
                size_t n = parser->execute(data, len, true);
                if(parser->hasError())
                {   
                    KIT_LOG_ERROR(g_logger) << "HttpConnection::recvResponse parser error";
                    close();
                    return nullptr;
                }

                len -= n;
                if(len == (int)buff_size)   //读了数据但是没有解析  直到自己定义的缓存已经满了
                {
                    KIT_LOG_WARN(g_logger) << "HttpConnection::recvResponse http response buffer out of  range";
                    close();
                    return nullptr;
                }

         
            } while (!parser->isFinished());     //当前分块是否已经解析完毕

            /*这个减2非常关键 因为报文首部和实体中间还有一个空行\r\n将它算作首部的长度 不应算在body的长度中*/
            len -= 2;


            // KIT_LOG_DEBUG(g_logger) << "一块报文长度: " << client_parser.content_len; 

            //如果当前解析包中的主体长度小于 当前已经接受到的并且解析好的报文主体长度
            if(client_parser.content_len <= len)
            {
                body.append(data, client_parser.content_len);
                //将已经装入的部分的内存 移动覆盖前面的内存
                memmove(data, data + client_parser.content_len, len - client_parser.content_len);
                len -= client_parser.content_len;
            }
            else    //否则当前解析包中的主体长度 大于 当前接收到的并且解析好的报文主体长度
            {
                body.append(data, len);

                //计算还有多少报文主体未接收到
                int left = client_parser.content_len - len;
                // KIT_LOG_DEBUG(g_logger) << "剩余未接收到的报文长度:" << left;
                while(left > 0)
                {
                    //继续接收剩余的报文主体
                    int ret = read(data, left > (int)buff_size ? (int)buff_size : left);
                    if(ret <= 0)
                    {
                        KIT_LOG_ERROR(g_logger) << "HttpConnection::recvResponse read error, errno=" << errno
                            << ", is:" << strerror(errno);
                        close();
                        return nullptr;
                    }

                    body.append(data, ret);
                    left -= ret;
                }

                len = 0;
            }
            
        } while (!client_parser.chunks_done);   //所有分块是否全部接受完毕
        
        parser->getData()->setBody(body);
    }
    else
    {
        //获取实体长度
        int64_t len = parser->getContentLength();

        //将报文实体读出 并且设置到HttpRequest对象中去
        if(len > 0)
        {
            std::string body;
            body.resize(len);

            int real_len = 0;

            //这里offset 就是报文首部解析完之后 剩余的报文实体的字节数
            if(len >= offset)
            {
                memcpy(&body[0], data, offset);
                //body.append(data, offset);
                real_len = offset;
            }
            else
            {
                memcpy(&body[0], data, len);
                // body.append(data, len);
                real_len = len;

            }


            len -= offset;
            //如果还有剩余的报文实体长度 说明报文实体没有在本次传输中全部被接收
            //设置一个循环继续接收报文实体
            if(len > 0)
            {
                if(readFixSize(&body[real_len], real_len) <= 0)
                {
                    KIT_LOG_ERROR(g_logger) << "HttpConnection::recvResponse readFixSize error, errno=" << errno
                        << ", is:" << strerror(errno);
                    close();
                    return nullptr;
                }
            }

            //等到报文实体完整之后 装入对象之中 
            parser->getData()->setBody(body);
        }
    }
    
    return parser->getData();
}

//发送HTTP请求报文
int HttpConnection::sendRequest(HttpRequest::ptr rsp)
{
    std::stringstream ss;
    ss << *rsp;

    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}



/*便捷的创建HTTP请求*/
HttpResult::ptr HttpConnection::DoGet(const std::string& url, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    //通过字符串解析并创建URI对象
    Uri::ptr uri = Uri::Create(url);
    if(!uri)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnection::DoGet Uri parser error";
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }

    return DoGet(uri, timeout_ms, headers, body);

}

HttpResult::ptr HttpConnection::DoPost(const std::string& url, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    //通过字符串解析并创建URI对象
    Uri::ptr uri = Uri::Create(url);
    if(!uri)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnection::DoPost Uri parser error";
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }

    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const std::string& url, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    //通过字符串解析并创建URI对象
    Uri::ptr uri = Uri::Create(url);
    if(!uri)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnection::DoRequest Uri parser error";
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }

    return DoRequest(method, uri, timeout_ms, headers, body);

}


HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, Uri::ptr uri, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    /*组装HTTP请求报文*/
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);                 //设置请求方法
    req->setPath(uri->getPath());           //设置请求资源路径  
    req->setQuery(uri->getQuery());         //设置查询参数
    req->setFragment(uri->getFragment());   //设置分段标识符
    bool has_host = false;
    for(auto &x : headers)
    {
        if(strcasecmp(x.first.c_str(), "connection") == 0 && 
            strcasecmp(x.second.c_str(), "keep-alive") == 0)
        {
                req->setClose(false);
        }

        if(has_host && strcasecmp(x.first.c_str(), "host") == 0)
            has_host = !x.second.empty();
        
        req->setHeader(x.first, x.second);
    }
    if(!has_host)
        req->setHeader("host", uri->getHost());
    
    req->setBody(body);

    return DoRequest(req, uri, timeout_ms);
}

//负责转发 修改一下从客户端收到的HTTP请求就将其转发出去
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms)
{
    //通过URI对象智能指针创建地址对象
    Address::ptr addr = uri->createAddress();
    KIT_LOG_DEBUG(g_logger) << "DoRequest addr=" << *addr;
    if(!addr)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnection::DoRequest get addr fail";
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_ADDR, nullptr, "invalid addr=" + uri->getHost());
    }
    
    //通过地址对象创建套接字对象
    Socket::ptr sock = Socket::CreateTCP(addr);
    if(!sock)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnection::DoRequest create socket fail";
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_SOCK, nullptr, "invalid sock addr=" + addr->toString());
    }
    
    //通过套接字连接目标服务器
    if(!sock->connect(addr))
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnection::DoRequest socket connect fail";
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "invalid sock addr=" + addr->toString());
    }
    //为套接字设置接收超时时间
    sock->setRecvTimeout(timeout_ms);

    //通过连接成功的套接字创建HTTP连接
    HttpConnection::ptr con = std::make_shared<HttpConnection>(sock);
    
    //通过连接发送HTTP请求报文
    int ret = con->sendRequest(req);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnection::DoRequest write fail, errno=" << errno
            << ", is:" << strerror(errno);

        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_REQ_FAIL, nullptr, "send request fail, addr=" + addr->toString() + ", errno=" + std::to_string(errno) + ",is:" + std::string(strerror(errno)));
    }

    //通过连接接收HTTP请求报文
    auto rsp = con->recvResponse();
    if(!rsp)
    {
        KIT_LOG_WARN(g_logger) << "HttpConnection::DoRequest recv response maybe timeout";

        return std::make_shared<HttpResult>((int)HttpResult::Error::RECV_RSP_TIMEOUT, nullptr, "recv response maybe timeout, addr=" + addr->toString() + ", timeout=" + std::to_string(timeout_ms));
    }

    //返回HTTP处理结果的结构体指针指针
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}


/***************************************HttpConnectionPool**********************************************/
HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost, 
    uint16_t port, uint32_t max_size, uint32_t maxAliveTime, uint32_t maxRequest)
        :m_host(host)
        ,m_vhost(vhost)
        ,m_port(port)
        ,m_maxSize(max_size)
        ,m_maxAliveTime(maxAliveTime)
        ,m_maxRequest(maxRequest)
{


}
//获取连接
HttpConnection::ptr HttpConnectionPool::getConnection()
{
    //收集过期连接vector容器
    std::vector<HttpConnection*> invalid_connections;
    //返回出去的可用连接
    HttpConnection* con_ptr = nullptr;

    MutexType::Lock lock(m_mutex);
    //从连接池里找出一个可用的资源
    while(m_connections.size())
    {
        auto con = m_connections.front();
        m_connections.pop_front();

        //当前连接已经断开 视为过期
        if(!con->isConnected())
        {
            invalid_connections.push_back(con);
            continue;
        }

        //当前连接超过了最大存活时间 视为过期
        if(con->getCreateTime() + m_maxAliveTime >= GetCurrentMs())
        {
            invalid_connections.push_back(con);
            continue;
        }

        con_ptr = con;
        break;

    }
    lock.unlock();

    //将过期连接资源都要释放
    for(auto &x : invalid_connections)
        delete x;
    
    m_total -= invalid_connections.size();

    //如果在连接池里没有拿到对应的资源 线程创建一个
    if(!con_ptr)
    {
        //通过域名创建通信地址对象
        IPAddress::ptr addr = Address::LookUpAnyIPAddress(m_host);
        if(!addr)
        {
            KIT_LOG_ERROR(g_logger) << "HttpConnectionPool::getConnection get addr fail, host=" << m_host << ", port=" << m_port;
            return nullptr;
        }
        
        //给地址设置端口号
        addr->setPort(m_port);
        //通过通信地址对象创建一个TCP套接字
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock)
        {
            KIT_LOG_ERROR(g_logger) << "HttpConnectionPool::getConnection create socket fail, addr=" << *addr;
            return nullptr;
        }

        //通过套接字连接目标服务器
        if(!sock->connect(addr))
        {
            KIT_LOG_ERROR(g_logger) << "HttpConnectionPool::getConnection connect fail, addr=" << *addr;
            return nullptr;
        }

        //通过连接好的套接字创建一条HTTP连接
        con_ptr = new HttpConnection(sock);
        //连接数+1
        ++m_total;
    }

    //指定一个用于释放的函数HttpConnectionPool::ReleasePtr
    return HttpConnection::ptr(con_ptr, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));

}

HttpResult::ptr HttpConnectionPool::doGet(const std::string& url, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{   
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& url, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    std::stringstream ss;
    ss << uri->getPath()
        << (uri->getQuery().size() ? "?" : "")
        << uri->getQuery()
        << (uri->getFragment().size() ? "#" : "")
        << uri->getFragment();
    
    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    std::stringstream ss;
    ss << uri->getPath()
        << (uri->getQuery().size() ? "?" : "")
        << uri->getQuery()
        << (uri->getFragment().size() ? "#" : "")
        << uri->getFragment();
    
    return doPost(ss.str(), timeout_ms, headers, body);
}


HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string& url, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{

    /*组装HTTP请求报文*/
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);
    req->setPath(url);
    req->setClose(false);

    bool has_host = false;  //域名标志位
    for(auto &x : headers)
    {
        if(strcasecmp(x.first.c_str(), "connection") == 0 && 
            strcasecmp(x.second.c_str(), "keep-alive") == 0)
        {
                req->setClose(false);
        }

        //是否存在host域名字段
        if(has_host && strcasecmp(x.first.c_str(), "host") == 0)
            has_host = !x.second.empty();
        
        req->setHeader(x.first, x.second);
    }

    //没有带域名字段 要使用备用域名m_vhost
    if(!has_host)
    {
        if(!m_vhost.size())
            req->setHeader("host", m_host);
        else 
            req->setHeader("host", m_vhost);
    }
    
    req->setBody(body);

    return doRequest(req, timeout_ms);

}


HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, Uri::ptr uri, 
    uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body)
{
    std::stringstream ss;
    //组装URI字符串
    ss << uri->getPath()
        << (uri->getQuery().size() ? "?" : "")
        << uri->getQuery()
        << (uri->getFragment().size() ? "#" : "")
        << uri->getFragment();
    
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}


HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout_ms)
{
    //获取HTTP连接资源
    auto con = getConnection();
    if(!con)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnectionPool::doRequest get connection from pool fail";

        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECT_FAIL, nullptr, "get connection from pool fail, host=" + m_host + ", port=" + std::to_string(m_port));
    }

    //通过连接获取到套接字
    auto sock = con->getSocket();
    if(!sock)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnectionPool::doRequest get sock fail";

        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_SOCK, nullptr, "send request fail, host=" + m_host + ", port=" + std::to_string(m_port));
    }
    //设置接收超时时间
    sock->setRecvTimeout(timeout_ms);

    //通过连接发送HTTP请求报文
    int ret = con->sendRequest(req);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "HttpConnectionPool::DoRequest write fail, errno=" << errno
            << ", is:" << strerror(errno);

        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_REQ_FAIL, nullptr, "send request fail, addr=" + con->getSocket()->getRemoteAddress()->toString() + ", port=" + std::to_string(m_port) + ", errno=" + std::to_string(errno) + ",is:" + std::string(strerror(errno)));
    }

    //通过连接接收HTTP响应报文
    auto rsp = con->recvResponse();
    if(!rsp)
    {
        KIT_LOG_WARN(g_logger) << "HttpConnectionPool::doRequest recv response maybe timeout, time=" << timeout_ms;

        return std::make_shared<HttpResult>((int)HttpResult::Error::RECV_RSP_TIMEOUT, nullptr, "recv response maybe timeout, ,time=" + std::to_string(timeout_ms) + ",addr=" + con->getSocket()->getRemoteAddress()->toString() + ", errno=" + std::to_string(errno) + ",is:" + std::string(strerror(errno)));
    }

    //返回连接上的收发结果
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool)
{
    //连接上的请求数+1
    ptr->addRequestCount();
    //判断当前连接是否过期:是否处于连接 || 是否超过存活时间 || 是否超出最大请求数 || 是否超出最大连接数
    if(!ptr->isConnected() || 
        (ptr->getCreateTime() + pool->m_maxAliveTime >= GetCurrentMs()) ||
        (ptr->getRequestCount() > pool->m_maxRequest) || 
        (pool->m_total > (int32_t)pool->m_maxSize))
    {
        delete ptr;
        --pool->m_total;
        return;
    }

    //连接未过期 把用完的连接放回连接池
    MutexType::Lock lock(pool->m_mutex);
    pool->m_connections.push_back(ptr);
}


}
}