#ifndef _KIT_COUROUTINE_H_
#define _KIT_COUROUTINE_H_

#include <ucontext.h>
#include <memory>
#include <functional>
#include "thread.h"

namespace kit_server
{

/**
 * @brief 协程类
 */
class Coroutine: public std::enable_shared_from_this<Coroutine>
{
public:
    /**
     * @brief 协程运行状态枚举类型
     */
    enum State
    {
        INIT,    //初始状态
        HOLD,    //挂起状态
        EXEC,    //运行状态
        TERM,    //终止状态
        READY,    //就绪状态
        EXCEPTION //异常状态
    };
public:
    typedef std::shared_ptr<Coroutine> ptr;

    /**
     * @brief 协程类构造函数
     * @param[in] cb 指定的执行函数
     * @param[in] stack_size 协程栈空间大小
     * @param[in] use_call 是否作为调度协程使用
     */
    Coroutine(std::function<void()> cb, size_t stack_size = 0, bool use_call = false);

    /**
     * @brief 协程类析构函数
     */
    ~Coroutine();

    //给协程体重置  INIT/TERM状态下才能重置
    /**
     * @brief 协程重置 重新指定执行函数
     * @param[in] cb 新指定的执行函数 
     */
    void reset(std::function<void()> cb);

    /**
     * @brief  init------>子协程
     */
    void swapIn();
    
    /**
     * @brief 子协程------>init
     */
    void swapOut();

    /**
     * @brief 持有调度器协程------>子协程
     */
    void call();

    /**
     * @brief 子协程------>持有调度器协程
     */
    void back();

    /**
     * @brief 获取协程ID
     * @return uint64_t 
     */
    uint64_t getID() const {return m_id;}

    /**
     * @brief 获取协程运行状态
     * @return State 
     */
    State getState() const {return m_state;}

    /**
     * @brief 设置协程状态
     * @param[in] state 
     */
    void setState(State state) {m_state = state;}

public:
    /**
     * @brief 初始化母协程init
     */
    static void Init();

    /**
     * @brief 给当前线程保存正在执行的协程this指针
     * @param[in] c 正在运行的协程this指针
     */
    static void SetThis(Coroutine *c);

    /**
     * @brief 返回当前在执行的协程的this指针
     * @return Coroutine::ptr 
     */
    static Coroutine::ptr GetThis();
    
    /**
     * @brief 当前协程让出执行权，并置为就绪状态READY
     */
    static void YieldToReady();

    /**
     * @brief 当前协程让出执行权，并置为挂起状态HOLD
     * 
     */
    static void YieldToHold();

    /**
     * @brief 获取当前线程上存在的协程总数
     * @return uint64_t 
     */
    static uint64_t TotalCoroutines();
    
    /**
     * @brief 一般协程的回调主函数
     */
    static void MainFunc();

    /**
     * @brief 持有调度器协程的回调主函数
     */
    static void CallMainFunc();

    /**
     * @brief 获取协程ID
     * @return uint64_t 
     */
    static uint64_t GetCoroutineId();

private:
    /**
     * @brief 协程类默认构造函数 负责生成init协程
     */
    Coroutine();

private:
    ///协程ID
    uint64_t m_id = 0;
    ///用户栈大小
    uint32_t m_stack_size = 0;
    ///协程状态
    State m_state = INIT;
    ///协程携带的程序上下文
    ucontext_t m_ctx;
    ///用户栈起始
    void *m_stack = nullptr;
    ///协程执行的回调函数
    std::function<void()> m_cb;

};
}


#endif