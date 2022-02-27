#include "../kit_server/env.h"
#include "../kit_server/Log.h"
#include "../kit_server/iomanager.h"

#include <fstream>


static kit_server::Logger::ptr g_logger = KIT_LOG_ROOT();


struct A
{
    A()
    {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);

        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        std::cout << "cmdline=" << std::endl;
        for(size_t i = 0;i < content.size();++i)
        {
            std::cout << i << "----"<< content[i] << "----" << (int)content[i] << std::endl; 
        }

    }
};

static A a;


void test_env(int argc, char *argv[])
{
    kit_server::Env* env = kit_server::EnvMgr::GetInstance();
    if(!env->init(argc, argv))
    {
        KIT_LOG_ERROR(g_logger) << "args parser error!";
        return;
    }   

    env->add("a", "6666");
    env->add("c", "8888");
    //env->del("b");
    env->addHelp("a", "hello");
    env->addHelp("b", "ahhahah");
    env->addHelp("c", "651651");
    env->printArgs();
    env->printHelp();
    // env->delHelp("c");
    env->del("c");
    if(env->has("a")) 
        {std::cout << "a存在" << std::endl;}
    else 
        {std::cout << "a不存在" << std::endl;}
    if(env->has("b")) 
        {std::cout << "b存在" << std::endl;}
    else 
        {std::cout << "b不存在" << std::endl;}

    if(env->has("c")) 
        {std::cout << "c存在" << std::endl;}
    else 
        {std::cout << "c不存在" << std::endl;}
        if(env->has("p")) 
        {std::cout << "p存在" << std::endl;}
    else 
        {std::cout << "p不存在" << std::endl;}

    env->printArgs();
    env->printHelp();

    std::cout << "exe=" << env->getExe() << std::endl;
    std::cout << "cwd=" << env->getCwd() << std::endl;
    std::cout << "path=" << env->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << env->getEnv("TEST", "aaa") << std::endl;
    env->setEnv("TEST", "666");
    std::cout << "test=" << env->getEnv("TEST", "aaa") << std::endl;
}


int main(int argc, char *argv[])
{
    KIT_LOG_INFO(g_logger) << "test begin";
    test_env(argc, argv);

    KIT_LOG_INFO(g_logger) << "test end";
    return 0;
}