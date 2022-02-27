#include "env.h"
#include "Log.h"

#include <string.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <errno.h>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


bool Env::init(int argc, char* argv[])
{
    char link[1024] = {0};
    char path[1024] = {0};
    //从环境变量中拼装出所需的可执行文件的符号链接路径
    sprintf(link, "/proc/%d/exe", getpid());
    //通过符号链接路径读取出真正的路径
    if(readlink(link, path, sizeof(path)) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "readlink error, errno=" << errno
            << ",is:" << strerror(errno);
        return false;
    }
    //拿到执行文件的绝对路径 /path/xxx/exe
    m_exe = path;

    //把执行文件的所在文件夹路径截取出来
    auto pos = m_exe.find_last_of("/");
    m_cwd = m_exe.substr(0, pos) + "/";


    //执行命令
    m_programName = argv[0];

    //  -config /path/to/config -file xxxxx
    const char* now_key = nullptr;

    //0号位置放的是执行程序名称
    for(int i = 1;i < argc;++i)
    {
        //'-'开头表示是key
        if(argv[i][0] == '-')
        {
            if(strlen(argv[i])> 1)
            {
                //当前key是否已经有值了
                if(now_key)
                {
                    //加入不带值的参数
                    add(now_key, "");
                }

                //具体字符的首地址记录下
                now_key = argv[i] + 1;
            }
            else
            {
                KIT_LOG_ERROR(g_logger) << "invalid arg, index=" << i 
                    << ", val=" << argv[i];
                
                return false;
            }
        }
        else    //代表是参数的值val
        {
            //配置字符已经记录
            if(now_key)
            {
                add(now_key, argv[i]);
                now_key = nullptr;
            }
            else
            {
                KIT_LOG_ERROR(g_logger) << "invalid arg, index=" << i 
                    << ", val=" << argv[i];
                
                return false;
            }

        }
    }

    //当只有一个配置且没有val值 将其加入
    if(now_key)
        add(now_key, "");

    return true;

}


void Env::add(const std::string& key, const std::string& val)
{

    MutexType::WriteLock lock(m_mutex);
    m_args[key] = val;
}


bool Env::has(const std::string& key)
{
    MutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end(); 
}


std::string Env::get(const std::string& key, const std::string& default_val)
{
    MutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it == m_args.end() ? default_val : it->second;
}

void Env::del(const std::string& key)
{   
    MutexType::WriteLock lock(m_mutex);
    m_args.erase(key);
}

void Env::printArgs()
{
    MutexType::ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_programName << " [args]" << std::endl;
    for(auto &x : m_args)
    {
        std::cout << std::setw(5) << "-" << x.first << ":" << x.second << std::endl;
    }

}


void Env::addHelp(const std::string &key, const std::string& description)
{
    MutexType::WriteLock lock(m_mutex);
    for(auto &x : m_helps)
    {
        if(x.first == key)
        {
            x.second = description;
            return;
        }
    }

    m_helps.push_back({key, description});
}

void Env::delHelp(const std::string &key)
{
    MutexType::WriteLock lock(m_mutex);
    for(auto it = m_helps.begin();it != m_helps.end();++it)
    {
        if(it->first == key)
        {
            m_helps.erase(it);
            break;
        }
    }
}


void Env::printHelp()
{
    MutexType::ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_programName << " [options]" << std::endl;
    for(auto &x : m_helps)
    {
        std::cout << std::setw(5) << "-" << x.first << ": " << x.second << std::endl;
    }

}


std::string Env::getAbsolutePath(const std::string& path) const
{
    //如果是空路径返回根路径
    if(!path.size())
        return "/";

    //如果首字符以根路径开始 已经是绝对路径了
    if(path[0] == '/')
        return path;
    
    return m_cwd + path;
}

std::string Env::getConfigPath()
{
    return getAbsolutePath(get("c", "conf"));
}

bool Env::setEnv(const std::string& name, const std::string& val)
{
    return !setenv(name.c_str(), val.c_str(), 1);
}


std::string Env::getEnv(const std::string& name, const std::string&default_val)
{
    const char* v = getenv(name.c_str());
    return v ? std::string(v) : default_val;
}

}