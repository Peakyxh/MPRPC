#include "logger.h"
#include <iostream>

// 获取日志的单例
Logger& Logger::GetInstance(){
    static Logger logger;
    return logger;
}

Logger::Logger() {
    // 启动写日志线程
    std::thread writeLogTask([&](){
        for(;;){
            // 写入日志文件中
            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char file_name[128];
            sprintf(file_name, "%d-%d-%d-log.txt", nowtm->tm_year+1900, nowtm->tm_mon+1, nowtm->tm_mday);

            FILE *pf = fopen(file_name, "a+");
            if(nullptr == pf){
                std::cout << "logger file: " << file_name << " open error!" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string logmsg = m_lockqueue.pop();

            char time_msg[128];
            sprintf(time_msg, "%d:%d:%d => [%s]", 
                nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec, (m_loglevel == INFO ? "info" : "error"));
            logmsg.insert(0, time_msg);
            logmsg.append("\n");

            fputs(logmsg.c_str(), pf);
            fclose(pf);
        }
    });
    // 设置分离线程，守护线程
    writeLogTask.detach();
}

// 设置日志级别
void Logger::setLogLevel(LogLevel level){
    m_loglevel = level;
}

// 写日志，把日志写入lockqueue缓冲队列中
void Logger::Log(std::string msg){
    m_lockqueue.push(msg);
}