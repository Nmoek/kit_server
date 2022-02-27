#include "daemon.h"
#include "util.h"
#include "Log.h"
#include "config.h"

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <wait.h>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

/**
 * @brief 默认配置项 服务器重启时间间隔5s
 */
static ConfigVar<uint32_t>::ptr g_daemon_restart_interval = 
    Config::LookUp("daemon.restart_interval", (uint32_t)5, "daemon server process restart interval");



/**
 * @brief 将进程信息打印输出
 * @return 返回进程信息的字符串 
 */
std::string ProcessInfo::toString() const
{
    std::stringstream ss;
    ss << "\n[ProcessInfo]:\n"
        << "paent_pid= " << parent_pid << "\n"
        << "main_pid= " << main_pid << "\n"
        << "parent_start_time= " << Timer2Str(parent_start_time) << "\n"
        << "main_start_time= " << Timer2Str(main_start_time) << "\n"
        << "restart_count= " << restart_count;

    return ss.str();
}


/**
 * @brief 执行进程中真正运行函数
 * @param[in] argc 传入参数的个数
 * @param[in] argv 传入参数的数值组
 * @param[in] main_cb 真正运行的函数
 * @return 返回程序执行结果
 */
static int real_start(int argc, char* argv[], 
    std::function<int(int argc, char* argv[])> main_cb)
{
    return main_cb(argc, argv);
}


/**
 * @brief 开辟守护进程以及崩溃后重启
 * @param[in] argc 传入参数的个数
 * @param[in] argv 传入参数的数值组
 * @param[in] main_cb 真正运行的函数
 * @return 返回程序执行结果 
 */
static int real_daemon(int argc, char* argv[], 
    std::function<int(int argc, char* argv[])> main_cb)
{
    if(daemon(1, 0) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "daemon error, errno=" << errno 
            << ",is:" << strerror(errno);
        return -1;
    }

    //获取当前进程(父进程)PID
    ProcessInfoMgr::GetInstance()->parent_pid = getpid();
    //获取当前进程(父进程)的启动时间
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);

    while(1)
    {
        pid_t pid = fork();
        if(pid == 0)    //子进程
        {
            ProcessInfoMgr::GetInstance()->main_pid = getpid();
            ProcessInfoMgr::GetInstance()->main_start_time = time(0);
            KIT_LOG_INFO(g_logger) << "process start pid=" << getpid();

            return real_start(argc, argv, main_cb);
        }
        else if(pid > 0)   //当前父进程
        {
            int status = 0;
            waitpid(pid, &status, 0);
            if(status)
            {
                //服务器进程崩溃
                KIT_LOG_ERROR(g_logger) << "child process crash! pid=" << pid << ", status=" << status;
            }
            else
            {   
                //服务器进程正常退出 父进程也随之退出
                KIT_LOG_INFO(g_logger) << "child process finished! pid=" << pid;
                break;
            }

            ProcessInfoMgr::GetInstance()->restart_count += 1;
            //等待资源释放 5s后重启服务器程序
            sleep(g_daemon_restart_interval->getValue());
        }
        else //出错
        {
            KIT_LOG_ERROR(g_logger) << "fork error ret=" << pid << 
                "errno=" << errno << ",is:" << strerror(errno);
            return -1;
        }  
    }

    return 0;
}


int start_daemon(int argc, char* argv[], 
    std::function<int(int argc, char*argv[])> main_cb, bool is_daemon)
{
    if(!is_daemon)
    {
        return real_start(argc, argv, main_cb);
    }

    return real_daemon(argc, argv, main_cb);
    
}



}