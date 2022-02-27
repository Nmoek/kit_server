#include "thread.h"
#include "Log.h"
#include "util.h"
#include "mutex.h"
#include <iostream>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

/**************************************Thread*******************************/
//线程局部变量
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "unknow";

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
{
    m_name = name.size() > 0 ? name : "UNKNOW";

    //创建线程
    int ret = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(ret < 0)
    {
        KIT_LOG_ERROR(g_logger) << "Thread:: pthread_create fail, ret= " << ret << " thread name= " << m_name;

        throw std::logic_error("pthread_create fail!!");
    }

    //试想 当执行完上面的API线程可能不会马上运行 手动等待一下直到线程完全开始运行
    m_sem.wait();

}

void *Thread::run(void * arg)
{
    //接收传入的this指针 因为是个static方法 因此依靠传入this指针很重要
    Thread* thread = (Thread*)arg;

    //初始化线程局部变量
    t_thread = thread;
    t_thread_name = thread->m_name;

    //获取内核线程ID
    thread->m_id = GetThreadId();  //util.cpp定义的方法

    //给内核线程命名 但是只能接收小于等于16个字符
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    //迷惑点 防止m_cb中智能指针的引用不被释放 交换一次会减少引用次数
    std::function<void()> cb;
    cb.swap(thread->m_cb);  

    //确保线程已经完全初始化好了 再进行唤醒
    thread->m_sem.notify();

    cb();

    //g++编译器要求返回值
    return 0;
}


Thread:: ~Thread()
{
    if(m_thread) //析构时候不马上杀死线程 而是将其置为分离态
        pthread_detach(m_thread);

}


//阻塞态函数
void Thread::join()
{
    if(m_thread)
    {
        int ret = pthread_join(m_thread, nullptr);
        if(ret < 0)
        {
            KIT_LOG_ERROR(g_logger) << "Thread:: pthread_join fail, ret= " << ret << " thread name= " << m_name;
    
            throw std::logic_error("pthread_join fail!!");
        }

        m_thread = 0;
    }  

}

//获得管理当前线程的对象this指针
Thread* Thread::_getThis()
{
    return t_thread;
}

//获取当前线程局部变量 命名
const std::string& Thread::_getName()
{
    return t_thread_name;
}

//设置当前线程局部变量 命名
void Thread::_setName(const std::string& name)
{
    if(t_thread)
    {
        t_thread->m_name = name;
    }

    t_thread_name = name;
}



    
} 


