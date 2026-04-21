#pragma once

#include "lockqueue.h"
#include <string>

enum LogLevel
{
    INFO,  // 普通信息
    ERROR,  // 错误信息
};

// Mprcp框架提供的日志系统
class Logger
{
public:
    // 获取日志的单例
    static Logger& GetInstance();
    // 设置日志级别
    void setLogLevel(LogLevel level);
    // 写日志
    void Log(std::string msg);
private:
    int m_loglevel;  // 日志级别
    LockQueue<std::string> m_lockqueue;  // 日志缓冲队列

    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
};

// 定义宏
#define LOG_INFO(logmsgformat, ...) \
    do \
    {  \
        Logger &logger = Logger::GetInstance(); \
        logger.setLogLevel(INFO); \
        char log[1024] = {0}; \
        snprintf(log, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(log); \
    } while (0);

#define LOG_ERR(logmsgformat, ...) \
    do \
    {  \
        Logger &logger = Logger::GetInstance(); \
        logger.setLogLevel(ERROR); \
        char log[1024] = {0}; \
        snprintf(log, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(log); \
    } while (0);
    