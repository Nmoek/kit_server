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


/**
 * @brief 线程类
 */
class Thread: Noncopyable
{
public:
    typedef std::shared_ptr<Thread> ptr;

    /**
     * @brief 线程类构造函数
     * @param[in] cb 线程要执行的函数
     * @param[in] name 线程名字
     */
    Thread(std::function<void()> cb, const std::string& name = "");

    /**
     * @brief 线程类析构函数
     */
    ~Thread();

    /**
     * @brief 线程回收处理函数
     */
    void join();

    /**
     * @brief 获取线程ID
     * @return pid_t 
     */
    pid_t getId() const {return m_id;}

    /**
     * @brief 获取线程名称
     * @return const std::string& 
     */
    const std::string& getName() const {return m_name;}

public:
    /**
     * @brief 获取当前正在运行的线程指针
     * @return Thread* 
     */
    static Thread* GetThis();

    /**
     * @brief 获取当前运行线程的名称
     * @return const std::string& 
     */
    static const std::string& GetName();

    /**
     * @brief 设置当前运行线程的名称 
     * @param[in] name 线程名称 
     */
    static void SetName(const std::string& name);

private:
    ///禁止拷贝构造函数、赋值函数生效
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    /**
     * @brief 线程回调函数（固定格式）
     * @return void* 
     */
    static void *run(void *);

private:
    ///线程ID  用户态的线程ID和内核线程ID不是一个概念 调试时候需要拿到内核中的ID
    pid_t m_id = -1;
    ///线程号
    pthread_t m_thread = 0;   //unsigned long int
    ///线程执行的回调函数
    std::function<void()> m_cb;
    ///线程名称
    std::string m_name;
    //信号量对象
    Semaphore m_sem;

};

}




#endif