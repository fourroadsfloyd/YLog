#ifndef __YLOG_LOGGER_FORMAT_H__
#define __YLOG_LOGGER_FORMAT_H__

#include "3rdparty/fmt/core.h"
#include "3rdparty/fmt/format.h"
#include "level.hpp"
#include <memory>
#include <chrono>

namespace YLog {

class LoggerFormat {
public:
    using ptr = std::shared_ptr<LoggerFormat>;

    LoggerFormat(const std::string &name) : _logName(name) {}

    virtual ~LoggerFormat() {}

    virtual fmt::format_string<> formatLog(LogLevel::Value level, const std::string msg) = 0;
    
protected:
    std::string _logName;   //日志器名称
};

class NormalFormat : public LoggerFormat {
public:
    fmt::format_string<> formatLog(LogLevel::Value level, const std::string msg) override
    {
        // 默认格式化： [LEVEL] LoggerName: msg
        return fmt::format("[{}] {}", LogLevel::toString(level), msg);
    }
};

class DetailFormat : public LoggerFormat {
public:
    fmt::format_string<> formatLog(LogLevel::Value level, const std::string msg) override
    {
        // 默认格式化： [LEVEL] LoggerName: msg
        return fmt::format("[{}][{}][{}] {}", std::chrono::system_clock::now(), _logName, LogLevel::toString(level), msg);
    }
};



}

#endif // __YLOG_LOGGER_FORMAT_H__