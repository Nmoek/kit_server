#ifndef _KIT_APPLICATION_H_
#define _KIT_APPLICATION_H_

#include "http/http_server.h"

#include <map>
#include <vector>


namespace kit_server
{

class Application
{
public:
    Application();
    static Application* GetInstance() {return s_instance;}

    /**
     * @brief 初始化程序
     * @param[in] argc 传入参数个数
     * @param[in] argv 传入参数数值组
     * @return 返回是否初始化成功
     */
    bool init(int argc, char* argv[]);

    /**
     * @brief 启动程序 可以选择是否是以守护进程的方式启动
     * @return 返回是否启动成功 
     */
    bool run();

    /**
     * @brief 获取指定服务器的智能指针
     * @param[in] type 指定要获取的服务器类型
     * @param[out] servers 输出对应的服务器指针
     * @return 返回获取服务器是否成功  
     */
    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& servers);

    /**
     * @brief 获取所有的服务器
     * @param[out] servers 输出所有服务器指针
     */
    void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);

private:
    /**
     * @brief 真正执行的main函数
     * @param[in] argc 传入参数个数
     * @param[in] argv 传入参数数值组
     * @return
     */
    int main(int argc, char* argv[]);

    /**
     * @brief 负责执行函数的协程以供调度
     * @return 
     */
    int run_coroutine();

private:
    //传入参数个数
    int m_argc = 0;
    //传入参数数值组
    char **m_argv = nullptr;
    //记录不同类型的服务器
    std::map<std::string, std::vector<TcpServer::ptr> > m_servers;
    //程序运行的调度器
    IOManager::ptr m_mainIOManager;
    //当前类指针
    static Application* s_instance;

};

}


#endif