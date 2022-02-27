#include "../kit_server/thread.h"
#include "../kit_server/Log.h"
#include "../kit_server/config.h"

#include <functional>
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <sys/time.h>

using namespace std;
using namespace kit_server;


long long int sum = 0;
RWMutex s_mutex;
Mutex m_mutex;
SpinMutex p_mutex;
CASMutex c_mutex;

#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000) 

void func1() 
{
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "func1 " << "name=" << Thread::_getName()
                            << " this.name=" << Thread::_getThis()->getName()
                            << " id=" << GetThreadId()
                            << " this.id=" << Thread::_getThis()->getId();
    for(int i = 0; i < 100000000;++i)
    {
        {
        //RWMutex::WriteLock lock(s_mutex);
        //RWMutex::ReadLock lock(s_mutex);
        Mutex::Lock lock(m_mutex);
        //CASMutex::Lock lock(c_mutex);
        //SpinMutex::Lock lock(p_mutex); 
        sum++;
        }
       
    }

    //sleep(5);
}


void func2()
{
    int i = 0;
    while(i++ < 20000000)
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    //std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" <<std::endl;
}

void func3()
{
    int i = 0;
    while(i++ < 20000000)
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "===================================================================================================================================================";
    //std::cout << "========================================" <<std::endl;
}

void func4()
{
    while(1)
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "66666666666666666666666666666666666666666666666666666";
    //std::cout << "========================================" <<std::endl;
}

void func5()
{
    while(1)
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";
    //std::cout << "========================================" <<std::endl;
}

void func6()
{
    int i = 0;
    while(i++ < 1000000)
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888";
    //std::cout << "========================================" <<std::endl;
}
   



int main()
{   
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "thread test begin!";

    // YAML::Node root = YAML::LoadFile("/home/nmoek/kit_server_project/tests/logs2.yaml");

    // Config::LoadFromYaml(root);
    
    vector<Thread::ptr> mv;

    struct timeval tv_begin;
    struct timeval tv_cur;
    gettimeofday(&tv_begin, nullptr);


    // for(int i = 0;i < 2;i++)
    // {
    //     Thread::ptr v(new Thread(&func2, "t_" + to_string(i * 2)));
    //     Thread::ptr v2(new Thread(&func3, "t_" + to_string(i * 2 + 1)));
    //     //Thread::ptr v3(new Thread(&func4, "t_" + to_string(i * 2 + 3)));
    //     //Thread::ptr v4(new Thread(&func5, "t_" + to_string(i * 2 + 3)));


    //     mv.push_back(v);
    //     mv.push_back(v2);
    //     //mv.push_back(v3);
    //     //mv.push_back(v4);
    // }


    // for(size_t i = 0; i < mv.size();i++)
    // {
    //     mv[i]->join();
    // }


    for(int i = 0;i < 5;i++)
    {
        Thread::ptr v(new Thread(&func1, "t_" + to_string(i)));
        //Thread::ptr v2(new Thread(&func3, "t_" + to_string(i * 2 + 1)));

        mv.push_back(v);
       // mv.push_back(v2);
    }


    for(size_t i = 0; i < 5;i++)
    {
        mv[i]->join();
    }


    KIT_LOG_INFO(KIT_LOG_ROOT()) << "thread test end";
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "sum=" << sum;
    memcpy(&tv_cur, &tv_begin, sizeof(struct timeval));
    gettimeofday(&tv_begin, nullptr);
    unsigned long int t = TIME_SUB_MS(tv_begin, tv_cur);
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "time=" << t << "ms";
   // std::cout << "sum=" << sum << std::endl;
    std::cout << "time=" << t << std::endl;
    

    
   
    return 0;
}