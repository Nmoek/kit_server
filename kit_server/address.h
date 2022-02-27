#ifndef _KIT_ADRESS_H_
#define _KIT_ADRESS_H_

#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <iostream>

namespace kit_server
{

class IPAddress;

/**
 * @brief 通信地址基类
 */
class Address
{
public:
    typedef std::shared_ptr<Address> ptr;

    virtual ~Address() {};

    /**
     * @brief 获取当前套接字所使用的协议类型
     * @return 返回协议类型 
     */
    int getFamily() const;

    /**
     * @brief 获取通信地址的结构体
     * @return 返回通信地址的结构体 const struct sockaddr*
     */
    virtual const struct sockaddr* getAddr() const = 0;

    /**
     * @brief 获取通信地址的结构体
     * @return 返回通信地址的结构体 struct sockaddr*
     */
    virtual struct sockaddr* getAddr() = 0;

    /**
     * @brief 获取通信结构体长度
     * @return 返回通信结构体长度
     */
    virtual socklen_t getAddrLen() const = 0;

    
    /**
     * @brief 输出通信地址的全部信息
     * @param[in] os 标准库规定的标准输出流对象 
     * @return 返回标准输出流对象
     */
    virtual std::ostream& insert(std::ostream& os) const = 0;

    /**
     * @brief 将通信地址的信息转为字符串显示
     * @return 返回通信地址的字符串
     */
    std::string toString() const;

    /**
     * @brief 重载符号 <   
     * @param[in] addr 通信地址对象
     * @return true 当前对象比传入对象"小"
     * @return false 当前对象比传入对象"大"
     */
    bool operator<(const Address& addr) const;

    /**
     * @brief 重载符号 ==   
     * @param[in] addr 通信地址对象
     * @return true 当前对象与传入对象"相等"
     * @return false 当前对象与传入对象"不相等"
     */
    bool operator==(const Address& addr) const;

    /**
     * @brief 重载符号 !=   
     * @param[in] addr 通信地址对象
     * @return true 当前对象与传入对象"不相等"
     * @return false 当前对象与传入对象"相等"
     */
    bool operator!=(const Address& addr) const;


    /**
     * @brief 根据传入的协议类型创建对应的通信地址子类对象
     * @param[in] sockaddr 通信地址结构体指针
     * @param[in] len 通信地址结构体长度
     * @return 返回通信地址对象指针
     */
    static Address::ptr CreateFromText(const struct sockaddr* sockaddr, socklen_t len);


    /**
     * @brief 通过域名/地址字符串获取所有通信地址
     * @param[out] result 保存通信地址的容器
     * @param[in] host 输入的域名
     * @param[in] family 输入的地址族类型
     * @param[in] type 输入的套接字类型
     * @param[in] protocol 输入的地址协议类型
     * @return true result只要有地址就获取成功
     * @return false result为空可能获取失败
     */
    static bool LookUp(std::vector<Address::ptr>& result, const std::string& host, 
        int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 通过域名获取任意一个通信地址
     * @param[in] host 输入的域名
     * @param[in] family 输入的地址族类型
     * @param[in] type 输入的套接字类型
     * @param[in] protocol 输入的地址协议类型
     * @return 返回的通信地址对象智能指针
     */
    static Address::ptr LookUpAny(const std::string& host, int family = AF_INET, 
        int type = 0, int protocol = 0);
    

    /**
     * @brief 通过域名获取任意一个IP地址
     * @param[in] host 输入的域名
     * @param[in] family 输入的地址族类型
     * @param[in] type 输入的套接字类型
     * @param[in] protocol 输入的地址协议类型
     * @return 返回的IP地址对象智能指针
     */
    static std::shared_ptr<IPAddress> LookUpAnyIPAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 通过所有网卡获取所有通信地址
     * @param[out] result 保存 网卡----{通信地址对象指针, 掩码长度}
     * @param[in] family 协议类型
     * @return true 获取成功
     * @return false 获取失败
     */
    static bool GetInertfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result, int family = AF_UNSPEC);

    /**
     * @brief 通过指定网卡获取所有通信地址
     * @param[out] result 保存 {通信地址对象指针, 掩码长度}
     * @param[in] iface 指定的网卡名称
     * @param[in] family 协议类型
     * @return true 获取成功
     * @return false 获取失败
     */
    static bool GetInertfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result, const std::string &iface, int family = AF_UNSPEC);

};

std::ostream& operator<<(std::ostream& os, const Address& addr);



/**
 * @brief IP地址基类
 */
class IPAddress: public Address
{
public:
    typedef std::shared_ptr<IPAddress> ptr;

    /**
     * @brief IP地址的广播地址
     * @param[in] len 掩码长度
     * @return 返回IP地址对象指针 
     */
    virtual IPAddress::ptr broadcastAddress(uint32_t len) = 0;

    /**
     * @brief 子网起始地址
     * @param[in] len 掩码长度
     * @return 返回IP地址对象指针 
     */
    virtual IPAddress::ptr subnetAddress(uint32_t len) = 0;

    /**
     * @brief 子网掩码
     * @param[in] len 掩码长度
     * @return 返回IP地址对象指针 
     */
    virtual IPAddress::ptr subnetMask(uint32_t len) = 0;

    /**
     * @brief 获取端口号
     * @return 返回端口号 uint16_t
     */
    virtual uint16_t getPort() const = 0;

    /**
     * @brief 设置端口号
     * @param[in] val  传入要设置的端口号uint16_t 
     */
    virtual void setPort(uint16_t val) = 0; 


    /**
     * @brief 从文本型网络地址转换为一个实际IP地址
     * @param[in] address 地址字符串
     * @param[in] port 端口号
     * @return 返回IP地址对象指针
     */
    static IPAddress::ptr CreateFromText(const char* address, uint16_t port = 0);
};

/**
 * @brief IPv4地址类 
 */
class IPv4Address: public IPAddress
{
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    /**
     * @brief IPv4类构造函数
     * @param[in] sockaddr 通信结构体 
     */
    IPv4Address(const struct sockaddr_in sockaddr);

    /**
     * @brief IPv4类构造函数
     * @param[in] address IPv4地址
     * @param[in] port 端口号
     */
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);


    /**
     * @brief 获取通信地址的结构体
     * @return 返回通信地址的结构体 const struct sockaddr*
     */
    const struct sockaddr* getAddr() const override;

    /**
     * @brief 获取通信地址的结构体
     * @return 返回通信地址的结构体 struct sockaddr*
     */
    struct sockaddr* getAddr() override;

    /**
     * @brief 获取通信地址的结构体长度
     * @return 返回通信地址的结构体长度
     */
    socklen_t getAddrLen() const override;

    //输出
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t len) override;
    IPAddress::ptr subnetAddress(uint32_t len) override;
    IPAddress::ptr subnetMask(uint32_t len) override;

    uint16_t getPort() const override;
    void setPort(uint16_t val) override;


    /**
     * @brief 从文本型网络地址转换为一个实际IPv4地址
     * @param[in] address 地址字符串
     * @param[in] port 端口号
     * @return 返回IPv4地址对象指针
     */
    static IPv4Address::ptr CreateFromText(const char *addr, uint16_t port = 0);

private:
    struct sockaddr_in m_sockaddr;

};


/**
 * @brief IPv6地址类
 */
class IPv6Address: public IPAddress
{
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    /**
     * @brief 无参IPv6类构造函数
     */
    IPv6Address();
    
    /**
     * @brief IPv6类构造函数
     * @param[in] sockaddr 通信结构体 
     */
    IPv6Address(const struct sockaddr_in6 sockaddr);

    /**
     * @brief IPv6类构造函数
     * @param[in] address IPv6地址数值组
     * @param port 端口号
     */
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    //获取通信结构体
    const struct sockaddr* getAddr() const override;
    struct sockaddr* getAddr() override;
    //获取通信结构体长度
    socklen_t getAddrLen() const override;
    //输出
    std::ostream& insert(std::ostream& os) const override;


    IPAddress::ptr broadcastAddress(uint32_t len) override;
    IPAddress::ptr subnetAddress(uint32_t len) override;
    IPAddress::ptr subnetMask(uint32_t len) override;

    uint16_t getPort() const override;
    void setPort(uint16_t val) override; 

    /**
     * @brief 从文本型网络地址转换为一个实际IPv6地址
     * @param[in] address 地址字符串
     * @param[in] port 端口号
     * @return 返回IPv6地址对象指针
     */
    static IPv6Address::ptr CreateFromText(const char* addr, uint16_t port = 0);


private:
    struct sockaddr_in6 m_sockaddr;

};

/**
 * @brief Unix域地址类
 */
class UnixAddress: public Address
{
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    /**
     * @brief 无参Unix域地址类构造函数
     */
    UnixAddress();

    /**
     * @brief Unix域地址类构造函数
     * @param[in] path 通信文件路径
     */
    UnixAddress(const std::string& path);

    /**
     * @brief Unix域地址类构造函数
     * @param[in] addr 通信结构体
     */
    UnixAddress(const struct sockaddr_un addr);

    /**
     * @brief Unix域地址类析构函数
     */
    ~UnixAddress();

    //获取通信结构体
    const struct sockaddr* getAddr() const override;
    struct sockaddr* getAddr() override;

    //设置通信结构体长度
    void setAddrLen(uint32_t len);
    //获取通信结构体长度
    socklen_t getAddrLen() const override;
    //输出
    std::ostream& insert(std::ostream& os) const override;

private:
    struct sockaddr_un m_sockaddr;
    std::string m_path;
    socklen_t m_sockaddrlen;

};

/**
 * @brief 未知地址类
 */
class UnkonwAddress: public Address
{
public:
    typedef std::shared_ptr<UnkonwAddress> ptr;

    /**
     * @brief 未知地址类构造函数
     * @param[in] family 协议类型
     */
    UnkonwAddress(int family);

    /**
     * @brief 未知地址类构造函数
     * @param[in] sockaddr 通信地址类 
     */
    UnkonwAddress(const struct sockaddr& sockaddr);

    //获取通信结构体
    const struct sockaddr* getAddr() const override;
    struct sockaddr* getAddr() override;
    //获取通信结构体长度
    socklen_t getAddrLen() const override;
    //输出
    std::ostream& insert(std::ostream& os) const override;

private:
    struct sockaddr m_sockaddr;
};

}





#endif 