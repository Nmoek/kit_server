#include "../kit_server/Log.h"
#include "../kit_server/config.h"
#include "../kit_server/env.h"

#include <yaml-cpp/yaml.h>
#include <vector>

using namespace std;
using namespace kit_server;

static Logger::ptr g_logger = KIT_LOG_ROOT();

//ConfigVar<int>::ptr q_int_value_config(new ConfigVar<int>("6666", (int)9999, "66666"));
   
ConfigVar<int>::ptr q_int_value_config =
    Config::LookUp("system.port", (int)8080, "system port");

ConfigVar<float>::ptr q_float_value_config = 
    Config::LookUp("system.float", (float)63.6, "system float");

/*
ConfigVar<std::string>::ptr q_string_value_config = 
    Config::LookUp("logs.name", std::string("66666"), "logs name");

ConfigVar<int>::ptr q_levl_value_config = 
    Config::LookUp("logs.level", (int)LogLevel::DEBUG, "logs level");*/

//传入一个vector类型
ConfigVar<std::vector<int> >::ptr q_vec_value_config = 
    Config::LookUp("system.int_vec", std::vector<int>{1, 2}, "system int_vec");

ConfigVar<std::list<int> >::ptr q_list_value_config = 
    Config::LookUp("system.int_list", std::list<int>{2, 3}, "system int_list");

ConfigVar<std::set<int> >::ptr q_set_value_config = 
    Config::LookUp("system.int_set", std::set<int>{2, 3}, "system int_set");

ConfigVar<std::unordered_set<int> >::ptr q_uset_value_config = 
    Config::LookUp("system.int_uset", std::unordered_set<int>{10, 11}, "system int_uset");

ConfigVar<std::unordered_map<char, int> >::ptr q_umap_value_config = 
    Config::LookUp("system.umap", std::unordered_map<char, int>{{'a',1}, {'b',2}}, "system umap");

ConfigVar<std::map<char, int> >::ptr q_map_value_config = 
    Config::LookUp("system.map", std::map<char, int>{{'c',1}, {'a',2}, {'b', 3}}, "system map");

ConfigVar<std::vector<vector<int> > >::ptr q_vvec_value_config = 
    Config::LookUp("system.int_vvec", std::vector<vector<int> >{{1, 2},{3, 4}}, "system int_vvec");


void show_yaml(const YAML::Node& node, int level)
{

    //打印 Scalar 标量
    if(node.IsScalar())
    {
        KIT_LOG_INFO(KIT_LOG_ROOT()) << std::string(level * 4, '-') << node.Scalar() << " - " << node.Type() << " - "<< level;

    }
    else if(node.IsNull())
    {
        KIT_LOG_INFO(KIT_LOG_ROOT()) << std::string(level * 4, '-') <<  "NULL" << " - " << node.Type() << " - "<< level;
    }
    if(node.IsMap())
    {
        for(auto it = node.begin(); it != node.end();it++)
        {
            KIT_LOG_INFO(KIT_LOG_ROOT()) << std::string(level * 4, '-') << it->first << " - " << it->second.Type() << " - " << level;

            show_yaml(it->second, level + 1);
        }
    }
    else if(node.IsSequence())
    {
       for(size_t i = 0;i < node.size();i++)
       {
           KIT_LOG_INFO(KIT_LOG_ROOT())  << std::string(level * 4, '-') << "list对象索引: " << node[i].Type() <<" - "<< i << " - "<< level;
       
           show_yaml(node[i], level + 1);
       }

    }

}

/*
void show_yaml(const YAML::Node& node, int level)
{

    //打印 Scalar 标量
    if(node.IsScalar())
    {
        std:: cout << std::string(level * 4, '-') << node.Scalar() << " - " << node.Type() << " - "<< level << std::endl;
    }
    else if(node.IsNull())
    {
        std:: cout << std::string(level * 4, '-') << "NULL" << " - " << node.Type() << " - "<< level << std::endl;
    }
    if(node.IsMap())
    {   
        //由迭代器打印hash对象的值
        for(auto it = node.begin(); it != node.end();it++)
        {
            std::cout << std::string(level * 4, '-') << it->first << " - " << it->second.Type() << " - " << level<< std::endl;
            
            show_yaml(it->second, level + 1);
        }
    }
    else if(node.IsSequence())
    {
        //由下标索引遍历list中的Node对象
        for(size_t i = 0;i < node.size();i++)
        {
            std::cout << std::string(level * 4, '-') << "list对象索引: " << node[i].Type() <<" - "<< i << " - "<< level << std::endl;

            show_yaml(node[i], level + 1);
        }

    }

}*/


void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/nmoek/kit_server_project/tests/logs.yaml");

    show_yaml(root, 0);

    //KIT_LOG_INFO(KIT_LOG_ROOT()) << root;

}


void test_config()
{

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << q_int_value_config->getValue();

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << q_float_value_config->getValue();

    //KIT_LOG_INFO(KIT_LOG_ROOT()) << "before";
    //KIT_LOG_FMT_INFO(KIT_LOG_ROOT(), "before:", "");
    // auto v = q_vec_value_config->getValue();
    // for(auto &x : v)
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "before int_vec:" << x ;


#define XX(q_var, name, prefix)do{\
    auto v = q_var->getValue();\
    for(auto &x : v)\
    {\
        KIT_LOG_INFO(KIT_LOG_ROOT()) << #prefix " " #name ":" << x ;\
    }\
}while(0)

    //XX(q_vec_value_config, int_vec, before);
    //XX(q_list_value_config, int_list, before);
    //XX(q_set_value_config, int_set, before);
    //XX(q_uset_value_config, int_uset, before);
    // auto v = q_umap_value_config->getValue();
    // for(auto &x : v)
    // {
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << x.first << ":" << x.second;
    // }

    // auto v = q_map_value_config->getValue();
    // for(auto &x : v)
    // {
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << x.first << ":" << x.second;
    // }

    //KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << q_string_value_config->getValue();

   // KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << q_levl_value_config->getValue();

    //从.yaml文件导入配置信息
    YAML::Node root = YAML::LoadFile("/home/nmoek/kit_server_project/tests/config.yaml");

    //将YAML::Node中的层级结构进行序列化 并且自动完成配置更迭
    Config::LoadFromYaml(root);


    //XX(q_vec_value_config, int_vec, after);
    //XX(q_list_value_config, int_list, after);
   //XX(q_set_value_config, int_set, after);
    //XX(q_uset_value_config, int_uset, after);
    // auto t = q_umap_value_config->getValue();
    // for(auto &x : t)
    // {
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << x.first << ":" << x.second;
    // }

    // auto t = q_map_value_config->getValue();
    // for(auto &x : t)
    // {
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << x.first << ":" << x.second;
    // }

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << q_int_value_config->getValue();

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << q_float_value_config->getValue();

    // auto t = q_vec_value_config->getValue();
    // for(auto &x : t)
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "after int_vec:" << x; 

    // auto t = q_vvec_value_config->getValue();
    // index = 0;
    
    // for(auto &x : t)
    // {
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << index++; 
    //     for(auto &k : x)
    //         KIT_LOG_INFO(KIT_LOG_ROOT()) << "before int_vvec:" << k ;
    // }

    //KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << q_string_value_config->getValue();

    //KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << q_levl_value_config->getValue();

}

/******************************自定义类型**********************************/
namespace kit_server
{
class Person
{
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;
    Person(){};
    Person(const std::string& name, int age, bool sex)
        :m_name(name), m_age(age), m_sex(sex){}

public:
    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name = " << m_name
           << ", age=" << m_age
           << ", sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& p) const 
    {
        return m_name == p.m_name && m_age == p.m_age && m_sex == p.m_sex;
    }

};


//自定义类型偏特化  string--------->Person 序列化
template< >
class LexicalCast<std::string, Person>
{
public:

    Person operator()(const std::string &val)
    {
        Person p;
        YAML::Node node = YAML::Load(val);

        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();

        return p;
    }


};


//自定义类型偏特化  Person--------->string 反序列化
template<>
class LexicalCast<Person, std::string>
{
public:
    std::string operator()(const Person& p)
    {
        YAML::Node node;

        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;

        std::stringstream ss;
        ss << node;

        return ss.str();
    }

};
}



ConfigVar<Person>::ptr q_preson_config = Config::LookUp("class.person", Person("liu", 18, 0), "class person");

ConfigVar<std::map<std::string, Person> >::ptr q_map_preson_config = Config::LookUp("class.map", std::map<std::string, Person>{{"p1", {"qian", 19, false}}}, "class map");

void test_class()
{
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << q_preson_config->getValue().toString() << " - \n" << q_preson_config->toString();

    // auto t = q_map_preson_config->getValue();
    // for(auto &x : t)
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << x.first << " - " << x.second.toString();

    YAML::Node root = YAML::LoadFile("/home/nmoek/kit_server_project/tests/class.yaml");

    Config::LoadFromYaml(root);

    KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << q_preson_config->getValue().toString() << " - \n" << q_preson_config->toString();

    // auto v = q_map_preson_config->getValue();
    // for(auto &x : v)
    //     KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << x.first << " - " << x.second.toString();
}


void test_event()
{
    q_preson_config->addListener([](const Person& o, const Person& n){
        KIT_LOG_DEBUG(KIT_LOG_ROOT()) << "old_value=" << o.toString();
        KIT_LOG_DEBUG(KIT_LOG_ROOT()) << "new_value=" << n.toString();

    });
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << q_preson_config->getValue().toString() << " - \n" << q_preson_config->toString();

    YAML::Node root = YAML::LoadFile("/home/nmoek/kit_server_project/tests/class.yaml");

    Config::LoadFromYaml(root);

    KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:" << q_preson_config->getValue().toString() << " - \n" << q_preson_config->toString();


}



//kit_server::ConfigVar<std::set<kit_server::LogDefine> >::ptr g_log_defines = kit_server::Config::LookUp("logs", std::set<kit_server::LogDefine>(), "logs configs");

void test_log()
{

    auto t = KIT_LOG_NAME("system");

    KIT_LOG_INFO(t) << "system before: \n" <<  t->toYamlString();
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "all before:\n" << kit_server::LoggerMgr::GetInstance()->toYamlString();

    //KIT_LOG_DEBUG(KIT_LOG_ROOT()) << "before: \n" << kit_server::LoggerMgr::GetInstance()->toYamlString();

    //std::cout << "before:\n" << kit_server::LoggerMgr::GetInstance()->toYamlString() << std::endl;

    //KIT_LOG_INFO(KIT_LOG_ROOT()) << "before:" << g_log_defines->getValue().toString();

    YAML::Node root = YAML::LoadFile("/home/nmoek/kit_server_project/tests/logs.yaml");


    Config::LoadFromYaml(root);

    auto v = KIT_LOG_NAME("system");
    KIT_LOG_INFO(v) << "system after: \n" <<  v->toYamlString();
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "all after:\n" << kit_server::LoggerMgr::GetInstance()->toYamlString();

    v->setFormatter("%d - %m%n");
    KIT_LOG_INFO(v) << "hello log";

    


    //std::cout << "after:\n" << kit_server::LoggerMgr::GetInstance()->toYamlString() << std::endl;

    //KIT_LOG_INFO(KIT_LOG_ROOT()) << "after:\n" << g_log_defines->getValue().toString();
    //KIT_LOG_DEBUG(KIT_LOG_ROOT()) << ".yaml文件:" << root;

    //KIT_LOG_DEBUG(KIT_LOG_ROOT()) << "after: \n" << kit_server::LoggerMgr::GetInstance()->toYamlString();
}


void test_loadconf()
{
    kit_server::Config::LoadFromConfigDir("conf");
    std::cout << "------------------------" << std::endl;
    kit_server::Config::LoadFromConfigDir("conf");
}



int main(int argc, char* argv[])
{

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << q_int_value_config->getValue();

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << q_int_value_config->toString();

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << "test flaot" << q_float_value_config->getValue();
    
    // test_yaml();

    //test_config();

    //test_class();
    //test_event();
    // test_log();

    kit_server::Env* env = kit_server::EnvMgr::GetInstance();
    if(!env->init(argc, argv))
    {
        KIT_LOG_ERROR(g_logger) << "args init fail!";
        return 0;
    }

    test_loadconf();

    // Config::Visit([](ConfigVarBase::ptr val){

    // KIT_LOG_INFO(KIT_LOG_ROOT()) << "\nname=" << val->getName()
    //                                  << "\ndescription=" << val->getDescription()
    //                                  << "\ntype.name=" << val->getTypeName()
    //                                  << "\nvalue=\n" << val->toString();
    // });

    return 0;
}