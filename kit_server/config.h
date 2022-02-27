#ifndef _KIT_CONFIG_H_
#define _KIT_CONFIG_H_

#include <iostream>
#include <memory>
#include <sstream>
#include <algorithm>
#include <string>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <stdint.h>


#include "Log.h"
#include "single.h"
#include "util.h"
#include "thread.h"



namespace kit_server
{

/**
 * @brief 配置项虚基类
 */
class ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    /**
     * @brief 配置项虚基类构造函数
     * @param[in] name  配置项名称 
     * @param[in] description 配置项描述
     */
    ConfigVarBase(const std::string & name, const std::string &description = "")
        :m_name(name), 
        m_description(description)
    {
        //把配置项的命名限制在小写范围内 输入的大写全部转为小写
        //std::transform(m_name.begin(), m_name.end(), m_name.begin(), tolower);

        std::for_each(m_name.begin(), m_name.end(), [&](char &c){ if(c >= 'A' && c <= 'Z') c += 32;});
    }

    /**
     * @brief 配置项虚基类析构函数
     */
    virtual ~ConfigVarBase(){}

    /**
     * @brief 获取配置项名称
     * @return const std::string& 
     */
    const std::string & getName() const {return m_name;}

    /**
     * @brief 获取配置项描述
     * @return const std::string& 
     */
    const std::string& getDescription() const {return m_description;}

    /**
     * @brief 将配置项数据内容转换为string类型
     * @return std::string 
     */
    virtual std::string toString() = 0;

    /**
     * @brief 将配置项从string类型转换为其他所需类型
     * @param[in] val 传入的字符串数据
     * @return true 转换成功
     * @return false 转换失败
     */
    virtual bool fromString(const std::string &val) = 0;

    /**
     * @brief 获取配置项类型名称
     * @return std::string 
     */
    virtual std::string getTypeName() const = 0;

protected:
    /// 配置项名称
    std::string m_name;
    /// 配置项描述
    std::string m_description;

};


/**
 * @brief 类型转换，只完成基础类型转换 type F ---> type T   
 * @tparam F 待转换类型
 * @tparam T 转换后类型
 */
template<class F, class T>
class LexicalCast
{
public:
    T operator()(const F& v)
    {
        return boost::lexical_cast<T>(v);
    }

};

/**********************************vector*********************************/
//vector模板偏特化 string---->vector<>
template<class T>
class LexicalCast<std::string, std::vector<T> >
{
public:
    typename std::vector<T> operator()(const std::string& val)
    {
        typename std::vector<T> mv;
        

        //利用到YAML库的Load()  string----->YAML::Node
        YAML::Node node = YAML::Load(val);
        for(size_t i = 0;i < node.size();++i)
        {
            std::stringstream ss;
            ss << node[i];
            //递归调用解析
            mv.push_back(LexicalCast<std::string, T>()(ss.str()));
        }

        return mv;
    }

};

//vector模板偏特化 vector<>---->string
template<class F>
class LexicalCast<std::vector<F>, std::string>
{
public:
    std::string operator()(const std::vector<F>& mv)
    {
        YAML::Node node;
      
        for(auto&x : mv)
        {
            node.push_back(YAML::Load(LexicalCast<F, std::string>()(x)));
        }

        std::stringstream ss;
        ss << node;
    
        return ss.str();
    }

};


/**********************************list************************************/
//list模板偏特化 string---->list<>
template<class T>
class LexicalCast<std::string, std::list<T> >
{
public:
    typename std::list<T> operator()(const std::string& val)
    {
        typename std::list<T> mv;
        

        //利用到YAML库的Load()  string----->YAML::Node
        YAML::Node node = YAML::Load(val);
        for(size_t i = 0;i < node.size();++i)
        {
            std::stringstream ss;
            ss << node[i];
            //递归调用解析
            mv.push_back(LexicalCast<std::string, T>()(ss.str()));
        }

        return mv;
    }

};

//list模板偏特化 list<>---->string
template<class F>
class LexicalCast<std::list<F>, std::string>
{
public:
    std::string operator()(const std::list<F>& mv)
    {
        YAML::Node node;
      
        for(auto&x : mv)
        {
            node.push_back(YAML::Load(LexicalCast<F, std::string>()(x)));
        }

        std::stringstream ss;
        ss << node;
    
        return ss.str();
    }

};

/*********************************set**************************************/
//set模板偏特化 string---->set<>
template<class T>
class LexicalCast<std::string, std::set<T> >
{
public:
    typename std::set<T> operator()(const std::string& val)
    {
        typename std::set<T> mv;
        
        //利用到YAML库的Load()  string----->YAML::Node
        YAML::Node node = YAML::Load(val);
        for(size_t i = 0;i < node.size();++i)
        {
            std::stringstream ss;
            ss << node[i];
            //递归调用解析
            mv.insert(LexicalCast<std::string, T>()(ss.str()));
        }

        return mv;
    }

};

//set模板偏特化 set<>---->string
template<class F>
class LexicalCast<std::set<F>, std::string>
{
public:
    std::string operator()(const std::set<F>& mv)
    {
        YAML::Node node;
      
        for(auto&x : mv)
        {
            node.push_back(YAML::Load(LexicalCast<F, std::string>()(x)));
        }

        std::stringstream ss;
        ss << node;
    
        return ss.str();
    }

};


/*********************************unordered_set****************************/
//unordered_set模板偏特化 string---->unordered_set<>
template<class T>
class LexicalCast<std::string, std::unordered_set<T> >
{
public:
    typename std::unordered_set<T> operator()(const std::string& val)
    {
        typename std::unordered_set<T> mv;
        
        //利用到YAML库的Load()  string----->YAML::Node
        YAML::Node node = YAML::Load(val);
        for(size_t i = 0;i < node.size();++i)
        {
            std::stringstream ss;
            ss << node[i];
            //递归调用解析
            mv.insert(LexicalCast<std::string, T>()(ss.str()));
        }

        return mv;
    }

};

//set模板偏特化 set<>---->string
template<class F>
class LexicalCast<std::unordered_set<F>, std::string>
{
public:
    std::string operator()(const std::unordered_set<F>& mv)
    {
        YAML::Node node;
      
        for(auto&x : mv)
        {
            node.push_back(YAML::Load(LexicalCast<F, std::string>()(x)));
        }

        std::stringstream ss;
        ss << node;
    
        return ss.str();
    }

};


/**********************************map**************************************/
//map模板偏特化 string---->map<>
template<class T1, class T2>
class LexicalCast<std::string, std::map<T1, T2> >
{
public:
    typename std::map<T1, T2> operator()(const std::string& val)
    {
        typename std::map<T1, T2> mv;
        
        //利用到YAML库的Load()  string----->YAML::Node
        YAML::Node node = YAML::Load(val);
        for(auto it = node.begin();it != node.end();++it)
        {
            std::stringstream ss1, ss2;
            ss1 << it->first;
            ss2 << it->second;
            //递归调用解析
            mv.insert({LexicalCast<std::string, T1>()(ss1.str()), LexicalCast<std::string, T2>()(ss2.str())});
        }

        return mv;
    }

};

//map模板偏特化 map<>---->string
template<class F1, class F2>
class LexicalCast<std::map<F1, F2>, std::string>
{
public:
    std::string operator()(const std::map<F1, F2>& mv)
    {
        YAML::Node node;
      
        for(auto&x : mv)
        {
            node[YAML::Load(LexicalCast<F1, std::string>()(x.first))] = YAML::Load(LexicalCast<F2, std::string>()(x.second));
        }

        std::stringstream ss;
        ss << node;
    
        return ss.str();
    }

};


/**********************************unordered_map****************************/
//unordered_map模板偏特化 string---->unordered_map<>
template<class T1, class T2>
class LexicalCast<std::string, std::unordered_map<T1, T2> >
{
public:
    typename std::unordered_map<T1, T2> operator()(const std::string& val)
    {
        typename std::unordered_map<T1, T2> mv;
        
        //利用到YAML库的Load()  string----->YAML::Node
        YAML::Node node = YAML::Load(val);
        for(auto it = node.begin();it != node.end();++it)
        {
            std::stringstream ss1, ss2;
            ss1 << it->first;
            ss2 << it->second;
            //递归调用解析
            mv.insert({LexicalCast<std::string, T1>()(ss1.str()), LexicalCast<std::string, T2>()(ss2.str())});
        }

        return mv;
    }

};

//unordered_map模板偏特化 unordered_map<>---->string
template<class F1, class F2>
class LexicalCast<std::unordered_map<F1, F2>, std::string>
{
public:
    std::string operator()(const std::unordered_map<F1, F2>& mv)
    {
        YAML::Node node;
      
        for(auto&x : mv)
        {
            node[YAML::Load(LexicalCast<F1, std::string>()(x.first))] = YAML::Load(LexicalCast<F2, std::string>()(x.second));
        }

        std::stringstream ss;
        ss << node;
    
        return ss.str();
    }

};


/**
 * @brief 配置项类
 * @tparam T 维护配置项的数据类型
 * @tparam FromStr 从字符串转为T类型采用的模板类
 * @tparam ToStr 从T类型转为字符串类型采用的模板类
 */
template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr =  LexicalCast<T, std::string> >
class ConfigVar :public ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;
    typedef RWMutex MutexType;

    /**
     * @brief 配置项类构造函数
     * @param[in] name 配置项名称，作为key值
     * @param[in] default_value 配置项的默认值
     * @param[in] description 配置项的描述
     */
    ConfigVar(const std::string &name, const T& default_value, const std::string description = "")
        :ConfigVarBase(name, description)
        ,m_val(default_value)
    {

    }


    /**
     * @brief 将配置项数据内容转换为string类型
     * @return std::string 
     */
    std::string toString() override 
    {
        try{
            //return boost::lexical_cast<std::string>(m_val);
            //加读锁
            MutexType::ReadLock lock(m_mutex);
            //调用函数对象
            return ToStr()(m_val);
            
        }catch (std::exception &e){

            KIT_LOG_ERROR(KIT_LOG_ROOT()) << "ConfigVar::toString exception" << e.what() << " convert:" << std::string(typeid(m_val).name()) << " to string"; 
        }

        return "";
    }

    /**
     * @brief 将配置项从string类型转换为其他所需类型
     * @param[in] val 传入的字符串数据
     * @return true 转换成功
     * @return false 转换失败
     */
    bool fromString(const std::string &v) override
    {
        try{

            setValue(FromStr()(v));

        }catch(std::exception &e){
            KIT_LOG_ERROR(KIT_LOG_ROOT()) << "ConfigVar::fromString exception  " << e.what() << " convert: string to " << std::string(typeid(m_val).name()); 
        }

        return false;
    }

    /**
     * @brief 获取配置项类型名称
     * @return std::string 
     */
    std::string getTypeName() const override { return typeid(T).name();}

    /**
     * @brief 获取配置项，传入什么类型就返回什么类型
     * @return const T 
     */
    const T getValue()
    {
        MutexType::ReadLock lock(m_mutex);
        return m_val;
    }

    /**
     * @brief 给配置项设置具体的数值，从.yaml读到信息后同步到内存中来
     * @param val 
     */
    void setValue(const T& val) 
    {
        {
        //加读锁
        MutexType::ReadLock lock(m_mutex);
        //原来的值是否和新值一样 就不用设置
        if(val == m_val)
            return;

        //一个配置项可设置多个回调函数进行触发
        for(auto &x : m_cbs) 
            x.second(m_val, val);  //调用回调函数执行相关操作

        }//出作用域 读锁解锁

        //加写锁
        MutexType::WriteLock lock(m_mutex);
        m_val = val;
    }

    /**
     * @brief 为配置项绑定一个回调函数
     * @param[in] cb 指定的回调函数
     * @return uint64_t 返回一个唯一标识回调函数的key值
     */
    uint64_t addListener(on_change_cb cb) 
    {
        //内部自己生成一个key(id)来唯一标识回调函数 key不能重复
        static uint64_t cb_fun_id = 0;
        
        //加写锁
        MutexType::WriteLock lock(m_mutex);
        ++cb_fun_id;
        m_cbs[cb_fun_id] = cb;

        return cb_fun_id;
    }

    /**
     * @brief 删除配置项上的某一回调函数
     * @param[in] key 唯一标识回调函数的key值
     */
    void delListener(uint64_t key) 
    {
        MutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }
  
    /**
     * @brief 获取配置项上的某一回调函数
     * @param[in] key 唯一标识回调函数的key值
     * @return on_change_cb 
     */
    on_change_cb getListener(uint64_t key)
    {
        MutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        
        return it == m_cbs.end() ? nullptr : it->second;
    }

    /**
     * @brief 清除配置项上的所有回调函数
     */
    void clearListener()
    {
        MutexType::ReadLock lock(m_mutex);
        m_cbs.clear();
    }


private:
    /// 配置项数据
    T m_val;
    /// 配置项所绑定的回调函数容器
    std::map<uint64_t, on_change_cb> m_cbs;
    /// 读写锁
    MutexType m_mutex;
};

/**
 * @brief 配置项管理类
 */
class Config
{
public:
    //将配置项容器重命名 方便使用
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex MutexType;

    /**
     * @brief 查询配置项，如果没有就会创建
     * @tparam T 配置项的参数类型
     * @param[in] name 配置项名称
     * @param[in] default_value 配置项默认值
     * @param[in] description 配置项描述
     * @return ConfigVar<T>::ptr 
     */
    template<class T>
    static typename ConfigVar<T>::ptr LookUp(const std::string &name, const T& default_value, 
        const std::string &description = "")
    {
        //加写锁
        MutexType::WriteLock lock(GetMutex());

        //判断查询的配置项是否存在
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()) //配置项存在
        {
            auto temp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
            if(temp)
            {
                // KIT_LOG_INFO(KIT_LOG_ROOT()) << "LookUp name:" << name << " exists";
                return temp;
            }
            else
            {
                KIT_LOG_ERROR(KIT_LOG_ROOT()) << "LookUp name:" << name << " exists but not =" << typeid(T).name() << " real-type=" << it->second->getTypeName();

                return nullptr;
            }
                
        }

        /*配置项不存在就创建*/
        //排除一些非法的配置项名称
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz_.0123456789") != std::string::npos)
        {
            KIT_LOG_ERROR(KIT_LOG_ROOT()) << "LookUp name invalid " << name;
            //抛出无效参数异常
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));

        GetDatas()[name] = v;
      
        return v;

    }

    /**
     * @brief 查询配置项，如果没有返回nullptr
     * @tparam T 配置项的参数类型
     * @param[in] name  配置项名称
     * @return ConfigVar<T>::ptr 
     */
    template<class T>
    static typename ConfigVar<T>::ptr LookUp(const std::string &name)
    {
        //加读锁
        MutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end())
        {
            return nullptr;
        }

        //当转化对象为智能指针时使用dynamic_pointer_cast 
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }

    /**
     * @brief 从yaml结点中加载配置项数据
     * @param[in] node 带有数据的yaml结点 
     */
    static void LoadFromYaml(const YAML::Node & node);

    /**
     * @brief 以文件夹为单位加载配置文件
     * @param[in] path 配置文件夹的路径
     */
    static void LoadFromConfigDir(const std::string& path);

    /**
     * @brief 返回配置项的基类指针
     * @param[in] name 配置项名称 
     * @return ConfigVarBase::ptr 
     */
    static ConfigVarBase::ptr LookUpBase(const std::string& name);

    /**
     * @brief 轮询对所有配置项进行某个操作
     * @param[in] cb 指定一个函数去操作配置项
     */
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
    //判断传入的name是否合法 只能包含"abcdefghijklmnopqrstuvwxyz_.0123456789"这些字符
    bool isRight(const std::string &str)
    {
        for(auto &x : str)
        {
            if((x < 'a' || x > 'z') && (x < 'A' || x > 'Z') && !isdigit(x) && x != '.' && x != '_')
                return false;
        }

        return true;
    }

private:
    /**
     * @brief 获取配置项容器，由于初始化顺序问题写成static函数形式
     * @return ConfigVarMap& 
     */
    static ConfigVarMap& GetDatas()
    {
        static ConfigVarMap m_datas;
        return m_datas;
    }

    /**
     * @brief 获取读写锁
     * @return MutexType& 
     */
    static MutexType& GetMutex()
    {
        static MutexType m_mutex;
        return m_mutex;
    }

};


}

#endif