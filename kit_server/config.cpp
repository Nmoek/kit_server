#include "config.h"
#include "env.h"
#include "util.h"
#include "Log.h"
#include "mutex.h"

#include <list>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>



namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

//static 变量记得初始化
//Config::ConfigVarMap Config::m_datas;

//返回多态接口 找是否存在匹配的 ConfigVar模块   ConfigVarBase---->ConfigVar
ConfigVarBase::ptr Config::LookUpBase(const std::string& name)
{
    //加读锁
    MutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);

    return it == GetDatas().end() ? nullptr : it->second;
}


/**
 * @brief 将yaml结点数据序列化
 * @param[in] prefix 格式控制+key值组合
 * @param[in] node 带有数据的yaml结点
 * @param[out] output 存储序列化结果的容器
 */
static void ListAllMember(const std::string& prefix, const YAML::Node& node, std::list<std::pair<std::string, YAML::Node> >&output)
{
    //检查格式控制字符串的合法性
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxzy._0123456789") != std::string::npos)
    {
        KIT_LOG_ERROR(KIT_LOG_ROOT()) << "Config invauld nmae:" <<  prefix << ":" << node;

        throw std::invalid_argument(prefix);
    }

    //将 key:Node 存储到容器中
    output.push_back({prefix, node});

    //如果为Map类型 递归处理
    if(node.IsMap())
    {  
        for(auto it = node.begin();it != node.end();++it)
        {
            //进行递归
            ListAllMember(prefix.size() ? prefix + '.' + it->first.Scalar() : it->first.Scalar(), it->second, output);
        }
    }
    // else if(node.IsSequence())
    // {
    //     for(size_t i = 0;i < node.size();i++)
    //     {
           
    //         std::string t = prefix.size() ? prefix + node[i].Scalar() : node[i].Scalar();
    //         if(i != node.size() - 1 && !node[i].Scalar().size())
    //             t += '.';

    //         ListAllMember(prefix, node[i], output);
    //     }
    // }

}


void Config::LoadFromYaml(const YAML::Node &root)
{
    std::list<std::pair<std::string, YAML::Node> > all_nodes;
   // std::map<std::string, YAML::Node> all_nodes;
    ListAllMember("", root, all_nodes);


    // std::cout << "list size:" << all_nodes.size() << std::endl;
    // int index = 0;
    for(auto &x : all_nodes)
    {

        std::string key = x.first;

        // std::cout << index++ << "::";
        // if(!key.size())
        //     std::cout << "NULL";
        // else 
        //     std::cout << x.first;
        // std::cout << " --- " << x.second << std::endl;
        
        //空字符串就跳过
        if(!key.size())
            continue;

        //将配置项名称中的大写字母全部转为小写字母
        std::for_each(key.begin(), key.end(), [&](char &c){ if(c >= 'A' && c <= 'Z') c += 32;});

        //寻找是否有已经约定的配置项，如果有就进行修改
        ConfigVarBase::ptr v = LookUpBase(key);
        if(v)
        {
            //如果当前yaml结点是标量型数据直接进行转化
            if(x.second.IsScalar())
            {
                v->fromString(x.second.Scalar());
            }
            else    //如果当前yaml结点是复合数据需要以流的方式传入参数
            {
                std::stringstream ss;
                ss << x.second;
                v->fromString(ss.str());
            }
        }
        
    }
  
}

/**
 * @brief 记录配置文件最后的修改时间的容器
 */
static std::map<std::string, uint64_t> s_file2modify_time;

/**
 * @brief 对修改和读取配置文件最后的修改时间的互斥锁
 */
static Mutex s_mutex;

void Config::LoadFromConfigDir(const std::string& path)
{
    //转换为当前运行路径下的绝对路径
    std::string absolute_path = EnvMgr::GetInstance()->getAbsolutePath(path);

    KIT_LOG_DEBUG(g_logger) << "绝对路径=" << absolute_path;

    std::vector<std::string> files;
    //找后缀为.yaml的文件
    FSUtil::ListAllFile(files, absolute_path, ".yaml");

    //只有当配置文件真的发生修改才去加载这个文件
    //通过查看文件的最后修改时间实现
    for(auto &x : files)
    {
        //读取文件信息
        struct stat st;
        if(lstat(x.c_str(), &st) < 0)
        {
            KIT_LOG_ERROR(g_logger) << "lstat error, errno" << errno
                << ", is:" << strerror(errno);
            return;
        }

        //加锁 读取和修改 文件最后修改时间
        {
            Mutex::Lock lock(s_mutex);
            if(s_file2modify_time[x] == (uint64_t)st.st_mtim.tv_sec)
                continue;
            
            s_file2modify_time[x] = (uint64_t)st.st_mtim.tv_sec;
        }

        try
        {
            YAML::Node root = YAML::LoadFile(x);
            LoadFromYaml(root);
            KIT_LOG_INFO(g_logger) << "load config file from .yaml OK, name=" << x;
        }
        catch(...)
        {
            KIT_LOG_ERROR(g_logger) << "load config file error, name=" << x;
        }
        
    }
}


//便于查看调试已经有什么配置项
void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
{
    MutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();

    for(auto &x : m)
    {
        cb(x.second);
    }
}
    
}