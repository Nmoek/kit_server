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

/**
 * @brief 服务类基类
 */
class Servlet
{
public:
    typedef std::shared_ptr<Servlet> ptr;  

    /**
     * @brief 服务类构造函数
     * @param[in] name 服务名称
     */
    Servlet(const std::string& name):m_name(name) { }

    /**
     * @brief 服务类析构函数
     */
    virtual ~Servlet() {}

    /**
     * @brief 执行服务（虚接口）
     * @param[in] request HTTP请求报文
     * @param[in] response HTTP响应报文
     * @param[in] session HTTP主动连接会话
     * @return int32_t 
     */
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) = 0;

    /**
     * @brief 获取服务名称
     * @return const std::string& 
     */
    const std::string& getName() const {return m_name;}

protected:
    /// 服务名称
    std::string m_name;
};


/**
 * @brief 函数服务类
 */
class FunctionServlet: public Servlet
{
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (HttpRequest::ptr request, 
        HttpResponse::ptr response, HttpSession::ptr session)> callback;

    /**
     * @brief 函数服务类
     * @param[in] cb 服务回调函数
     * @param[in] name 服务名称
     */
    FunctionServlet(callback cb, const std::string& name = "FunctionServlet");

    /**
     * @brief 执行服务
     * @param[in] request HTTP请求报文
     * @param[in] response HTTP响应报文
     * @param[in] session HTTP主动连接会话
     * @return int32_t 
     */
    int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

private:
    /// 回调函数
    callback m_cb;
};

/**
 * @brief 服务分发类
 */
class ServletDispatch: public Servlet
{
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex MutexType;

    /**
     * @brief 服务分发类构造函数
     * @param[in] name 服务名称 
     */
    ServletDispatch(const std::string& name = "ServletDispatch");

    /**
     * @brief 执行服务
     * @param[in] request HTTP请求报文
     * @param[in] response HTTP响应报文
     * @param[in] session HTTP主动连接会话
     * @return int32_t 
     */
    int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

    /**
     * @brief 添加服务，精准匹配
     * @param[in] uri 资源路径
     * @param[in] slt 对应的服务类对象
     */
    void addServlet(const std::string& uri, Servlet::ptr slt);

    /**
     * @brief 添加服务，精准匹配
     * @param[in] uri 资源路径
     * @param[in] cb 对应的回调函数 
     */
    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    /**
     * @brief 添加服务，模糊匹配
     * @param[in] uri 资源路径
     * @param[in] slt 对应的服务类对象 
     */
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);

    /**
     * @brief 添加服务，模糊匹配
     * @param[in] uri 资源路径
     * @param[in] cb 对应的回调函数 
     */
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    /**
     * @brief 删除服务，精准匹配
     * @param[in] uri 资源路径
     */
    void delServlet(const std::string& uri);

    /**
     * @brief 删除服务，模糊匹配
     * @param[in] uri 资源路径
     */
    void delGlobServlet(const std::string& uri);
    
    /**
     * @brief 获取默认服务
     * @return Servlet::ptr 
     */
    Servlet::ptr getDefault() const {return m_default;}

    /**
     * @brief 设置默认服务
     * @param[in] v 指定的服务类对象智能指针
     */
    void setDefault(Servlet::ptr v) {m_default = v;} 

    /**
     * @brief 获取精准匹配服务对象
     * @param[in] uri 资源路径 
     * @return Servlet::ptr 
     */
    Servlet::ptr getServlet(const std::string& uri);

    /**
     * @brief 获取模糊匹配服务对象
     * @param[in] uri 资源路径 
     * @return Servlet::ptr 
     */
    Servlet::ptr getGlobServlet(const std::string& uri);

    /**
     * @brief 根据uri获取任意一个匹配的服务对象
     * @param[in] uri 资源路径 
     * @return Servlet::ptr 
     */
    Servlet::ptr getMatchedServlet(const std::string& uri);

private:
    /// uri--->servlet  精准匹配  temp/xxxx
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    /// uri--->servlet  模糊匹配  temp/*
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    /// 默认Servlet   所有路径都没匹配到时使用
    Servlet::ptr m_default;
    /// 读写锁  读多写少
    MutexType m_mutex;

};

/**
 * @brief 404服务类
 */
class NotFoundServlet: public Servlet
{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    /**
     * @brief 404服务类构造函数
     * @param[in] serverName 服务器名称 
     * @param[in] name 服务名称 
     */
    NotFoundServlet(const std::string& serverName, const std::string& name = "NotFoundServlet");

    /**
     * @brief 执行服务
     * @param[in] request HTTP请求报文
     * @param[in] response HTTP响应报文
     * @param[in] session HTTP主动连接会话
     * @return int32_t 
     */
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

private:
    /// 服务器名称
    std::string m_serverName;
    /// 报文实体内容
    std::string m_content;
};

/**
 * @brief Hello服务类
 */
class HelloServlet: public Servlet
{
public:
    typedef std::shared_ptr<HelloServlet> ptr;

    /**
     * @brief 404服务类构造函数
     * @param[in] serverName 服务器名称 
     * @param[in] name 服务名称 
     */
    HelloServlet(const std::string& serverName, const std::string& name = "HelloServlet");

    /**
     * @brief 执行服务
     * @param[in] request HTTP请求报文
     * @param[in] response HTTP响应报文
     * @param[in] session HTTP主动连接会话
     * @return int32_t 
     */
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

private:
    /// 服务器名称
    std::string m_serverName;
    /// 报文实体内容
    std::string m_content;
};

}
}


#endif