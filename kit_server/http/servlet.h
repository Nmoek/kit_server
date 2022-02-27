#ifndef _KIT_HTTP_SERVLET_H_
#define _KIT_HTTP_SERVLET_H_


#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "http.h"
#include "http_session.h"
#include "../mutex.h"

namespace kit_server
{
namespace http
{

class Servlet
{
public:
    typedef std::shared_ptr<Servlet> ptr;  

    Servlet(const std::string& name):m_name(name) { }

    virtual ~Servlet() {}

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) = 0;

    const std::string& getName() const {return m_name;}

protected:
    std::string m_name;
};


class FunctionServlet: public Servlet
{
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (HttpRequest::ptr request, 
        HttpResponse::ptr response, HttpSession::ptr session)> callback;

    FunctionServlet(callback cb, const std::string& name = "FunctionServlet");

    int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

private:
    callback m_cb;

};


class ServletDispatch: public Servlet
{
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex MutexType;

    ServletDispatch(const std::string& name = "ServletDispatch");

    //服务执行
    int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

    //添加服务
    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    //删除精准匹配服务
    void delServlet(const std::string& uri);
    //删除模糊匹配服务
    void delGlobServlet(const std::string& uri);
    
    //获取默认服务
    Servlet::ptr getDefault() const {return m_default;}
    //设置默认服务
    void setDefault(Servlet::ptr v) {m_default = v;} 

    //获取精准匹配服务对象
    Servlet::ptr getServlet(const std::string& uri);
    //获取模糊匹配服务对象
    Servlet::ptr getGlobServlet(const std::string& uri);
    //获取任意一个符合的服务对象
    Servlet::ptr getMatchedServlet(const std::string& uri);

private:
    //uri--->servlet  精准匹配  temp/xxxx
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    //uri--->servlet  模糊匹配  temp/*
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    //默认Servlet   所有路径都没匹配到时使用
    Servlet::ptr m_default;
    //读写锁  读多写少
    MutexType m_mutex;

};


class NotFoundServlet: public Servlet
{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet(const std::string& serverName, const std::string& name = "NotFoundServlet");

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

private:
    std::string m_serverName;
    std::string m_content;
};

}
}


#endif