#ifndef _KIT_COUROUTINE_H_
#define _KIT_COUROUTINE_H_

#include <ucontext.h>
#include <memory>
#include <functional>
#include "thread.h"

namespace kit_server
{

//
class Coroutine: public std::enable_shared_from_this<Coroutine>
{
public:
    //协程状态
    enum State
    {
        INIT,    //初始状态
        HOLD,    //挂起状态
        EXEC,    //运行状态
        TERM,    //终止状态
        READY,    //就绪状态
        EXCEPTION
    };
public:
    typedef std::shared_ptr<Coroutine> ptr;

    Coroutine(std::function<void()> cb, size_t stack_size = 0, bool use_call = false);
    ~Coroutine();

    //给协程体重置  INIT/TERM状态下才能重置
    void reset(std::function<void()> cb);

    //从init协程 切换到 到目标代码序列
    void swapIn();
    //从目标代码序列切换到init协程
    void swapOut();

    //从调度器协程 切换到 到目标代码序列
    void call();
    //从到目标代码序列 切换到 调度器协程
    void back();

    //获取协程ID
    uint64_t getID() const {return m_id;}

    //获得协程状态
    State getState() const;
    //设置协程状态
    void setState(State state);

public:
    //初始化母协程
    static void Init();
    //设置当前在执行的协程
    static void SetThis(Coroutine *c);
    //返回当前在执行的协程
    static Coroutine::ptr GetThis();
    //协程让出 置为READY状态
    static void YieldToReady();
    //协程让出 置为HOLD状态
    static void YieldToHold();
    //获得当前总协程数
    static uint64_t TotalCoroutines();
    
    //协程需要执行的任务函数
    static void MainFunc();

    static void CallMainFunc();
    //获得协程ID
    static uint64_t GetCoroutineId();


private:
    //负责生成init协程
    Coroutine();


private:
    //协程ID
    uint64_t m_id = 0;
    //用户栈大小
    uint32_t m_stack_size = 0;
    //协程状态
    State m_state = INIT;
    //协程携带的程序上下文
    ucontext_t m_ctx;
    //用户栈地址
    void *m_stack = nullptr;
    //协程执行的回调函数
    std::function<void()> m_cb;


};
}


#endif