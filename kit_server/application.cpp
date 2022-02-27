#include "application.h"
#include "address.h"
#include "Log.h"
#include "env.h"
#include "config.h"
#include "util.h"
#include "daemon.h"
#include "http/http_server.h"

#include <vector>
#include <string>
#include <unistd.h>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


/**
 * @brief 配置项:服务器启动目录
 */
static ConfigVar<std::string>::ptr g_server_work_path = 
    Config::LookUp("server.work_path", std::string("/home/nmoek/apps/work/kit_server"), "server work path");

/**
 * @brief 配置项:服务器程序pid文件的名称
 */
static ConfigVar<std::string>::ptr g_server_pid_file = 
    Config::LookUp("server.pid_file", std::string("kit_server.pid"), "server pid file");


/**
 * @brief 服务器配置项
 */
struct HttpServerConf
{
    std::vector<std::string> address;
    int keepalive = 0;
    int recv_timeout = 2 * 60 * 1000;
    std::string name = "http_server";

    //没有地址那么这个配置项是非法的
    bool isValid() const 
    {
        return address.size();
    }

    bool operator==(const HttpServerConf& conf) const 
    {
        return conf.address == address && 
            conf.keepalive == keepalive &&
            conf.recv_timeout == recv_timeout && 
            conf.name == name;
    }

};

/**
 * @brief 自定义结构 模板偏特化 string---->HttpServerConf
 * @tparam  
 */
template<>
class LexicalCast<std::string, HttpServerConf>
{
public:
    HttpServerConf operator()(const std::string& val)
    {
        HttpServerConf conf;
        
        //利用到YAML库的Load()  string----->YAML::Node
        YAML::Node node = YAML::Load(val);
        conf.name = node["name"].as<std::string>(conf.name);
        conf.keepalive = node["keppalive"].as<int>(conf.keepalive);
        conf.recv_timeout = node["recv_timeout"].as<int>(conf.recv_timeout);

        if(node["address"].IsDefined())
        {
            for(size_t i = 0;i < node["address"].size();++i)
            {
                std::stringstream ss;
                ss << node["address"][i];

                conf.address.push_back(ss.str());
            }
        }

        return conf;
    }

};


/**
 * @brief 自定义结构 模板偏特化 HttpServerConf---->string
 * @tparam  
 */
template<>
class LexicalCast<HttpServerConf, std::string>
{
public:
    std::string operator()(const HttpServerConf& conf)
    {
        YAML::Node node;
      
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["recv_timeout"] = conf.recv_timeout;

        for(auto &x : conf.address)
        {
            node["address"].push_back(x);
        }

        std::stringstream ss;
        ss << node;
    
        return ss.str();
    }

};


/**
 * @brief 配置项:服务器加载进来的配置信息
 */
static ConfigVar<std::vector<HttpServerConf> >::ptr g_http_servers_conf = 
    Config::LookUp("http_servers", std::vector<HttpServerConf>(), "http servers config");

/**
 * @brief s_instance静态变量初始化
 */
Application* Application::s_instance = nullptr;

Application::Application()
{
    s_instance = this;
}

bool Application::init(int argc, char* argv[])
{

    m_argc = argc;
    m_argv = argv;

    //添加启动程序的相关指令说明
    Env* env = EnvMgr::GetInstance();
    env->addHelp("s", "start with the terminal");
    env->addHelp("d", "run as daemon");
    env->addHelp("c", "config file peth, default: \"./conf\"");
    env->addHelp("h", "see help");

    bool is_print_help = false;
    if(!env->init(argc, argv))
        is_print_help = true;

    if(env->has("h"))
        is_print_help = true;
    
    //获取配置文件的文件夹路径 并且加载
    std::string conf_path = env->getConfigPath();
    KIT_LOG_INFO(g_logger) << "load config path:" << conf_path;

    Config::LoadFromConfigDir(conf_path);

    if(is_print_help)
    {
        env->printHelp();
        return false;
    }

    //判断以什么方式启动程序
    int run_type = 0;
    if(env->has("s"))
        run_type = 1;
    else if(env->has("d"))
        run_type = 2;
    
    if(run_type == 0)
    {
        env->printHelp();
        return false;
    }

    std::string pid_file = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();

    if(FSUtil::IsRunningPidFile(pid_file))
    {
        KIT_LOG_ERROR(g_logger) << "server is running! pid_file=" << pid_file;
        return false;
    }

    //如果提供的工作路径不存在要创建一个
    if(!FSUtil::Mkdir(g_server_work_path->getValue()))
    {
        KIT_LOG_ERROR(g_logger) << "creat work path error:[" << g_server_work_path->getValue() << ", errno=" << errno << ",is:" << strerror(errno);

        return false; 
    }

    return true;
}

bool Application::run()
{
    //看是否是以守护进程的方式启动
    bool is_daemon = EnvMgr::GetInstance()->has("d");
    return start_daemon(m_argc, m_argv, std::bind(&Application::main, this, std::placeholders::_1, std::placeholders::_2), is_daemon);

}

bool Application::getServer(const std::string& type, std::vector<TcpServer::ptr>& servers)
{
    auto it = m_servers.find(type);
    if(it == m_servers.end())
        return false;
    
    servers = it->second;
    return true;

}


void Application::listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers)
{
    servers = m_servers;
}

int Application::main(int argc, char* argv[])
{
    std::string pid_file = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();

    //打开pid文件 写入进程PID号
    std::ofstream ofs(pid_file);
    if(!ofs)
    {
        KIT_LOG_ERROR(g_logger) << "open pid file error, file=" << pid_file;
        return -1; 
    }

    //需要实际执行服务器代码子进程的PID
    ofs << getpid();

    m_mainIOManager.reset(new IOManager("app", 1));
    m_mainIOManager->schedule(std::bind(&Application::run_coroutine, this));
    m_mainIOManager->stop();

    return 0;
}

int Application::run_coroutine()
{
    auto server_conf = g_http_servers_conf->getValue();
    for(size_t i = 0;i < server_conf.size();++i)
    {
        KIT_LOG_INFO(g_logger) << "server_conf[" << i << "]:\n" << LexicalCast<HttpServerConf, std::string>()(server_conf[i]);

        std::vector<Address::ptr> address;
        for(auto &x : server_conf[i].address)
        {
            size_t pos = x.find(":");
            if(pos == std::string::npos)
            {
                KIT_LOG_ERROR(g_logger) << "invalid IPv4 address=" << x;
                continue;
            }

            auto addr = Address::LookUpAny(x);
            if(addr)
            {
                address.push_back(addr);
                continue;
            }

            std::vector<std::pair<Address::ptr, uint32_t> > result;
            if(!Address::GetInertfaceAddresses(result, x.substr(0, pos)))
            {
                KIT_LOG_ERROR(g_logger) << "invlid address=" << x;
                continue;
            }

            for(auto &i : result)
            {
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(i.first);
                if(ipaddr)
                    ipaddr->setPort(atoi(x.substr(pos + 1).c_str()));
                
                address.push_back(ipaddr);

            }
        }

        http::HttpServer::ptr http_server(new http::HttpServer(server_conf[i].keepalive));

        std::vector<Address::ptr> fails;
        if(!http_server->bind(address, fails))
        {
            for(auto& x : fails)
            {
                KIT_LOG_ERROR(g_logger) << "bind address fail:" << *x;
            }

            _exit(0);
        }
        http_server->start();
        if(server_conf[i].name.size())
            http_server->setName(server_conf[i].name);
            
        m_servers[server_conf[i].name].push_back(http_server);

    }

    return 0;
}

}