#include "http_session.h"
#include "http_parser.h"

#include <errno.h>

namespace kit_server
{
namespace http
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner)
{


}

//收到HTTP请求报文
HttpRequest::ptr HttpSession::recvRequest()
{
    HttpRequestParser::ptr parser(new HttpRequestParser);
    //使用某一时刻的值即可 不需要实时的值
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    // uint64_t buff_size = 300;
    // uint64_t buff_size = 100;
    //创建一个接受请求报文的缓冲区 指定析构
    std::shared_ptr<char> buffer(new char[buff_size], [](char *ptr){
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
            KIT_LOG_ERROR(g_logger) << "HttpSession::recvRequest read error, errno=" << errno
                << ", is:" << strerror(errno);
            close();
            return nullptr;
        }

       //KIT_LOG_DEBUG(g_logger) << "读取的原始数据, size=" << strlen(data) << "\n" << data;

        ret += offset;

        size_t n = parser->execute(data, ret);
        //KIT_LOG_DEBUG(g_logger) << "解析后的数据:\n" <<  data;
        // KIT_LOG_DEBUG(g_logger) << "execute n= " << n
        // << ",has error=" << parser->hasError()
        // << ",is finished=" << parser->isFinished()
        // << ", total len=" << ret
        // << ", content length=" << parser->getContenLength()
        // << ", data=\n" << parser->getData()->toString(); 

        if(parser->hasError())
        {   
            KIT_LOG_ERROR(g_logger) << "HttpSession::recvRequest parser error";
            close();
            return nullptr;
        }

        
        offset = ret - n;
        if(offset == (int)buff_size)
        {
            KIT_LOG_WARN(g_logger) << "HttpConnection::recvRequest http request buffer out of  range";
            close();
            return nullptr;
        }

        //如果解析已经结束
        if(parser->isFinished())
            break;
    
    }

    KIT_LOG_DEBUG(g_logger) << "已经解析得到的报文:" << parser->getData()->toString();

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
                KIT_LOG_ERROR(g_logger) << "HttpSession::recvRequest readFixSize error, errno=" << errno
                    << ", is:" << strerror(errno);
                close();
                return nullptr;
            }
        }

        //等到报文实体完整之后 装入对象之中 
        parser->getData()->setBody(body);
    }
    parser->getData()->init();
    
    return parser->getData();
}

//发回HTTP响应报文
int HttpSession::sendResponse(HttpResponse::ptr rsp)
{
    std::stringstream ss;
    ss << *rsp;

    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}



}
}