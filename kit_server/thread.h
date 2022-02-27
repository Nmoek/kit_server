#ifndef _KIT_THREAD_H_
#define _KIT_THREAD_H_

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>

#include "mutex.h"
#include "noncopyable.h"


namespace kit_server
{


//线程
class Thread: Noncopyable
{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name = "");
    ~Thread();

    //回收函数
    void join();

    //获取线程ID
    pid_t getId() const {return m_id;}
    //获取线程名称
    const std::string& getName() const {return m_name;}


public:
    //获得管理当前线程局部变量的Thread类 this指针
    static Thread* _getThis();

    //获取当前线程局部变量名称
    static const std::string& _getName();

    //设置当前线程局部变量名称
    static void _setName(const std::string& name);


private:
    //禁止拷贝构造函数、赋值函数生效
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void *run(void *);

private:
    //线程ID  用户态的线程ID和内核线程ID不是一个概念 调试时候需要拿到内核中的ID
    pid_t m_id = -1;
    pthread_t m_thread = 0;   //unsigned long int
    //线程执行的回调函数
    std::function<void()> m_cb;
    //线程名称
    std::string m_name;

    //信号量
    Semaphore m_sem;

};

}




#endif