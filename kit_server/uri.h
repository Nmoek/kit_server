#ifndef _KIT_URI_H_
#define _KIT_URI_H_


#include <memory>
#include <string>
#include <stdint.h>

#include "address.h"

namespace kit_server
{

/*
         foo://user@example.com:8042/over/there?name=ferret#nose
         \_/   \___________________/\__________/ \_________/ \__/
          |             |             |               |        |
       scheme       authority        path           query   fragment
          |      _____________________|__
         / \    /                        \
         urn :  example:animal:ferret:nose
*/

/**
 * @brief 统一资源定位符类
 */
class Uri
{
public:
    typedef std::shared_ptr<Uri> ptr;

    /**
     * @brief 统一资源定位符类构造函数
     * 
     */
    Uri();

    /**
     * @brief 设置scheme字段
     * @param[in] v 具体scheme值 http https ftp等
     */
    void setScheme(const std::string& v) {m_scheme = v;}

    /**
     * @brief 获取scheme字段
     * @return const std::string& 
     */
    const std::string& getScheme() const {return m_scheme;}

    /**
     * @brief 设置用户认证信息
     * @param[in] v  具体的用户认证信息值
     */
    void setUserinfo(const std::string& v) {m_userinfo = v;}

    /**
     * @brief 获取用户认证信息
     * @return const std::string& 
     */
    const std::string& getUserinfo() const {return m_userinfo;}

    /**
     * @brief 设置域名
     * @param[in] v 具体域名值  www.xxxx.com
     */
    void setHost(const std::string& v) {m_host = v;}

    /**
     * @brief 获取域名
     * @return const std::string& 
     */
    const std::string& getHost() const {return m_host;}
    
    /**
     * @brief 设置端口号
     * @param[in] v 具体端口号 
     */
    void setPort(int16_t v) {m_port = v;}

    /**
     * @brief 获取端口号 没有设置则根据scheme来设置http = 80 https = 443
     * @return int16_t 
     */
    int16_t getPort() const;

    /**
     * @brief 设置资源路径
     * @param[in] v 具体资源路径 
     */
    void setPath(const std::string& v) {m_path = v;}

    /**
     * @brief 获取资源路径 
     * @return const std::string& 
     */
    const std::string& getPath() const {return m_path;};

    /**
     * @brief 设置查询参数
     * @param[in] v 具体查询参数  "?query"
     */
    void setQuery(const std::string& v) {m_query = v;}

    /**
     * @brief 获取查询参数
     * @return const std::string& 
     */
    const std::string& getQuery() const {return m_query;}

    /**
     * @brief 设置片段标识符
     * @param[in] v 具体片段标识符的值
     */
    void setFragment(const std::string& v) {m_fragment = v;}

    /**
     * @brief 获取片段标识符
     * @return const std::string& 
     */
    const std::string& getFragment() const {return m_fragment;}

    /**
     * @brief 将URI信息以流的形式组装
     * @param[in] os 标准输出流 
     * @return std::ostream& 
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 将URI信息以字符串形式输出
     * @return std::string 
     */
    std::string toString() const;

    /**
     * @brief 通过URI中的域名创建通信地址对象
     * @return Address::ptr 
     */
    Address::ptr createAddress() const;

public:
    /**
     * @brief 通过字符串URI创建URI对象
     * @param[in] uri URI字符串 
     * @return Uri::ptr 
     */
    static Uri::ptr Create(const std::string& uri);

private:
    /**
     * @brief 是否被设置为默认端口号
     * @return true 是默认端口号
     * @return false 不是默认端口号
     */
    bool isDefaultPort() const;

private:
    //scheme字段
    std::string m_scheme;
    //用户认证信息字段
    std::string m_userinfo;
    //域名字段
    std::string m_host;
    //端口号字段
    int16_t m_port;
    //资源路径字段
    std::string m_path;
    //查询参数字段
    std::string m_query;
    //片段标识符字段
    std::string m_fragment;
};

}

#endif