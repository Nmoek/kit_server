#ifndef _KIT_ENV_H_
#define _KIT_ENV_H_


#include "single.h"
#include "mutex.h"

#include <memory>
#include <map>
#include <vector>


namespace kit_server
{

/**
 * @brief 解析外部输入参数以及环境变量
 */
class Env
{
public:
    typedef std::shared_ptr<Env> ptr;
    typedef RWMutex MutexType;

    /**
     * @brief 解析传入的外部参数以及获取可执行文件的绝对路径
     * @param[in] argc 传入的参数个数
     * @param[in] argv 传入的参数数值组
     * @return 返回解析是否成功
     */
    bool init(int argc, char* argv[]);

    /**
     * @brief 新添加参数信息
     * @param[in] key 参数的key值
     * @param[in] val 参数的val值
     */
    void add(const std::string& key, const std::string& val);

    /**
     * @brief 查询参数是否存在
     * @param[in] key 传入要查询的参数key值
     * @return 返回存在与否
     */
    bool has(const std::string& key);

    /**
     * @brief 获取对应的参数信息
     * @param[in] key 参数的key值
     * @param[in] default_val 参数的val值 如果不存在返回默认值
     * @return 返回参数信息中的val值
     */
    std::string get(const std::string& key, const std::string& default_val = "");

    /**
     * @brief 删除对应的参数信息
     * @param[in] key 参数的key值
     */
    void del(const std::string& key);

    /**
     * @brief 打印所有的参数键值对
     */
    void printArgs();

    /**
     * @brief 添加参数信息的说明
     * @param[in] key 参数的key值
     * @param description 参数补充说明信息
     */
    void addHelp(const std::string &key, const std::string& description);

    /**
     * @brief 删除参数信息的说明
     * @param[in] key 参数的key值
     */
    void delHelp(const std::string &key);

    /**
     * @brief 打印所有的参数说明信息
     */
    void printHelp();

    /**
     * @brief 获取当前可执行文件的绝对路径
     * @return 返回一个路径的字符串
     */
    const std::string& getExe() const {return m_exe;}

    /**
     * @brief 获取当前可执行文件的文件夹的绝对路径
     * @return 返回一个路径的字符串
     */
    const std::string& getCwd() const {return m_cwd;}


    /**
     * @brief 获取并且转换一个路径的绝对路径
     * @param[in] path 传入的需要转换的路径
     * @return 返回转换好的绝对路径
     */
    std::string getAbsolutePath(const std::string& path) const;

    /**
     * @brief 获取配置文件的绝对路径
     * @return 返回路径的字符串 
     */
    std::string getConfigPath();

    /**
     * @brief 设置环境变量
     * @param name 环境变量的名称key值
     * @param val 环境变量的值val值
     * @return 返回是否设置成功
     */
    bool setEnv(const std::string& name, const std::string& val);

    /**
     * @brief 获取环境变量
     * @param name 环境变量的名称key值
     * @param default_val 环境变量的值val默认值
     * @return 返回环境变量对应val值字符串
     */
    std::string getEnv(const std::string& name, const std::string&default_val = "");

private:
    //读写锁
    MutexType m_mutex;
    //参数集合
    std::map<std::string, std::string> m_args;
    //对参数的解释说明
    std::vector<std::pair<std::string, std::string> > m_helps;
    //当前运行程序名称
    std::string m_programName;
    //二进制可执行文件的绝对路径
    std::string m_exe;
    //二进制可执行文件的包含文件夹的绝对路径
    std::string m_cwd;

};
typedef Single<Env> EnvMgr;

}

#endif