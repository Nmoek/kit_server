#ifndef _KIT_LOG_H_
#define _KIT_LOG_H_


#include <string>
#include <memory>
#include <stdint.h>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include <yaml-cpp/yaml.h>
#include "single.h"
#include "util.h"
#include "thread.h"

/*固定日志级别输出日志内容*/
#define KIT_LOG_LEVEL(logger, level)\
    if(logger->getLevel() <= level)\
        kit_server::LogEventWrap(kit_server::LogEvent::ptr(new kit_server::LogEvent(logger, level, __FILE__, __LINE__, 0, kit_server::GetThreadId(), \
        kit_server::GetThreadName(), kit_server::GetCoroutineId(), time(0)))).getSS()


#define KIT_LOG_DEBUG(logger) KIT_LOG_LEVEL(logger, kit_server::LogLevel::DEBUG)
#define KIT_LOG_INFO(logger) KIT_LOG_LEVEL(logger, kit_server::LogLevel::INFO)
#define KIT_LOG_WARN(logger) KIT_LOG_LEVEL(logger, kit_server::LogLevel::WARN)
#define KIT_LOG_ERROR(logger) KIT_LOG_LEVEL(logger, kit_server::LogLevel::ERROR)
#define KIT_LOG_FATAL(logger) KIT_LOG_LEVEL(logger, kit_server::LogLevel::FATAL)

/*带参日志输出日志内容*/
#define KIT_LOG_FMT_LEVEL(logger, level, fmt, ...)\
    if(level >= logger->getLevel())\
        kit_server::LogEventWrap(kit_server::LogEvent::ptr(new kit_server::LogEvent(logger, level, __FILE__, __LINE__, 0, kit_server::GetThreadId(), kit_server::GetThreadName(), kit_server::GetCoroutineId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define KIT_LOG_FMT_DEBUG(logger, fmt, ...) KIT_LOG_FMT_LEVEL(logger, kit_server::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define KIT_LOG_FMT_INFO(logger, fmt, ...) KIT_LOG_FMT_LEVEL(logger, kit_server::LogLevel::INFO, fmt, __VA_ARGS__)
#define KIT_LOG_FMT_WARN(logger, fmt, ...) KIT_LOG_FMT_LEVEL(logger, kit_server::LogLevel::WARN, fmt, __VA_ARGS__)
#define KIT_LOG_FMT_ERROR(logger, fmt, ...) KIT_LOG_FMT_LEVEL(logger, kit_server::LogLevel::ERROR, fmt, __VA_ARGS__)
#define KIT_LOG_FMT_FATAL(logger, fmt, ...) KIT_LOG_FMT_LEVEL(logger, kit_server::LogLevel::FATAL, fmt, __VA_ARGS__)

//通过单例LoggerMgr--->访问到LogManager中默认的Logger实例化对象
#define KIT_LOG_ROOT() kit_server::LoggerMgr::GetInstance()->getRoot()

//通过日志器的名字获取日志器实体
#define KIT_LOG_NAME(name) kit_server::LoggerMgr::GetInstance()->getLogger(name)


namespace kit_server{

class Logger;
class LogManager;

/**
 * @brief 日志级别类
 */
class LogLevel
{
public:
    //日志级别
    enum Level{
        UNKNOW = 0, //未知信息
        DEBUG = 1,  //调试信息
        INFO  = 2,  //一般信息
        WARN  = 3,  //警告信息
        ERROR = 4,  //一般错误
        FATAL = 5   //致命错误
    };

    /**
     * @brief 从枚举类型转为字符串 
     * @param[in] level 日志级别枚举类型 
     * @return const char* 
     */
    static const char* ToString(LogLevel::Level level);

    /**
     * @brief 从字符串转为日志枚举类型
     * @param[in] val 日志级别字符串 
     * @return LogLevel::Level 
     */
    static LogLevel::Level FromString(const std::string& val);
};


/**
 * @brief 日志属性类
 */
class LogEvent
{
public:
    typedef std::shared_ptr<LogEvent> ptr;

    /**
     * @brief 日志属性类构造函数
     * @param[in] logger 具体日志器
     * @param[in] level 具体日志级别
     * @param[in] file 从哪一个文件输出
     * @param[in] m_line 从哪一行输出
     * @param[in] elapse 程序启动后总共时间 单位ms
     * @param[in] thread_id 线程号
     * @param[in] thread_name 线程名称
     * @param[in] coroutine_id 协程号
     * @param[in] time 日志输出时间
     */
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, 
        int32_t m_line, uint32_t elapse, uint32_t thread_id, 
        const std::string& thread_name, uint32_t coroutine_id, uint64_t time);
    
    /**
     * @brief 获取文件名称
     * @return const char* 
     */
    const char* getFileName() const {return m_file;}

    /**
     * @brief 获取行号
     * @return int32_t 
     */
    int32_t getLine() const {return m_line;}

    /**
     * @brief 获取程序启动后总共时间 单位ms
     * @return uint32_t 
     */
    uint32_t getElapse() const {return m_elapse;}

    /**
     * @brief 获取线程号
     * @return uint32_t 
     */
    uint32_t getThreadId() const {return m_threadid;}

    /**
     * @brief 获取线程名称
     * @return const std::string& 
     */
    const std::string& getThreadName() const {return m_thread_name;}

    /**
     * @brief 获取协程号
     * @return uint32_t 
     */
    uint32_t getCoroutineId() const {return m_coroutineid;}

    /**
     * @brief 获取输出时间
     * @return uint64_t 
     */
    uint64_t getTime() const {return m_time;}

    /**
     * @brief 获取日志内容，以字符串返回
     * @return std::string 
     */
    std::string getContent() const {return m_content.str();}

    /**
     * @brief 获取日志内容，以流返回（能加const修饰，需要不断的对"流"内容进行追加和修改）
     * @return std::stringstream& 
     */
    std::stringstream& getSS() {return m_content;} 

    /**
     * @brief 获取日志器
     * @return std::shared_ptr<Logger> 
     */
    std::shared_ptr<Logger> getLogger() const {return m_logger;}

    /**
     * @brief 获取日志级别
     * @return LogLevel::Level 
     */
    LogLevel::Level getLevel() const {return m_level;}
    
    /**
     * @brief 自定义日志模板格式输出
     * @param[in] fmt 传入的具体日志模板 
     * @param[in] ... 可变参数 传参
     */
    void format(const char *fmt, ...);

    /**
     * @brief 自定义日志模板格式输出
     * @param[in] fmt 传入的具体日志模板 
     * @param[in] al 可变参数列表
     */
    void format(const char *fmt, va_list al);

private:
    //文件名
    const char*           m_file = nullptr;
    //行号
    int32_t               m_line = 0;
    //程序启动到现在的毫秒数
    uint32_t              m_elapse = 0;
    //线程ID
    uint32_t              m_threadid = 0;
    //线程名称
    std::string           m_thread_name = "";
    //协程ID
    uint32_t              m_coroutineid = 0;
    //日志时间戳
    uint64_t              m_time;
    //日志内容
    std::stringstream     m_content;
    //所属日志器
    std::shared_ptr<Logger> m_logger;
    //日志级别
    LogLevel::Level         m_level;

};


/**
 * @brief 日志属性包装器类
 */
class LogEventWrap
{
public:
    /**
     * @brief 日志属性包装器类构造函数
     * @param[in] event 日志属性对象智能指针
     */
    LogEventWrap(LogEvent::ptr event);

    /**
     * @brief 日志属性包装器类析构函数
     */
    ~LogEventWrap();

    /**
     * @brief 获取日志属性中的日志内容
     * @return std::stringstream& 
     */
    std::stringstream& getSS();

    /**
     * @brief 获取日志属性智能指针
     * @return std::shared_ptr<LogEvent> 
     */
    std::shared_ptr<LogEvent> getEvent();

private:
    //日志属性对象智能指针
    LogEvent::ptr m_event;
};



/**
 * @brief 日志格式器类
 */
class LogFormatter
{
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    /**
     * @brief 日志格式器类构造函数
     * @param[in] pattern 具体模板格式字符串 
     */
    LogFormatter(const std::string &pattern);

    /**
     * @brief 输出具体格式要求内容
     * @param[in] logger 具体日志器
     * @param[in] level 具体日志级别
     * @param[in] event 具体日志属性
     * @return std::string 
     */
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    /**
     * @brief 获取具体日志输出格式字符串
     * @return const std::string& 
     */
    const std::string& getPattern() const {return m_pattern;}


private:
    /**
     * @brief 日志格式器初始化，解析模板字符串
     */
    void init();

public:
    /**
     * @brief 实现具体格式输出的子类
     */
    class FormatItem
    {
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        /**
         * @brief 具体格式子类构造函数
         * @param[in] fmt 具体模板格式字符串 
         */
        FormatItem(const std::string &fmt = "") {}

        /**
         * @brief 具体格式子类析构函数
         */
        virtual ~FormatItem(){}

        /**
         * @brief 输出具体格式要求内容
         * @param[in] os 标准输出流
         * @param[in] logger 具体日志器
         * @param[in] level 具体日志级别
         * @param[in] event 具体日志属性
         */
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    /**
     * @brief 判断模板字符串是否有错误
     * @return true 有错
     * @return false 没有错
     */
    bool isError() const {return m_error;}

private:
    //日志格式模板 对应模板解析对应内容
    std::string m_pattern;
    //具体格式输出的具体子类队列
    std::vector<FormatItem::ptr> m_items;
    //格式模板是否发生错误
    bool m_error = false;
};


/**
 * @brief 日志输出器类
 */
class LogAppender
{
    friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Mutex MutexType;
    
    /**
     * @brief 日志输出器类析构函数
     */
    virtual ~LogAppender(){}

    /**
     * @brief 日志输出的多态接口
     * @param[in] logger 传入的日志器
     * @param[in] level 日志级别
     * @param[in] event 日志属性
     */
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    /**
     * @brief 日志输出器信息以yaml格式用字符串输出 多态接口
     * @return std::string 
     */
    virtual std::string toYamlString() = 0;

    //void setFormatter(LogFormatter::ptr val){m_formatter = val;}
    /**
     * @brief 设置日志输出器输出格式
     * @param[in] val 具体格式 
     */
    void setFormatter(LogFormatter::ptr val);

    //LogFormatter::ptr getFormatter() const {return m_formatter;}
    /**
     * @brief 获取日志输出器输出格式
     * @return LogFormatter::ptr 
     */
    LogFormatter::ptr getFormatter();

    /**
     * @brief 获取日志输出器的日志级别
     * @return LogLevel::Level 
     */
    LogLevel::Level getLevel() const {return m_level;}

    /**
     * @brief 设置日志输出器的日志级别 
     * @param val 
     */
    void setLevel(LogLevel::Level val){m_level = val; }

    /**
     * @brief 设置是否设置过日志格式
     */
    void setIsFormatter() {is_set_formatter = true;}

    /**
     * @brief 获取是否设置过日志格式
     * @return true 设置过
     * @return false 没有设置过
     */
    bool getIsFormatter() {return is_set_formatter;}
    
protected:
    //输出器日志级别
    LogLevel::Level m_level;
    //日志格式器智能指针
    LogFormatter::ptr m_formatter;
    //互斥锁
    MutexType m_mutex;
    //是否设置过日志格式
    bool is_set_formatter = false;     
};


/**
 * @brief 日志器类
 */
class Logger :public std::enable_shared_from_this<Logger>
{
    //不太推荐这样做 破坏封装性
   // friend class LogManager; 声明该友元是为了在LogManager中使用该类中的成员
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 日志器类构造函数
     * @param[in] name 日志器的名称 默认为"root"
     */
    Logger(const std::string &name = "root");

    /**
     * @brief 输出日志内容
     * @param[in] level 日志级别
     * @param[in] event 日志属性
     */
    void log(LogLevel::Level level, const LogEvent::ptr event);

    /**
     * @brief DUBEG级别输出日志内容
     * @param[in] event 日志属性
     */
    void debug(LogEvent::ptr event);

    /**
     * @brief INFO级别输出日志内容
     * @param[in] event 日志属性
     */
    void info(LogEvent::ptr event);

    /**
     * @brief WARN级别输出日志内容
     * @param[in] event 日志属性
     */
    void warn(LogEvent::ptr event);

    /**
     * @brief ERROR级别输出日志内容
     * @param[in] event 日志属性
     */
    void error(LogEvent::ptr event);

    /**
     * @brief FATAL级别输出日志内容
     * @param[in] event 日志属性
     */
    void fatal(LogEvent::ptr event);

    /**
     * @brief 添加日志输出器
     * @param[in] appender 具体的日志输出器
     */
    void addAppender(LogAppender::ptr appender);

    /**
     * @brief 删除日志输出器
     * @param[in] appender 具体的日志输出器
     */
    void delAppender(LogAppender::ptr appender);

    /**
     * @brief 清空日志输出队列
     */
    void clearAppender();

    /**
     * @brief 设置日志器格式
     * @param[in] val 具体格式字符串
     */
    void setFormatter(const std::string& val);

    //LogFormatter::ptr getFormatter() const {return m_formatter;}
    /**
     * @brief 获取日志器格式
     * @return LogFormatter::ptr 
     */
    LogFormatter::ptr getFormatter(); 

    /**
     * @brief 获取日志器级别
     * @return LogLevel::Level 
     */
    LogLevel::Level getLevel() const {return m_level;}

    /**
     * @brief 设置日志器级别
     * @param[in] val 具体日志级别
     */
    void setLevel(LogLevel::Level val){m_level = val;}

    /**
     * @brief 获取日志的名字
     * @return const std::string& 
     */
    const std::string& getName() const {return m_name;}

    /**
     * @brief 设置默认Root日志器
     * @param[in] p 默认的日志器
     */
    void setDefaultRoot(Logger::ptr p) {default_root = p;} 

    /**
     * @brief 将日志器信息以yaml格式用字符串输出
     * @return std::string 
     */
    std::string toYamlString();

private:
    //日志器名字
    std::string m_name;
    //日志级别
    LogLevel::Level m_level;
    //日志输出器的指针队列
    std::list<LogAppender::ptr> m_appenders;
    //互斥锁
    MutexType m_mutex;
    //Logger自带的一个LogFormatter 防止LogAppender没有LogFormatter
    LogFormatter::ptr m_formatter;
    //改动:默认Logger
    Logger::ptr default_root;   

};


/**
 * @brief 日志管理器类
 */
class LogManager
{
public:
    typedef std::shared_ptr<LogManager> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 日志管理器类构造函数
     */
    LogManager();

    /**
     * @brief 获取日志器
     * @param[in] name 具体日志器名称 
     * @return Logger::ptr 
     */
    Logger::ptr getLogger(const std::string &name);

    /**
     * @brief 获取默认日志器root
     * @return Logger::ptr 
     */
    Logger::ptr getRoot() const {return m_root;} 


    /**
     * @brief 将所有日志器信息以yaml结点返回
     * @return YAML::Node 
     */
    YAML::Node toYamlString();


private:
    //建立名字和日志器的映射关系
    std::map<std::string, Logger::ptr> s_loggers;
    //默认日志器
    Logger::ptr m_root;
    //互斥锁
    MutexType m_mutex;
};
//将日志器管理类置为单例
typedef kit_server::Single<LogManager> LoggerMgr;


/**
 * @brief 输出到控制台的日志输出器类
 */
class StdoutLogAppender: public LogAppender
{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    /**
     * @brief 输出日志内容到控制台显示
     * @param[in] logger 具体日志器 
     * @param[in] level 日志级别
     * @param[in] event 日志属性
     */
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

    /**
     * @brief 将日志器输出器信息以yaml格式用字符串输出
     * @return std::string 
     */
    std::string toYamlString() override;
    //YAML::Node toYamlString() override;

};


/**
 * @brief 输出到文件的日志输出器类
 */
class FileLogAppender: public LogAppender
{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    /**
     * @brief 输出到文件的日志输出器类构造函数
     * @param[in] filename 输出到的具体文件路径 
     */
    FileLogAppender(const std::string& filename);
    
    /**
     * @brief 输出日志内容到文件中
     * @param[in] logger 具体日志器
     * @param[in] level 日志级别
     * @param[in] event 日志属性
     */
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

    /**
     * @brief 将日志器输出器以yaml格式用字符串输出
     * @return std::string 
     */
    std::string toYamlString() override;
    //YAML::Node toYamlString() override;

    /**
     * @brief 文件重打开
     * @return true 文件重打开成功
     * @return false 文件重打开失败
     */
    bool reopen();


private:
    //输出文件路径
    std::string m_filename;
    //输出的文件流
    std::ofstream m_filestream;
    //上一次打开文件的时间
    uint64_t m_last_time = 0;

    // uint64_t count_time = 0;
    struct timeval tv_begin;
    struct timeval tv_cur;

    uint64_t  m_size = 0;

};

}

#endif