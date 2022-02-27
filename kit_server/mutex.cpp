#include "mutex.h"
#include "thread.h"
#include "Log.h"
#include "util.h"
#include <iostream>


namespace kit_server
{
static Logger::ptr g_logger = KIT_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count)
{
    if(sem_init(&m_semaphore, 0, count) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "Semaphore::Semaphore sem_init fail";

        throw std::logic_error("sem_init fail!!");
    }
}

Semaphore::~Semaphore()
{
    sem_destroy(&m_semaphore);
}

//数+1
void Semaphore::wait()
{
    while(1)
    {
        //递减1 函数成功时返回0;出错返回-1
        if(!sem_wait(&m_semaphore))
            break;

        //如果中断就继续 
        if(errno == EINTR)
            continue;
        
        KIT_LOG_ERROR(g_logger)  << "Semaphore::wait sem_wait error";
        throw std::logic_error("sem_wait error!!");
        
    }
}

//数-1
void Semaphore::notify()
{
    if(sem_post(&m_semaphore) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "Semaphore::notify sem_post error";

        throw std::logic_error("sem_post error!!");
    }

}

}
