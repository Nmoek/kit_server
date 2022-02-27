#include "address.h"
#include "Log.h"
#include "endian.h"


#include <sstream>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


/**
 * @brief 创建一个只有主机号的掩码
 * @param[in] bits  掩码位数 
 * @return T 返回传入类型的数据
 */
template<class T>
static T CreateHostMask(uint32_t bits)
{
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

/**
 * @brief 计算一个数里面 二进制表示中有多少个1
 * @param[in] val 传入的数据 
 * @return 返回传入数据中二进制共有"1"的个数 uint32_t 
 */
template<class T>
static uint32_t CountBytes(T val)
{
    uint32_t count = 0;
    for(;val;++count)
        val &= val - 1;
    
    return count;
}

/*******************************************Address****************************************/
//获取协议类型
int Address::getFamily() const
{
    return getAddr()->sa_family;
}

//以字符串形式输出
std::string Address::toString() const
{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

//流式输出
std::ostream& operator<<(std::ostream& os, const Address& addr)
{
    return addr.insert(os);
}

//用于容器的比较
bool Address::operator<(const Address& addr) const
{
    //拿到最小的长度
    socklen_t min_len = std::min(getAddrLen(), addr.getAddrLen());

    //以最小字节长度 比较地址大小
    int ret = memcmp(getAddr(), addr.getAddr(), min_len);
    if(ret < 0)
        return true;
    else if (ret > 0)
        return false;
    else if(getAddrLen() < addr.getAddrLen())   //地址字节大小相同的 比较长度
        return true;
    
    return false;
    
}

bool Address::operator==(const Address& addr) const
{
    return getAddrLen() == addr.getAddrLen() && 
        memcmp(getAddr(), addr.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& addr) const
{
    return !(*this == addr);
}


Address::ptr Address::CreateFromText(const struct sockaddr* sockaddr, socklen_t len)
{
    if(sockaddr == nullptr)
        return nullptr;
    
    Address::ptr result;
    switch(sockaddr->sa_family)
    {
        //IPv4类型地址
        case AF_INET:
        {
            result.reset(new IPv4Address(*(const struct sockaddr_in*)sockaddr));
        }break;

        //IPv6类型地址
        case AF_INET6:
        {
            result.reset(new IPv6Address(*(const struct sockaddr_in6*)sockaddr));
        }break;

        //Unix域类型地址
        case AF_UNIX:
        {
            result.reset(new UnixAddress(*(const struct sockaddr_un*)sockaddr));
        }   break;

        //其他为未知类型地址
        default:
        {
            result.reset(new UnkonwAddress(*sockaddr));
        }break;

    }

    return result;
}

bool Address::LookUp(std::vector<Address::ptr>& result, const std::string& host, 
    int family, int type, int protocol)
{
    struct addrinfo hints, *results, *next;
    hints.ai_family = family;
    hints.ai_flags = 0;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char* service = nullptr;

    /*1. 检查是否IPv6形式的请求服务 [xxx]:service*/
    if(host.size() && host[0] == '[')
    {
        //先找到IPv6地址中"]"的位置
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) //如果不为NULL 说明使用一个IPv6的URL
        {
            //如果"]"后紧跟着一个":"则认为有service字段
            if(*(endipv6 + 1) == ':')
            {
                //存储service字段的首地址
                service = endipv6 + 2;
            }

            //将 [xxx]:service 中xxx的部分取出放入string node中
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    /*2.如果node为空 说明是IPv4形式请求服务  xxx:service  */
    if(!node.size())
    {
        service = (const char*)memchr(host.c_str(), ':', host.size());

        //不为空存在service字段
        if(service)
        {
            // xxx:service 找到 ':' 后是否还存在另外一个 ':'
            if(!memchr(service + 1, ':',  host.c_str() + host.size() - service - 1))
            {
                node = host.substr(0, service - host.c_str());
                //此时字符串指针指向":" +1便是service字段起始处
                ++service;
            }
        }

        
    }

    /*3. 如果node在上面的两次检查中都未被赋值 说明不存在service字段 直接赋值*/
    if(!node.size())
        node = host;


    /*4.调用API获取域名上的网络通信地址*/
    int ret = getaddrinfo(node.c_str(), service, &hints, &results);
    if(ret != 0)
    {
        KIT_LOG_ERROR(g_logger) << "Address::LookUp(" << host << ", "
            << family << ", " << type << ") error=" << ret << ", is:" << gai_strerror(ret);

        return false;   
    } 

    /*5.获取到的所有网络通信地址是一个链表的形式 依次访问构建对应的地址类对象*/
    next = results;
    while(next)
    {
        result.emplace_back(CreateFromText(next->ai_addr, (socklen_t)next->ai_addrlen));

        next = next->ai_next;
    }

    freeaddrinfo(results);
    return result.size();

}


//
Address::ptr Address::LookUpAny(const std::string& host, int family, int type, int protocol)
{
    std::vector<Address::ptr> result;

    if(LookUp(result, host, type, protocol))
    {
        return result[0];
    }

    return nullptr;

}

//通过域名获取IP地址
std::shared_ptr<IPAddress> Address::LookUpAnyIPAddress(const std::string& host, int family, int type, int protocol)
{
    std::vector<Address::ptr> result;

    if(LookUp(result, host, type, protocol))
    {
        for(auto &x : result)
        {
            IPAddress::ptr val = std::dynamic_pointer_cast<IPAddress>(x);
            if(val)
                return val;
        }
    }

    return nullptr;
}


//通过网卡获取地址
bool Address::GetInertfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result, int family)
{
    struct ifaddrs *next, *results;
    int ret = getifaddrs(&results);
    if(ret != 0)
    {   
        KIT_LOG_ERROR(g_logger) << "Address::GetInertfaceAddresses getifaddrs error=" << errno << ", is:" << strerror(errno);

        freeifaddrs(results);
        return false;
    }
    
    try
    {

        for(next = results;next;next = next->ifa_next)
        {
            Address::ptr addr;
            uint32_t len = ~0u;

            //过滤掉不为指定地址族的结点
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family)
                continue;


            switch(next->ifa_addr->sa_family)
            {
                case AF_INET:
                {
                    addr = CreateFromText(next->ifa_addr, sizeof(struct sockaddr_in));
                    //小技巧 掩码转换 sockaddr---->sockaddr_in
                    uint32_t netmask = ((struct sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                    //计算掩码长度
                    len = CountBytes(netmask);

                }break;

                case AF_INET6:
                {
        
                    addr = CreateFromText(next->ifa_addr, sizeof(struct sockaddr_in6));

                    //小技巧 掩码转换 sockaddr---->sockaddr_in6
                    struct in6_addr& netmask = ((struct sockaddr_in6*)next->ifa_netmask)->sin6_addr;

                    //计算前缀长度
                    len = 0;
                    //每一个元素为uint8_t 共有16个
                    for(int i = 0;i < 16;++i)
                    {
                        len += CountBytes(netmask.s6_addr[i]);               
                    }

                }break;

                default:
                    break;
            }

            if(addr)
            {
                result.insert({next->ifa_name, {addr, len}});
            }
        
        }
    }
    catch(...)
    {
        KIT_LOG_ERROR(g_logger) << "Address::GetInertfaceAddresses error!";
        freeifaddrs(results);
        return false;
    }

    freeifaddrs(results);
    return true;

}

//指定网卡获取地址
bool Address::GetInertfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result, const std::string &iface, int family )
{
    //如果网卡没有指定 或者 为任意网卡
    if(!iface.size() || iface == "*")
    {
        if(family == AF_INET || family == AF_UNSPEC)
        {
            result.push_back({Address::ptr(new IPv4Address), 0u});
        }

        if(family == AF_INET6 || family == AF_UNSPEC)
        {
            result.push_back({Address::ptr(new IPv6Address), 0u});
        }

        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;
    if(!GetInertfaceAddresses(results, family))
    {
        return false;
    }

    //返回一个迭代器区间 first为首迭代器 second为尾迭代器
    //迭代器包装的内含容器类型仍为multimap<std::string, std::pair<Address::ptr, uint32_t> >
    auto it = results.equal_range(iface);
    for(;it.first != it.second;++it.first)
    {
        result.push_back(it.first->second);
    }

    return true;

}


/*******************************************IPAddress****************************************/

IPAddress::ptr IPAddress::CreateFromText(const char* address, uint16_t port)
{
    struct addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));

    //hints.ai_flags = AI_NUMERICHOST; //只允许使用点分十进制类型
    hints.ai_family = AF_UNSPEC;

    int ret = getaddrinfo(address, NULL, &hints, &results);
    if(ret != 0)
    {
        KIT_LOG_ERROR(g_logger) << "IPAddress::CreateFromText(" << address << ", " << port << "), "
            << "error=" << ret
            << ", is " << gai_strerror(ret);
        return nullptr;
    }

    try
    {
        //复用Address类中的方法
        IPAddress::ptr ret = std::dynamic_pointer_cast<IPAddress>(
            Address::CreateFromText(results->ai_addr, (socklen_t)results->ai_addrlen));
        
        if(ret)
        {
            ret->setPort(port);
        }

        freeaddrinfo(results);
        return ret;

    }
    catch(...)
    {
        freeaddrinfo(results);
        return nullptr;
    }
    
}


/*******************************************IPv4Address****************************************/
IPv4Address::IPv4Address(uint32_t address, uint16_t port)
{
    memset(&m_sockaddr, 0 ,sizeof(m_sockaddr));
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_port = byteswapOnSmallEndian(port);              //转换为大端字节序
    m_sockaddr.sin_addr.s_addr = byteswapOnSmallEndian(address);    //转换为大端字节序

}

IPv4Address::IPv4Address(const struct sockaddr_in sockaddr)
    :m_sockaddr(sockaddr)
{

}

//获取通信结构体
const struct sockaddr* IPv4Address::getAddr() const
{
    return (struct sockaddr*)&m_sockaddr;
}

struct sockaddr* IPv4Address::getAddr()
{
    return (struct sockaddr*)&m_sockaddr;
}

//获取通信结构体长度
socklen_t IPv4Address::getAddrLen() const
{
    return sizeof(m_sockaddr);
}

//输出
std::ostream& IPv4Address::insert(std::ostream& os) const 
{
    uint32_t addr = byteswapOnSmallEndian(m_sockaddr.sin_addr.s_addr); 
    //手动转 点分十进制
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       <<  (addr & 0xff);
 
    // char p[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &m_sockaddr.sin_addr, p, sizeof(m_sockaddr));
    // os << p;
    
    os << ":" << byteswapOnSmallEndian(m_sockaddr.sin_port);

    return os;     
}


IPAddress::ptr IPv4Address::broadcastAddress(uint32_t len)
{
    if(len > 32)
        return nullptr;
    
    struct sockaddr_in baddr(m_sockaddr);
    //或运算 一个主机号掩码
    baddr.sin_addr.s_addr |= byteswapOnSmallEndian(CreateHostMask<uint32_t>(len));

    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetAddress(uint32_t len)
{
    if(len > 32)
        return nullptr;
    
    struct sockaddr_in baddr(m_sockaddr);
    //与运算 一个子网掩码
    baddr.sin_addr.s_addr &= ~byteswapOnSmallEndian(CreateHostMask<uint32_t>(len));

    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t len)
{
    if(len > 32)
        return nullptr;

    struct sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;

    subnet.sin_addr.s_addr = ~byteswapOnSmallEndian(CreateHostMask<uint32_t>(len));

    return IPv4Address::ptr(new IPv4Address(subnet));

}

uint16_t IPv4Address::getPort() const
{
    return byteswapOnSmallEndian(m_sockaddr.sin_port);
}

void IPv4Address::setPort(uint16_t val)
{
    m_sockaddr.sin_port = byteswapOnSmallEndian(val);
}

//从文本型地址 转换为 IPv4地址
IPv4Address::ptr IPv4Address::CreateFromText(const char *addr, uint16_t port)
{
    IPv4Address::ptr ret(new IPv4Address);

    ret->m_sockaddr.sin_port = byteswapOnSmallEndian(port);

    int n = inet_pton(AF_INET, addr, &ret->m_sockaddr.sin_addr);
    if(n < 0)
    {
        KIT_LOG_ERROR(g_logger) << "IPv4Address::CreateFromText(" << addr << ", " << port << ")"
            << "n = " << n << ", errno=" << errno << ",is:" << strerror(errno);

        return nullptr;  
    }
    else if(n == 0)
    {
        KIT_LOG_ERROR(g_logger) << "IPv4Address::CreateFromText addr string is invalid!"; 
        return nullptr;
    }

    return ret;

}


/*******************************************IPv6Address****************************************/
IPv6Address::IPv6Address()
{
    memset(&m_sockaddr, 0, sizeof(m_sockaddr));
    m_sockaddr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
{
    memset(&m_sockaddr, 0, sizeof(m_sockaddr));
    m_sockaddr.sin6_family = AF_INET6;
    m_sockaddr.sin6_port = byteswapOnSmallEndian(port);
    memcpy(&m_sockaddr.sin6_addr.s6_addr, address, 16);
}

IPv6Address::IPv6Address(const struct sockaddr_in6 sockaddr)
    :m_sockaddr(sockaddr)
{

}

//获取通信结构体
const struct sockaddr* IPv6Address::getAddr() const
{
    return (struct sockaddr*)&m_sockaddr;
}

struct sockaddr* IPv6Address::getAddr()
{
    return (struct sockaddr*)&m_sockaddr;
}

//获取通信结构体长度
socklen_t IPv6Address::getAddrLen() const
{
    return sizeof(m_sockaddr);
}

//输出
std::ostream& IPv6Address::insert(std::ostream& os) const 
{
    // os << "[";
    // char p[INET6_ADDRSTRLEN];
    // inet_ntop(AF_INET6, &m_sockaddr.sin6_addr, p, sizeof(m_sockaddr));
    // os << p;

    os << "[";
    uint16_t* addr = (uint16_t*)&m_sockaddr.sin6_addr.s6_addr;
    bool used_zero8 = false;
    for(size_t i = 0;i < 8;++i)
    {
        if(addr[i] == 0 && !used_zero8)
            continue;

        //连续0用两个"::"代替 且只能代替一次
        if(i && addr[i - 1] == 0 && !used_zero8)
        {
            os << ":";
            used_zero8 = true;
        }

        if(i)
            os << ":";

        //地址数字 输出十六进制
        os << std::hex << (int)byteswapOnSmallEndian(addr[i]);
    }

    if(!used_zero8 && !addr[7])
        os << "::";
    
    
    os << "]:" << std::dec << byteswapOnSmallEndian(m_sockaddr.sin6_port);

    return os;

    
}


IPAddress::ptr IPv6Address::broadcastAddress(uint32_t len)
{
    if(len > 128)
        return nullptr;

    struct  sockaddr_in6 baddr(m_sockaddr);
    //处理网络标识和主机标识交界地方的比特位
    baddr.sin6_addr.s6_addr[len / 8] |= CreateHostMask<uint8_t>(len % 8);

    for(size_t i = len / 8 + 1;i < 16;++i)
    {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }

    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetAddress(uint32_t len)
{
    if(len > 128)
        return nullptr;

    struct  sockaddr_in6 sock_addr(m_sockaddr);
    sock_addr.sin6_addr.s6_addr[len / 8] &= ~CreateHostMask<uint8_t>(len % 8);

    for(size_t i = len / 8 + 1;i < 16;++i)
    {
        sock_addr.sin6_addr.s6_addr[i] = 0;
    }

    return IPv6Address::ptr(new IPv6Address(sock_addr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t len)
{
    if(len > 128)
        return nullptr;

    struct sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[len / 8] = ~CreateHostMask<uint8_t>(len % 8);
    
    for(size_t i = 0;i < len / 8;++i)
    {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }

    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint16_t IPv6Address::getPort() const
{
    return byteswapOnSmallEndian(m_sockaddr.sin6_port);
}

void IPv6Address::setPort(uint16_t port)
{
    m_sockaddr.sin6_port = byteswapOnSmallEndian(port);
}

IPv6Address::ptr IPv6Address::CreateFromText(const char* addr, uint16_t port)
{
    IPv6Address::ptr ret(new IPv6Address);

    ret->m_sockaddr.sin6_port = byteswapOnSmallEndian(port);

    int n = inet_pton(AF_INET, addr, &ret->m_sockaddr.sin6_addr);
    if(n < 0)
    {
        KIT_LOG_ERROR(g_logger) << "IPv6Address::CreateFromText(" << addr << ", " << port << ")"
            << "n = " << n << ", errno=" << errno << ",is:" << strerror(errno);

        return nullptr;  
    }
    else if(n == 0)
    {
        KIT_LOG_ERROR(g_logger) << "IPv6Address::CreateFromText addr string is invalid!"; 
        return nullptr;
    }

    return ret;
}


/*******************************************UnixAddress****************************************/
static const size_t MAX_PATH_LEN = sizeof(((struct sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress()
{
    memset(&m_sockaddr, 0, sizeof(m_sockaddr));
    m_sockaddr.sun_family = AF_UNIX;
    m_sockaddrlen = offsetof(struct sockaddr_un, sun_path) + MAX_PATH_LEN;
   
}

UnixAddress::UnixAddress(const std::string& path)
    :m_path(path)
{
    memset(&m_sockaddr, 0, sizeof(m_sockaddr));
    m_sockaddr.sun_family = AF_UNIX;

    m_sockaddrlen = path.size() + 1;

    if(path.size() && path[0] == '\0')
        --m_sockaddrlen;
    
    if(m_sockaddrlen > sizeof(m_sockaddr.sun_path))
        throw std::logic_error("Unix socket path length too long!!");
    
    memcpy(m_sockaddr.sun_path, path.c_str(), m_sockaddrlen);
    m_sockaddrlen += offsetof(struct sockaddr_un, sun_path);
}

UnixAddress::UnixAddress(const struct sockaddr_un addr)
{
    m_sockaddr = addr;
}

UnixAddress::~UnixAddress()
{
    if(m_path.size())
        unlink(m_path.c_str());
}

//获取通信结构体
const struct sockaddr* UnixAddress::getAddr() const
{
    return (struct sockaddr*)&m_sockaddr;
}

struct sockaddr* UnixAddress::getAddr()
{
    return (struct sockaddr*)&m_sockaddr;
}

void UnixAddress::setAddrLen(uint32_t len)
{
    m_sockaddrlen = len;
}

//获取通信结构体长度
socklen_t UnixAddress::getAddrLen() const
{
    return m_sockaddrlen;
}

//输出地址
std::ostream& UnixAddress::insert(std::ostream& os) const
{
    if(m_sockaddrlen > offsetof(struct sockaddr_un, sun_path) && 
        m_sockaddr.sun_path[0] == '\0')
    {
        return os << "\\0" << std::string(m_sockaddr.sun_path + 1, m_sockaddrlen - offsetof(struct sockaddr_un, sun_path) - 1); 
    } 
    os << m_sockaddr.sun_path;

    return os;
}   


/*******************************************UnkonwAddress****************************************/
UnkonwAddress::UnkonwAddress(int family)
{
    memset(&m_sockaddr, 0 , sizeof(m_sockaddr));
    m_sockaddr.sa_family = family;
}

UnkonwAddress::UnkonwAddress(const struct sockaddr& sockaddr)
{
    m_sockaddr = sockaddr;
}


//获取通信结构体
const struct sockaddr* UnkonwAddress::getAddr() const
{
    return &m_sockaddr;
}

struct sockaddr* UnkonwAddress::getAddr()
{
    return &m_sockaddr;
}

//获取通信结构体长度
socklen_t UnkonwAddress::getAddrLen() const
{
    return sizeof(m_sockaddr);
}
//输出
std::ostream& UnkonwAddress::insert(std::ostream& os) const
{
    os << "[UnkonwAdress family=" << m_sockaddr.sa_family << "]";

    return os;
}


}