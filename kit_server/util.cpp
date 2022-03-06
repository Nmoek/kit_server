#include "util.h"
#include "Log.h"
#include "coroutine.h"
#include "thread.h"

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fstream>
#include <signal.h>

namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

pid_t GetThreadId()
{
    //返回内核中tid  不使用pthread_self()是因为其返回的不是真正线程ID
    return syscall(SYS_gettid);
}

const std::string& GetThreadName()
{
    //返回内核中tid  不使用pthread_self()是因为其返回的不是真正线程ID
    return Thread::GetName();
}

uint64_t GetCoroutineId()
{
    return Coroutine::GetCoroutineId();
}

//获取函数堆栈
void BackTrace(std::vector<std::string>& bt, int size, int skip)
{
    void ** arr = (void**)malloc(sizeof(void *) * size);

    //返回当前堆栈使用的真实层数  size是期望层数上限
    size_t true_size = backtrace(arr, size);

    //这里面有malloc的过程 strings是一块动态分配内存
    char **strings = backtrace_symbols(arr, true_size);
    if(strings == nullptr)
    {
        KIT_LOG_ERROR(g_logger) << "BackTrace: backtrace_symbols error";
        return;
    }

    for(size_t i = skip;i < true_size;++i)
    {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(arr);
}


//打印函数堆栈
std::string BackTraceToString(int size, const std::string &prefix, int skip)
{
    std::vector<std::string> bt;
    BackTrace(bt, size, skip);

    //借助流 输出
    std::stringstream ss;
    for(size_t i = 0;i < bt.size();++i)
    {
        ss << prefix << bt[i] << std::endl;
    }

    return ss.str();
}



//获取ms级时间
uint64_t GetCurrentMs()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

//获取us级时间
uint64_t GetCurrentUs()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

std::string Timer2Str(time_t ts, const std::string& format)
{
    struct tm tm;
    tm = *localtime(&ts);
    char buf[100];
    strftime(buf, sizeof(buf), format.c_str(), &tm);

    return buf;
}

/**********************************FSUtil*******************************/

void FSUtil::ListAllFile(std::vector<std::string>& files, 
    const std::string& path, const std::string& subfix)
{
    //探测目录是否存在
    if(access(path.c_str(), F_OK) < 0)
    {
        KIT_LOG_ERROR(g_logger) << "file path don't exist access error, errno=" 
            << errno << ", is:" << strerror(errno);
        return;
    }

    //打开目录流
    DIR* dir = opendir(path.c_str());
    if(!dir)
    {
        KIT_LOG_ERROR(g_logger) << "open dir error, errno=" << errno
            << ",is:" << strerror(errno);
        return;
    }

    //从目录流中读取文件信息
    struct dirent* dp = nullptr;
    errno = 0;
    while((dp = readdir(dir)) != nullptr)
    {
        //如果是一个文件夹类型 需要继续读取 进行递归
        if(dp->d_type == DT_DIR)
        {
            //把当前路径和父目录路径过滤
            if(strcmp(dp->d_name, ".") == 0 ||
                strcmp(dp->d_name, "..") == 0)
                    continue;

            ListAllFile(files, path + "/" + dp->d_name, subfix);
        }
        else if(dp->d_type == DT_REG)   //如果是一个普通文件
        {
            std::string file_name = dp->d_name;
            //如果后缀为空
            if(!subfix.size())
            {
                files.push_back(path + "/" + file_name);
            }
            else
            {
                //文件名比传入后缀还要短 就不是我们要找的文件
                if(file_name.size() < subfix.size())
                    continue;
                
                //截取出后缀 如果为目标文件要收集起来
                if(file_name.substr(file_name.length() - subfix.size()) == subfix)
                {
                    files.push_back(path + "/" + file_name);
                }
                
            }
           
        }
    }

    //关闭目录流
    closedir(dir);

    //判断readir()函数是否出错
    if(errno != 0)
    {
        KIT_LOG_ERROR(g_logger) << "readdir error, errnn=" << errno 
            << ",is:" << strerror(errno);
        return;
    }

}

/**
 * @brief 辅助函数 主要用于探测一下文件是否存在
 * @param[in] file 文件名称
 * @param[out] st 更新当前文件信息
 * @return 返回执行结果
 */
static int __lstat(const char *file, struct stat *st = nullptr)
{
    struct stat new_st;
    int ret = lstat(file, &new_st);
    if(st)
        *st = new_st;

    return ret;
}


/**
 * @brief 创建目录的辅助函数
 * @param[in] dirname 目录名称
 * @return 返回创建目录的结果
 */
static int __mkdir(const char* dirname)
{
    //该目录已经存在 不需要再创建
    if(access(dirname, F_OK) == 0)
        return 0;
    
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool FSUtil::Mkdir(const std::string& dirname)
{
    //目录已经存在
    if(__lstat(dirname.c_str()) == 0)
        return true;
    
    //给目录名称进行深拷贝
    char *path = strdup(dirname.c_str());
    if(!path)
    {
        KIT_LOG_ERROR(g_logger) << "strdup mem is not enough! errno=" << errno << ", is:" << strerror(errno);
        return false;
    }

    char *ptr = strchr(path + 1, '/');
    if(!ptr)
    {
        KIT_LOG_ERROR(g_logger) << "strchr: no find '/' invalid dirname!";
        return false;
    }

    //如果存在多级目录 逐级检查是否存在 不存在就会创建
    for(;ptr; *ptr = '/', ptr = strchr(ptr + 1, '/'))
    {
        //截断到当前层的目录路径
        *ptr = '\0';
        //检查是否存在 不存在就创建 创建失败就退出
        if(__mkdir(path) != 0)
            break;
    }

    //找完了最后的'/' 尝试最后一层目录是否存在
    if(ptr == nullptr)
    {
        if(__mkdir(path) == 0)
        {
            free(path);
            return true;
        }
    }

    free(path);
    return false;
    
}


bool FSUtil::IsRunningPidFile(const std::string& pidfile)
{
    std::ifstream ifs(pidfile);
    std::string line;

    //从文件中获取pid
    if(!ifs || !std::getline(ifs, line))
        return false;

    if(!line.size())
        return false;
    
    //文件中字符串pid转化为PID读取出来
    pid_t pid = atoi(line.c_str());
    if(pid <= 1)
        return false;
    
    if(kill(pid, 0) != 0)
        return false;
    
    return true;
}


}