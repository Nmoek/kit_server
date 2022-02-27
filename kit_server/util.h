#ifndef _KIT_UTIL_H_
#define _KIT_UTIL_H_

#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <execinfo.h>     //返回函数栈



namespace kit_server
{

/**
 * @brief 获取当前协程ID
 * @return 返回PID值
 */
pid_t GetThreadId();

/**
 * @brief 获取当前协程ID
 * @return 返回ID值
 */
uint64_t GetCoroutineId();

/**
 * @brief 获取当前线程名称
 * @return 返回名称字符串
 */
const std::string& GetThreadName();

/**
 * @brief 获取函数调用栈
 * @param[out] bt 传出已经获取的函数调用栈信息 
 * @param[in] size 期望获取的函数栈层数上限
 * @param[in] skip 选择跳过显示的层数
 */
void BackTrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 打印函数调用栈信息
 * @param[in] size 期望获取的函数栈层数上限
 * @param[in] prefix 打印格式控制
 * @param[in] skip 选择跳过显示的层数
 * @return 返回函数调用栈信息的字符串
 */
std::string BackTraceToString(int size = 64, const std::string &prefix = "", int skip = 2);


/**
 * @brief 获取ms级精度时间
 * @return 返回时间值
 */
uint64_t GetCurrentMs();


/**
 * @brief 获取us级时间
 * @return 返回时间值
 */
uint64_t GetCurrentUs();

/**
 * @brief 将时间戳转换为字符串形式
 * @param ts  时间戳数值
 * @param format 转换为具体的字符串时间格式
 * @return 返回时间字符串
 */
std::string Timer2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * @brief 文件流操作类
 */
class FSUtil
{
public:

    /**
     * @brief 获取当前文件夹路径下所有的文件
     * @param[out] files 输出当前路径下所有文件的名称
     * @param[in] path 当前要操作的文件夹的路径
     * @param[in] subfix 指定的后缀名
     */
    static void ListAllFile(std::vector<std::string>& files, 
        const std::string& path, const std::string& subfix);
    

    /**
     * @brief 创建一个目录
     * @param[in] dirname 目录名称
     * @return 返回是否成功创建了目录
     */
    static bool Mkdir(const std::string& dirname);

    /**
     * @brief 判断程序是否已经启动 
     * @param[in] pidfile 需要进程文件路径/pro/[pid]
     * @return 返回启动与否
     */
    static bool IsRunningPidFile(const std::string& pidfile);

};

}

#endif