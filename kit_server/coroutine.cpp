#include "coroutine.h"
#include "Log.h"
#include "config.h"
#include "macro.h"
#include "scheduler.h"

#include <atomic>
#include <thread>
#include <stdint.h>


namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");


/**
 * @brief 协程ID累加器
 */
static std::atomic<uint64_t> s_cor_id(0);

/**
 * @brief 当前线程下存在协程的总数
 */
static std::atomic<uint64_t> s_cor_sum(0);

/**
 * @brief 当前线程下正在运行协程
 */
static thread_local Coroutine* cor_this = nullptr;

/**
 * @brief 上一次切出的协程
 */
static thread_local Coroutine::ptr init_cor_sp = nullptr;

/**
 * @brief 配置项 每个协程的栈默认大小为1MB
 */
static ConfigVar<uint32_t>::ptr g_cor_stack_size =
    Config::LookUp("coroutine.stack_size", (uint32_t)1024*1024, "coroutine stack size");

/**
 * @brief 协程栈内存分配器类
 */
class MallocStackAllocator
{
public:
    /**
     * @brief 分配内存
     * @param[in] size 所需内存大小 
     * @return void* 
     */
    static void* Alloc(size_t size)
    {
        return malloc(size);
    }

    /**
     * @brief 释放内存
     * @param[in] vp 栈空间指针 
     * @param[in] size 栈空间大小
     */
    static void Dealloc(void *vp, size_t size)
    {
        free(vp);
    }
};
//使用using起别名
using StackAllocator = MallocStackAllocator;

Coroutine::Coroutine()
{
    m_state = State::EXEC;
    SetThis(this);

    if(getcontext(&m_ctx) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "Cortione: getcontext error";

        KIT_ASSERT2(false, "getcontext error");
    }
    ++s_cor_sum;
}

Coroutine::Coroutine(std::function<void()> cb, size_t stack_size, bool use_call)
    :m_id(++s_cor_id), m_cb(cb)
{
    ++s_cor_sum;

    //为协程分配栈空间 让回调函数在对应栈空间去运行
    m_stack_size = stack_size ? stack_size : g_cor_stack_size->getValue(); 
    m_stack = StackAllocator::Alloc(m_stack_size);
    if(getcontext(&m_ctx) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "Cortione: getcontext error";

        KIT_ASSERT2(false, "getcontext error");
    }

    //指定代码序列执行完毕后 自动跳转到的指定地方
    m_ctx.uc_link = nullptr;        
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stack_size;

    //use_call 标识当前的协程是否是调度协程
    if(!use_call)
        //指定要运行的代码序列
        makecontext(&m_ctx, &Coroutine::MainFunc, 0);
    else 
        makecontext(&m_ctx, &Coroutine::CallMainFunc, 0);


    KIT_LOG_DEBUG(g_logger) << "协程构造:" << m_id;
    
}

Coroutine::~Coroutine()
{
    --s_cor_sum;
    if(m_stack)
    {
        //只要不是运行态 或者 挂起就释放栈空间
        KIT_ASSERT(m_state != State::EXEC || m_state != State::HOLD);

        //释放栈空间
        StackAllocator::Dealloc(m_stack, m_stack_size);
        
        KIT_LOG_DEBUG(g_logger) << "协程析构:" << m_id;
    }
    else  //没有栈是主协程
    {
        KIT_ASSERT(!m_cb);

        KIT_ASSERT(m_state == State::EXEC);
        
        Coroutine* cur = cor_this;
        if(cur == this)
            SetThis(nullptr);
    }

    
}

//协程重置
void Coroutine::reset(std::function<void()> cb)
{
    KIT_ASSERT(m_stack);
    KIT_ASSERT(m_state == State::INIT || m_state == State::TERM || 
               m_state == State::EXCEPTION);

    m_cb = cb;
    if(getcontext(&m_ctx) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "reset: getcontext error";

        KIT_ASSERT2(false, "getcontext error");
    }

    m_cb = cb;

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stack_size;

    makecontext(&m_ctx, &Coroutine::MainFunc, 0);

    m_state = State::INIT;

}

//从调度器 切换到 到目标代码序列
void Coroutine::swapIn()
{
    //将当前的子协程Coroutine * 设置到 cor_this中 表明是这个协程正在运行
    SetThis(this);

    //没在运行态才能 调入运行
    KIT_ASSERT(m_state != State::EXEC);

    m_state = State::EXEC;

    if(swapcontext(&Scheduler::GetMainCor()->m_ctx, &m_ctx) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "swapIn: swapcontext error";

        KIT_ASSERT2(false, "swapcontext error");
    }

}

//从目标代码序列切换到调度器/init协程
void Coroutine::swapOut()
{
    SetThis(Scheduler::GetMainCor());
    if(swapcontext(&m_ctx, &Scheduler::GetMainCor()->m_ctx) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "swapOut: swapcontext error";

        KIT_ASSERT2(false, "swapcontext error");
    }
   
}

//从init协程 切换到 目标代码
void Coroutine::call()
{

    SetThis(this);
    m_state = State::EXEC;
    //应该是把当前创建调度器的那个协程的上下文拿出来运行
    if(swapcontext(&init_cor_sp->m_ctx, &m_ctx) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "call: swapcontext error";

        KIT_ASSERT2(false, "swapcontext error");
    }

}

// 目标代码 切换到 从init协程
void Coroutine::back()
{
    SetThis(init_cor_sp.get());
    if(swapcontext(&m_ctx, &init_cor_sp->m_ctx) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "back: swapcontext error";

        KIT_ASSERT2(false, "swapcontext error");
    }

}


void Coroutine::Init()
{
    //创建母协程init
    Coroutine::ptr main_cor(new Coroutine);

    KIT_ASSERT(cor_this == main_cor.get());

    //这句话很关键
    init_cor_sp = main_cor;
}

void Coroutine::SetThis(Coroutine *c)
{
    cor_this = c;
}

//返回当前在执行的协程
Coroutine::ptr Coroutine::GetThis()
{
    if(cor_this)
    {
        return cor_this->shared_from_this();
    }

    Init();

    return cor_this->shared_from_this();

}

//协程让出 置为READY状态
void Coroutine::YieldToReady()
{
    
    Coroutine::ptr cur = GetThis();
    KIT_ASSERT(cur->m_state == State::EXEC);

    cur->m_state = State::READY;
    cur->swapOut();

}

//协程让出 置为HOLD状态
void Coroutine::YieldToHold()
{
    Coroutine::ptr cur = GetThis();
    KIT_ASSERT(cur->m_state == State::EXEC);
    // cur->m_state = State::HOLD;
    cur->swapOut();

}

//获得当前总协程数
uint64_t Coroutine::TotalCoroutines()
{
    return s_cor_sum;
}


void Coroutine::MainFunc()
{

    Coroutine::ptr cur = GetThis();
   
    KIT_ASSERT(cur);


    try
    {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = State::TERM; //协程已经执行完毕。
    }
    catch(const std::exception &e)
    {
        cur->m_state = State::EXCEPTION;
        KIT_LOG_ERROR(g_logger) << "Coroutine: MainFunc exception:" << e.what()
            << std::endl
            << BackTraceToString();
    }
    catch(...)
    {
        cur->m_state = State::EXCEPTION;
        KIT_LOG_ERROR(g_logger) << "Coroutine: MainFunc exception:" << ",but dont konw reson"
            << std::endl
            << BackTraceToString();
    }


    auto p = cur.get();
    cur.reset();  //让其减少一次该函数调用中应该减少的引用次数
   
    p->swapOut();

    //不会再回到这个地方 回来了说明有问题
    KIT_ASSERT2(false, "never reach here!");
}



//由于是静态函数必须再写一遍代码 比较冗余
void Coroutine::CallMainFunc()
{
    
    Coroutine::ptr cur = GetThis();
   
    KIT_ASSERT(cur);

    try
    {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = State::TERM; //协程已经执行完毕。
    }
    catch(const std::exception &e)
    {
        cur->m_state = State::EXCEPTION;
        KIT_LOG_ERROR(g_logger) << "Coroutine: MainFunc exception:" << e.what()
            << std::endl
            << BackTraceToString();
    }
    catch(...)
    {
        cur->m_state = State::EXCEPTION;
        KIT_LOG_ERROR(g_logger) << "Coroutine: MainFunc exception:" << ",but dont konw reson"
            << std::endl
            << BackTraceToString();
    }

    auto p = cur.get();
    cur.reset();  //让其减少一次该函数调用中应该减少的引用次数

    p->back();

    //不会再回到这个地方 回来了说明有问题
    KIT_ASSERT2(false, "never reach here!");

}


uint64_t Coroutine::GetCoroutineId()
{
    if(cor_this)
    {
        return cor_this->getID();
    } 

    return 0;
}


}