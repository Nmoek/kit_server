#include <iostream>

#include "../kit_server/Log.h"
#include "../kit_server/util.h"

using namespace kit_server;
using namespace std;

int main()
{
    //实例化一个日志器 
    Logger::ptr logger(new Logger);

    //控制台输出器加入队列
    logger->addAppender(LogAppender::ptr(new StdoutLogAppender));
    //文件输出器加入队列 默认级别:DEBUG
    logger->addAppender(LogAppender::ptr(new FileLogAppender("./test_log.txt")));

    //文件输出器加入队列 设置级别:ERROR、指定模板
    LogAppender::ptr fmt_file_app(new FileLogAppender("./fmt_test_log.txt"));
    LogFormatter::ptr fmt(new LogFormatter("[%p]%T%d%T%m%T%n"));
    fmt_file_app->setFormatter(fmt);
    fmt_file_app->setLevel(LogLevel::ERROR);

    logger->addAppender(fmt_file_app);

/*
    //实例化一个日志事件
    LogEvent::ptr event(new LogEvent(logger, LogLevel::DEBUG, __FILE__, __LINE__, 0, GetThreadId(), GetCoroutineId(), time(0)));

    //以"流"的方式追加文本内容
    event->getSS() << "hello log";

    //按序打印日志信息
    logger->log(LogLevel::DEBUG, event); */

    /*
    FileLogAppender::ptr file_addpender(new FileLogAppender("./log.txt"));

    LogFormatter::ptr fmt(new LogFormatter("%d%T%p%T%m%n"));

    file_addpender->setFormatter(fmt);
    file_addpender->setLevel(LogLevel::ERROR);

    logger->addAppender(file_addpender);
*/
    //cout << "hello kit_server log" << endl;
/*
    KIT_LOG_DEBUG(logger) << "hello log";
    KIT_LOG_FATAL(logger) << "有致命错误";

    KIT_LOG_FMT_DEBUG(logger, "fmt debug test: %d", 666);
    KIT_LOG_FMT_ERROR(logger, "fmt error test: %s", "成功");
    KIT_LOG_WARN(logger) << "warn log";*/

    auto x = LoggerMgr::GetInstance()->getLogger("xx");
    KIT_LOG_DEBUG(x) << "test logmgr";

    

    return 0;
}


