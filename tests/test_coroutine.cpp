#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"


using namespace std;
using namespace kit_server;



void func_in()
{
  
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "func1 begin";

    Coroutine::YieldToHold();

    KIT_LOG_INFO(KIT_LOG_ROOT()) << "func1 end";

    //Coroutine::GetThis()->YieldToHold();
}




void test_coroutine()
{
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "sub_thread test begin";
    //初始化母协程
    Coroutine::Init();
    //创建一个子协程
    Coroutine::ptr cor1(new Coroutine(func_in));

    cor1->swapIn();
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "1 swapIn after";

    cor1->swapIn();

    KIT_LOG_INFO(KIT_LOG_ROOT()) << "2 swapIn after";

    KIT_LOG_INFO(KIT_LOG_ROOT()) << "sub_thread test end";
}



int main()
{
    Thread::SetName("main thread");
    vector<Thread::ptr> mv;

    //启动3个线程 每个线程上有2个协程：1个母协程 1个子协程
    for(int i = 0; i < 3;i++)
    {
        Thread::ptr p(new Thread(&test_coroutine, "t_" + to_string(i)));
        mv.emplace_back(p);
    }

    for(auto &x : mv)
        x->join();

  


    return 0;
}