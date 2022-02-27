#ifndef _KIT_DAEMON_H_
#define _KIT_DAEMON_H_

#include <functional>
#include <unistd.h>
#include <string>

#include "single.h"

namespace kit_server
{

/**
 * @brief 服务器进程信息结构体
 */
struct ProcessInfo
{
    pid_t           parent_pid;             //父进程PID
    pid_t           main_pid;               //主进程PID
    uint64_t        parent_start_time = 0;  //父进程开启时间
    uint64_t        main_start_time = 0;    //主进程开启时间
    uint32_t        restart_count = 0;      //服务器重启次数
    
    std::string toString() const;
};
//置为单例
typedef Single<ProcessInfo> ProcessInfoMgr;


/**
 * @brief 以守护进程的方式启动程序
 * @param[in] argc 传入参数的个数
 * @param[in] argv 传入参数的数值组
 * @param[in] main_cb 真正运行的函数
 * @param[in] is_daemon 是否以守护进程的方式启动
 * @return 返回程序执行结果
 */
int start_daemon(int argc, char* argv[], 
    std::function<int(int argc, char* argv[])> main_cb, bool is_daemon);


}



#endif 
