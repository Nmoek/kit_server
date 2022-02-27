#include "servlet.h"
#include "../Log.h"

#include <fnmatch.h>

namespace kit_server
{
namespace http
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

/*************************************FunctionServlet**************************************/

FunctionServlet::FunctionServlet(callback cb, const std::string& name)
    :Servlet(name)
    ,m_cb(cb)
{

}

int32_t FunctionServlet::handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session)
{
    return m_cb(request, response, session);
}

/*************************************ServletDispatch**************************************/
ServletDispatch::ServletDispatch(const std::string& name)
    :Servlet(name)
{
    m_default.reset(new NotFoundServlet("kit_server/1.0.0"));
}

//服务执行
int32_t ServletDispatch::handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session)
{
    //KIT_LOG_DEBUG(g_logger) << "获得路径:" << request->getPath();
    auto slt = getMatchedServlet(request->getPath());
    if(slt)
        slt->handle(request, response, session);
    
    return 0;
}   


//添加服务
void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt)
{
    MutexType::WriteLock lock(m_mutex);
    m_datas[uri] = slt;
}

void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::callback cb)
{
    MutexType::WriteLock lock(m_mutex);
    m_datas[uri].reset(new FunctionServlet(cb));
}

void ServletDispatch::addGlobServlet(const std::string& uri, Servlet::ptr slt)
{
    MutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end();++it)
    {
        if(it->first == uri)
        {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back({uri, slt});
}

void ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::callback cb)
{
    addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

//删除精准匹配服务
void ServletDispatch::delServlet(const std::string& uri)
{
    MutexType::WriteLock lock(m_mutex);
    m_datas.erase(uri);
}

//删除模糊匹配服务
void ServletDispatch::delGlobServlet(const std::string& uri)
{
    MutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end();++it)
    {
        if(it->first == uri)
        {
            m_globs.erase(it);
            break;
        }
    }
}


//获取精准匹配服务对象
Servlet::ptr ServletDispatch::getServlet(const std::string& uri)
{
    MutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

//获取模糊匹配服务对象
Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri)
{
    MutexType::ReadLock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end();++it)
    {
        if(it->first == uri)
            return it->second;
    }
    return nullptr;
}

//获取任意一个符合的服务对象
Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri)
{
    MutexType::ReadLock lock(m_mutex);
    //先在精准匹配中找，再从模糊匹配中找 都没有返回默认的
    auto it = m_datas.find(uri);
    if(it != m_datas.end())
        return it->second;
    
    for(auto it2 = m_globs.begin();it2 != m_globs.end();++it2)
    {
        if(fnmatch(it2->first.c_str(), uri.c_str(), 0) == 0)
            return it2->second;
    }
    
    return m_default;
}

/*************************************NotFoundServlet**************************************/


NotFoundServlet::NotFoundServlet(const std::string& serverName, const std::string& name)
    :Servlet(name)
    ,m_serverName(serverName)
{
    m_content = 
    "<html>"
        "<head>"
            "<title>404 Not Found</title>"
        "</head>"
        "<body>"
            "<center><h1>404 Not Found</hl></center>"
            "<hr><center>"+ m_serverName + "</center></hr>"
        "</body>"
    "</html>";

}

int32_t NotFoundServlet::handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session)
{
    response->setStatus(HttpStatus::NOT_FOUND);
    response->setHeader("Server", m_serverName);
    response->setHeader("Content-type", "text/html");
    response->setBody(m_content);
    
    return 0;
}

}
}