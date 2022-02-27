#include "Log.h"
#include "config.h"

#include <map>
#include <functional>
#include <iostream>
#include <string.h>
#include <time.h>
#include <algorithm>



namespace kit_server{

const char* LogLevel::ToString(LogLevel::Level level)
{
    /*采用宏替换能减少重复代码*/
    switch(level)
    {
#define XX(name)\
        case LogLevel::name:\
            return #name;\
        break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(FATAL);
        XX(ERROR);
#undef XX   //用完一个宏XX 希望下面的代码不再使用到它
        default:
            return "UNKNOW";
    }

    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& val)
{
#define XX(name)\
    if(strcasecmp(val.c_str(), #name) == 0)\
    {\
        return LogLevel::name;\
    }
        //解决小写不识别
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(FATAL);
        XX(ERROR);

#undef XX

    return LogLevel::UNKNOW;
}



/***************************FormatItem****************************************/

//输出日志消息内容
class MessageFormatItem: public LogFormatter::FormatItem
{
public:
    MessageFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level,  LogEvent::ptr event) override
    {
        os << event->getContent();
    }

};

//输出日志级别
class LevelFormatItem: public LogFormatter::FormatItem
{
public:
    LevelFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << LogLevel::ToString(level);
    }
};

//输出启动后累加时间
class ElapseFormatItem: public LogFormatter::FormatItem
{
public:

    ElapseFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getElapse();
    }
};

//输出日志器名称
/*
class NameFormatItem: public LogFormatter::FormatItem
{
public:
    NameFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
       //os << logger->getName();
       //改动: 打印者Logger 可能是default_root 不能直接如上面使用getName()
       os << event->getLogger()->getName();
    }
};*/


/**
 * @brief 输出日志名
 */
class LogNameFormatItem: public LogFormatter::FormatItem
{
public:
    //该构造函数为了和基类保持一致 无作用
    LogNameFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        //os << logger->getName();
       //改动: 打印者Logger 可能是default_root 不能直接如上面使用getName()
       os << event->getLogger()->getName();
    }
};

/**
 * @brief 输出线程ID
 */
class ThreadIdFormatItem: public LogFormatter::FormatItem
{
public:
    ThreadIdFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getThreadId();
    }
};


/**
 * @brief 输出线程名
 */
class ThreadNameFormatItem: public LogFormatter::FormatItem
{
public:
    ThreadNameFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getThreadName();
    }
};

/**
 * @brief 输出协程ID
 */
class CoroutineIdFormatItem: public LogFormatter::FormatItem
{
public:
    CoroutineIdFormatItem(const std::string& str = ""){}


    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getCoroutineId();
    }
};

/**
 * @brief 输出日期时间
 */
class DateTimeFormatItem: public LogFormatter::FormatItem
{
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format)
    {  
        if(!m_format.size())
            m_format = "%Y-%m-%d %H:%M:%S";

    }


    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        time_t t = (time_t)(event->getTime());
        struct tm tm;
        char s[100];
        tm = *localtime(&t);
        strftime(s, sizeof(s), m_format.c_str(), &tm);
        os << s;
    }

private:
    //时间显示格式
    std::string m_format;
};

/**
 * @brief 输出行号
 */
class LineFormatItem: public LogFormatter::FormatItem
{
public:
    LineFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getLine();
    }
};


/**
 * @brief 输出换行符
 */
class NewLineFormatItem: public LogFormatter::FormatItem
{
public:
    NewLineFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << std::endl;
    }
};

/**
 * @brief 输出文件名
 */
class FileNameFormatItem: public LogFormatter::FormatItem
{
public:
    FileNameFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFileName();
    }
};

/**
 * @brief 输出文本内容
 */
class StringFormatItem: public LogFormatter::FormatItem
{
public:
    StringFormatItem(const std::string& m_s)
        :m_string(m_s){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << m_string;
    }

private:
    //文本内容
    std::string m_string;
};


/**
 * @brief 输出一个Tab键
 */
class TabFormatItem: public LogFormatter::FormatItem
{
public:
    TabFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << "\t";
    }
};



/*
class TestFormatItem: public LogFormatter::FormatItem
{
public:
    TestFormatItem(const std::string& str = ""){}

    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << "测试用";
    }
};*/


/*****************************LogEvent***************************************/
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse, uint32_t thread_id, const std::string& thread_name, uint32_t coroutine_id, uint64_t time)
                :m_file(file), 
                m_line(line), 
                m_elapse(elapse), 
                m_threadid(thread_id),
                m_thread_name(thread_name),
                m_coroutineid(coroutine_id),
                m_time(time),
                m_logger(logger),
                m_level(level)
{

}

//做一个可变参数列表
void LogEvent::format(const char* fmt, ...)
{
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);

}

//传入已经准备好的可变参数 进行一个组合的得到的结果放在buf中
//并且构造一个string对象以"流"的方式送入到stringstream中去作为日志内容
void LogEvent::format(const char* fmt, va_list al)
{
    char *buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    //vasprintf会动态分配内存 失败返回-1
    if(len != -1)
    {
        m_content << std::string(buf, len);
        free(buf);
    }
}


/***************************LogEventWrap**************************************************************/

LogEventWrap::LogEventWrap(LogEvent::ptr event)
    :m_event(event)
{
    
}

LogEventWrap::~LogEventWrap()
{
    //通过日志属性对象中日志器去打印日志内容
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS()
{
    return m_event->getSS();
}

std::shared_ptr<LogEvent> LogEventWrap::getEvent()
{
    return m_event;
}



/***********************************Logger********************************/
Logger::Logger(const std::string &name)
    :m_name(name), m_level(LogLevel::DEBUG)
{
    //在生成Logger时候会生成一个默认的LogFormatter
    m_formatter.reset(new LogFormatter("[%p]%T<%f:%l>%T%d{%Y-%m-%d %H:%M:%S}%T%t(%tn)%T%c%T%g%T%m%n"));


}

//添加日志输出器
void Logger::addAppender(LogAppender::ptr appender)
{
    //加锁
    MutexType::Lock lock(m_mutex);

    //发现加入的日志输出器没有格式器的话 赋予默认格式器
    if(!appender->getFormatter())
    {
        //此处造成死锁了
        //锁 LogAppender中的LogFormatter  
        appender->setFormatter(m_formatter);    
    }

    m_appenders.push_back(appender);
}

//删除日志输出器
void Logger::delAppender(LogAppender::ptr appender)
{
    //锁 m_appenders
    MutexType::Lock lock(m_mutex);
    auto it = find(m_appenders.begin(), m_appenders.end(), appender);
    m_appenders.erase(it);

    //遍历的方式删除
    // for(auto it = m_appenders.begin();it != m_appenders.end();it++)
    // {
    //     if(*it == appender)
    //     {
    //         m_appenders.erase(it);
    //         break;
    //     }
    // }

}

//清空日志输出队列
void Logger::clearAppender()
{
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}


//设置日志格式器
void Logger::setFormatter(const std::string& val)
{
    //解析模板并创建新的日志格式器
    LogFormatter::ptr new_value(new LogFormatter(val));
    //模板解析是否有错误
    if(new_value->isError())
    {
        std::cout << "Logger setFormatter name=" << m_name
                  << "value=" << val << "invaild formatter" << std::endl; 

        return;
    }

    //加锁
    MutexType::Lock lock(m_mutex);
    m_formatter = new_value;
    
    //将所有LogAppender 没有设置过格式的formatter和Logger同步
    for(auto &x : m_appenders)
    {
        if(!x->getIsFormatter())
            x->setFormatter(new_value);
    }
}

//获取日志格式器
LogFormatter::ptr Logger::getFormatter()
{
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

std::string Logger::toYamlString()
{
    //锁 m_formatter
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    node["level"] = LogLevel::ToString(m_level);
    node["formatter"] = m_formatter->getPattern();

    for(auto &x : m_appenders)
    {
        node["appender"].push_back(YAML::Load(x->toYamlString()));
    }
    //如果日志输出器队列为空 也需要标识
    if(!node["appender"].size())
        node["appender"] = "NULL";

    std::stringstream ss;
    ss << node;

    return ss.str();
}



//打印日志 最终有LogAppender下的子类中的log去完成
void Logger::log(LogLevel::Level level, const LogEvent::ptr event)
{
    //如果传入的日志级别 >= 当前日志器的级别都能进行输出
    if(level >= m_level)
    {
        //拿到this指针的智能指针
        auto self = shared_from_this();

        if(m_appenders.size())
        {
            //操作日志输出器的时候要独占 锁住
            MutexType::Lock lock(m_mutex);
            for(auto &x : m_appenders)
            {
                //调用的是appender中的log()进行实际输出
                //有点类似观察者模式
                x->log(self, level, event);
            }
        }
        else if(default_root)  //如果当前的Logger没有分配LogAppender就使用default_root中的LogAppender来打印
        {
            //是一层递归  使用default_root中的日志输出器来打印
            default_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}

/**********************************LogAppender***************************/
#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000) 

#include <sys/time.h>

//多线程环境下 可能别的线程在读 而当前要写
void LogAppender::setFormatter(LogFormatter::ptr val)
{
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    //传入的日志格式器不为nullptr 该输出器被设置过格式
    if(m_formatter)
        is_set_formatter = true;
    else 
        is_set_formatter = false;
}

//多线程环境下 可能别的线程在写 而当前要读
LogFormatter::ptr LogAppender::getFormatter()
{
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}


FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename)
{
    //count_time = time(0);
    //初始化时间结构体
    memset(&tv_cur, 0, sizeof(struct timeval));
    //gettimeofday(&tv_cur, nullptr);
    //将当前文件重打开一次
    reopen();
}

//文件重打开
bool FileLogAppender::reopen()
{
    //锁 文件句柄
    MutexType::Lock lock(m_mutex);
    if(m_filestream)
    {
        m_filestream.close();
    }

    //以追加方式打开文件
    m_filestream.open(m_filename, std::ios::app);
    return !m_filestream;

}



//文件存储日志信息
void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
              
        uint64_t now = time(0);

        if(now != m_last_time)  //这个!=不等于意味着 每过1s就reopen一次
        {
            reopen();
            m_last_time = now;
            
        }

        MutexType::Lock lock(m_mutex);
        // gettimeofday(&tv_begin, nullptr);
        std::string t = m_formatter->format(logger, level, event);
        m_filestream << t;

        // m_size += t.size();

        //memcpy(&tv_cur, &tv_begin, sizeof(struct timeval));
        //gettimeofday(&tv_begin, nullptr);
        // unsigned long int m_time = TIME_SUB_MS(tv_begin, tv_cur);
        // if(m_time >= 1000) //1s打印一次写入量
        // {
        //     //打印耗时
        //     std::cout <<"out file size: "<< (m_size / 1024.0) / 1024.0 << "MB" << ", time=" << m_time << "ms" << ",t_id=" << GetThreadId() <<std::endl;
        //     count_time = now;

        //     memcpy(&tv_cur, &tv_begin, sizeof(struct timeval));
        //     m_size = 0;
        // }
        
        //每秒重新打开文件一次 意味着被强删 会重新生成新文件，之前的内容会丢失

        /*
        if(m_filestream.is_open())  
        {
            //锁m_m_filestream
            MutexType::Lock lock(m_mutex);
            std::string t = m_formatter->format(logger, level, event);
            m_filestream << t;

            //std::cout << t.size() / 1024 << "MB" << std::endl;
        
            return;

        }*/
        
        //uint64_t now = time(0);
        //reopen();
        //m_last_time = now;

    }

}

std::string FileLogAppender::toYamlString()
{
    //锁m_formatter
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    node["level"] = LogLevel::ToString(m_level);
    if(m_formatter && getIsFormatter())
        node["formatter"] = m_formatter->getPattern();

    std::stringstream ss;
    ss << node;
    return ss.str();
}

//控制台打印日志信息
void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        //std::cout 不是线程安全的 
        //锁m_formatter
        MutexType::Lock lock(m_mutex);
        std::cout << m_formatter->format(logger, level, event);
    }
    
}

//打印来自.yaml中appender信息
std::string StdoutLogAppender::toYamlString()
{
    //锁m_formatter
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["level"] = LogLevel::ToString(m_level);
    //如果输出器单独设置了格式 也要显示
    if(m_formatter && getIsFormatter())
        node["formatter"] = m_formatter->getPattern();

    std::stringstream ss;
    ss << node;
    return ss.str();
}


/**********************LogFormatter**********************/
LogFormatter::LogFormatter(const std::string &pattern)
    :m_pattern(pattern)
{
    //解析传入模板格式 
    init();
}

//LogFormatter中的format  最终调用具体FormatItem中的format进行打印
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    std::stringstream ss;
    for(auto &x : m_items)
    {
        x->format(ss, logger, level, event);
    }

    return ss.str();
} 



//解析模板 套用模板
void LogFormatter::init()
{
    // 1.模板字符串 2.普通字符串 3.是否为正确有效模板
    std::vector<std::tuple<std::string, std::string, int> > mv;
    //缓存和模板内容无关的普通字符串 一并加入到mv中
    std::string str = ""; 
 
    for(size_t i = 0;i < m_pattern.size();i++)
    {
        //遇到"%"前的普通字符存入到str中并且跳到下一个字符
        if(m_pattern[i] != '%')
        {
            str += m_pattern[i];
            continue;
        }


        //连续遇到两个"%%" 进行转义处理
        if(i + 1 < m_pattern.size() && m_pattern[i + 1] == '%')
        {
            str += '%';
            continue;
        }

        std::string pcahr = ""; //暂时存储相关模板代表字母
        std::string fmt = "";  //存储"{...}"间的模板内容
        size_t index = i + 1;  // '%'后的第一个位置
        int fmt_status = 0;    //有限状态机表示
        size_t fmt_begin = 0;  // "{...}"间内容的起点

        while(index < m_pattern.size())
        {
            // 碰到符号H:非字符 && 非'{' && 非'}' 后将"%XXXH"紧跟串XXX存入temp中，并且break
            if(fmt_status == 0 && (!isalpha(m_pattern[index]) && m_pattern[index] != '{' && m_pattern[index] != '}'))
            {
                pcahr = m_pattern.substr(i + 1, index - i - 1);
                break;
            }

            //fmt_status表示一种有限状态机,处理括号中的内容 
            //0 = 未遇到 '{'  
            //1 = 已经遇到'{' , 未遇到'}'
            if(fmt_status == 0)
            {
                //%XXX{   存储XXX串
                if(m_pattern[index] == '{')
                {
                    pcahr = m_pattern.substr(i + 1, index - i - 1);
                    fmt_status = 1;
                    fmt_begin = index;
                    ++index;
                    continue;
                }

            }
            else if(fmt_status == 1)
            {
                // {XXXX} 存储XXX串
                if(m_pattern[index] == '}')
                {
                    fmt = m_pattern.substr(fmt_begin + 1, index - fmt_begin - 1);
                    fmt_status = 0;
                    ++index;
                    break;
                }
            }

            ++index;

            // "%XXXXXX"  存储XXXXXX串 
            if(index == m_pattern.size())
            {
                if(!pcahr.size())
                {
                    pcahr = m_pattern.substr(i + 1);
                }
            }
        }


        //没有遇到 { 或者 完成了括号序列的扫描
        if(fmt_status == 0)
        {
            //%aaaaXXX%bbbb  或者  XXX%aaaa  存储XXX
            if(pcahr.size())
            {
                mv.push_back(std::make_tuple(str, std::string(), 0));
                str.clear();
            }

            //%XXX 存储XXX 且 {YYY} 存储YYY
            mv.push_back(std::make_tuple(pcahr, fmt, 1));
            i = index - 1;
        } 
        //有 { 但没有遇到 } 一定是一个错误的序列
        else if(fmt_status == 1)  
        {
            std::cout << "pattern parse error" << m_pattern << "--" << m_pattern.substr(i) << std::endl;

            m_error = true;

            mv.push_back(std::make_tuple("<<pattern error>>", fmt, 0));
 
        }


    }

    // "%aaaXXX"存储XXX 即最后尾部为普通字符串的情况
    if(str.size())
    {
        mv.push_back(std::make_tuple(str, "", 0));
    }

    // %n-------换行符
    // %m-------日志内容
    // %p-------level
    // %r-------程序启动到现在的耗时
    // %%-------输出一个%
    // %t-------当前线程ID
    // %T-------Tab键
    // %tn------当前线程名称
    // %d-------日期和时间
    // %f-------文件名
    // %l-------行号
    // %g-------日志器名字
    // %c-------当前协程ID

    //将字符 和 对应的函数操作建立映射关系
    //可以使用函数指针完成，这里使用了函数包装器function + lambda表达式，非常方便
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
 #define XX(str, C)\
        {#str, [](const std::string& fmt){ return FormatItem::ptr(new C(fmt));}}

        /*注意必须是 ',' */
        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(tn, ThreadNameFormatItem),
        XX(d, DateTimeFormatItem),
        XX(l, LineFormatItem),
        XX(n, NewLineFormatItem),
        XX(f, FileNameFormatItem),
        XX(T, TabFormatItem),
        XX(c, CoroutineIdFormatItem),
        XX(g, LogNameFormatItem)
        //XX(test, TestFormatItem),
#undef XX  
    };

    for(auto &x : mv)
    {
        //元组中标志int = 0 说明不是模板内容 构造为普通字符串对象
        if(std::get<2>(x) == 0)
        {
            //构造输出文本内容的子类对象
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(x))));    
        }
        else 
        {
            auto it = s_format_items.find(std::get<0>(x));
            //如果mv中 存储的<0>位置的字符串 无法与map中的映射关系对应,说明存储了一个错误的字符模板
            if(it == s_format_items.end())
            {
                //构造输出文本内容的子类对象 输出一下错误
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(x) + ">>")));

                m_error = true; 
            }
            else
            {
                //给对应的子类构造传参<1>位置的fmt字符串
                m_items.push_back(it->second(std::get<1>(x)));
            }
        }

        /*
        std::cout << std::get<0>(x) << "--" << std::get<1>(x) << "--" << std::get<2>(x) << std::endl;*/
    }
}

/*********************************LogManager**************************************************/

LogManager::LogManager()
{
    //生成一个默认的Logger 并且配置一个默认的LogAppender 输出到控制台
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    //存入到容器中
    s_loggers[m_root->getName()] = m_root;

}

Logger::ptr LogManager::getLogger(const std::string &name)
{
    /*  原来的做法
    auto it = s_loggers.find(name);

    //注意：当指定一个name之后找不到对应的Logger  返回的是默认的root
    return it == s_loggers.end() ? m_root : it->second;*/
    
    //改动:
    //锁 map容器s_loggers
    MutexType::Lock lock(m_mutex);

    auto it = s_loggers.find(name);
    if(it != s_loggers.end())
        return it->second;
    
    Logger::ptr logger(new Logger(name));
    s_loggers[name] = logger;
    //给新创建的日志器Logger一个
    logger->setDefaultRoot(m_root);

    return logger;

}


//打印当前日志配置信息
YAML::Node LogManager::toYamlString()
{
    //锁 map容器s_loggers
    MutexType::Lock lock(m_mutex);
    YAML::Node node;

    //将管理的所有日志器的信息打包到yaml结点中
    for(auto &x : s_loggers)
    {
        node.push_back(YAML::Load(x.second->toYamlString()));
    }

    return node;

}



/*****************************************日志配置**************************************/

struct LogAppenderDefine
{
    //0：StdOut  1:File
    int type = -1;
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& d) const 
    {
        return type == d.type && level == d.level && 
                formatter == d.formatter && file == d.file; 
    }
};


struct LogDefine
{
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& d) const
    {
        return name == d.name && level == d.level && 
                formatter == d.formatter && appenders == d.appenders;
    }

    bool operator<(const LogDefine& d) const 
    {
        return name < d.name;
    }
};

//自定义类型偏特化  string--------->LogAppenderDefine 序列化
template<>
class LexicalCast<std::string, LogAppenderDefine>
{
public:

    LogAppenderDefine operator()(const std::string &val)
    {
        LogAppenderDefine p;
        YAML::Node node = YAML::Load(val);

        //std::cout << node << std::endl << std::endl;

        if(!node["type"].IsDefined())
        {
            std::cout << "log config error: LogAppender name is NULL!!" << std::endl;

            throw std::logic_error("LogAppender name is NULL");
        }
        else
        {
            
            p.name = node["type"].as<std::string>();
            if(p.name == "FileLogAppender")
            {
                p.type = 1;

                if(!node["file"].IsDefined())
                {
                    std::cout << "log config error: LogAppender file is NULL!!" << std::endl;

                    throw std::logic_error("LogAppender file is NULL");
                }
                else
                    p.file = node["file"].as<std::string>();

            }
            else if(p.name == "StdoutLogAppender")
                p.type = 0;
            else 
            {
                p.type = -1;
                p.name = "";
                std::cout << "log config error: LogAppender type is invaild!!" << std::endl;

                throw std::logic_error("LogAppender type is invaild");
            }
        }
        
 
        p.level = LogLevel::FromString(node["level"].IsDefined() ?  node["level"].as<std::string>() : "");

        p.formatter = node["formatter"].IsDefined() ? node["formatter"].as<std::string>() : "";



        return p;
    }


};


//自定义类型偏特化  LogAppenderDefine--------->string 反序列化
template<>
class LexicalCast<LogAppenderDefine, std::string>
{
public:
    std::string operator()(const LogAppenderDefine& p)
    {
        YAML::Node node;
        std::string str;
        switch(p.type)
        {
            case 0: str = "FileLogAppender";break;
            case 1: str = "StdoutLogAppender";break;
        }

        node["type"] = str;
        node["level"] =  LogLevel::ToString(p.level);
        node["formatter"] = p.formatter;
        node["file"] = p.file;

        std::stringstream ss;
        ss << node;

        return ss.str();
    }

};



//自定义类型偏特化  string--------->LogDefine 序列化
template< >
class LexicalCast<std::string, LogDefine>
{
public:

    LogDefine operator()(const std::string &val)
    {
        LogDefine d;
        YAML::Node node = YAML::Load(val);

        if(!node["name"].IsDefined())
        {
            std::cout << "log config error: Logger name is NULL!!" << std::endl;

            throw std::logic_error("Logger name is NULL");

        }
        else
            d.name = node["name"].as<std::string>();

        d.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");

        d.formatter = node["formatter"].IsDefined() ? node["formatter"].as<std::string>() : "";

        if(node["appender"].IsDefined())
        {
            for(size_t i = 0;i < node["appender"].size();++i)
            {
                std::stringstream ss;
                ss << node["appender"][i];
                d.appenders.push_back(LexicalCast<std::string, LogAppenderDefine>()(ss.str()));
            }
        }

        return d;
    }


};


//自定义类型偏特化  LogDefine--------->string 反序列化
template<>
class LexicalCast<LogDefine, std::string>
{
public:
    std::string operator()(const LogDefine& p)
    {
        YAML::Node node;

        node["name"] = p.name;
        node["level"] = LogLevel::ToString(p.level);
        node["formatter"] = p.formatter;

        for(size_t i = 0;i < p.appenders.size();++i)
        {
            node["appender"].push_back(YAML::Load(LexicalCast<LogAppenderDefine, std::string>()(p.appenders[i])));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }

};




kit_server::ConfigVar<std::set<LogDefine> >::ptr g_log_defines = 
    kit_server::Config::LookUp("logs", std::set<LogDefine>(), "logs configs");


//冷知识:全局变量在main函数之前构建
struct LogIniter
{
    LogIniter()
    {
        //设置好 key值="logs"的配置项的触发事件，一旦从.yaml文件读到"logs"相关配置就会进入到该lambda函数中来
        g_log_defines->addListener([](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value){
            /*  1.有新增日志
            *   2.有旧日志修改
            *   3.有旧日志删除*/
            KIT_LOG_INFO(KIT_LOG_ROOT()) << "logger configs changed!";
            for(auto &x : new_value)
            {
                auto it = old_value.find(x);
                Logger::ptr logger;
                if(it == old_value.end())  //新增日志器
                {
                    //新创建一个日志器并以.yaml中读取的名字命名
                    //logger.reset(new kit_server::Logger(x.name));
                    logger = KIT_LOG_NAME(x.name);
                    //std::cout << "正在新增:" << logger->getName() << std::endl;

                }
                else  //修改日志器  老的里面存在  新的里面也有
                {     
                    //判断一下内容是否发生变化
                    if(!(x == *it))  //有变化
                    {
                        logger = KIT_LOG_NAME(x.name);                     
                    }
                    else        //无变化
                        continue;
               
                    //std::cout << "正在修改:" << logger->getName() << std::endl;
                }
                
                //设定日志级别
                logger->setLevel(x.level);
                //std::cout << logger->getName() << " 修改级别" << LogLevel::ToString(logger->getLevel()) << std::endl;

                //设定日志格式
                if(x.formatter.size())
                {
                    logger->setFormatter(x.formatter);
                }
                //std::cout << logger->getName() << " 修改格式" << logger->getFormatter()->getPattern() << std::endl;

                //清空原有的日志输出器 重新根据.yaml文件设置输出器
                logger->clearAppender();
                for(auto &k : x.appenders)
                {
                    LogAppender::ptr p;
                    if(k.type == 0) //输出到控制台
                    {
                        p.reset(new StdoutLogAppender);
                    }
                    else if(k.type == 1) //输出到文件
                    {
                        p.reset(new FileLogAppender(k.file));

                    }

                    //设置输出器级别
                    p->setLevel(k.level);

                    //设置日志格式
                    if(k.formatter.size())
                    {
                        //只有在文件中手动设置过的才展示
                        //从Logger继承过来的不展示
                        LogFormatter::ptr f(new LogFormatter(k.formatter));
                        //如果配置的formatter不合法也不能进行初始化 那么就会继续使用Logger的formatter
                        if(!f->isError())
                        {
                            //p->setIsFormatter();
                            p->setFormatter(f);
                        }
                        else
                            std::cout << "[ERROR]: " << "yaml Logger name= " << logger->getName() << " include LogAppender name= " << k.name << " formatter= " << k.formatter << " is invaild" << std::endl;
                    }

                    //添加到日志器的输出队列中 会将Logger的formatter也默认给LogAppender
                    logger->addAppender(p);
                }
            }

            //老的里面有  新的里面没有
            //删除不是真的将空间释放，打一个标记或者使用别的方法让其不再写日志
            //不真正删除的理由:防止添加回来又要重新创建，开销大
            for(auto &x : old_value)
            {
                auto it = new_value.find(x);
                if(it == new_value.end())
                {
                    auto t_logger = KIT_LOG_NAME(x.name);
                    t_logger->setLevel((LogLevel::Level)100);
                    t_logger->clearAppender();
                }
            }

        });
    }


};

static LogIniter __log_init;



}
