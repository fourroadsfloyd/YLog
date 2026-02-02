#ifndef __YLOG_LOGGER_FORMAT_H__
#define __YLOG_LOGGER_FORMAT_H__

#include "3rdparty/fmt/core.h"
#include "3rdparty/fmt/format.h"
#include "3rdparty/fmt/chrono.h"

#include "level.hpp"
#include <memory>
#include <chrono>
#include <ctime>

namespace YLog {

class LoggerFormat {
public:

    enum FormatType {
        FORMAT_NORMAL = 0,
        FORMAT_DETAIL
    };

    using ptr = std::shared_ptr<LoggerFormat>;

    LoggerFormat() {}

    LoggerFormat(const std::string &name) : _logName(name) {}

    virtual ~LoggerFormat() {}

    virtual std::string formatLog(LogLevel::Value level, const std::string& msg) = 0;
    
protected:
    std::string _logName;   //日志器名称
};

class NormalFormat : public LoggerFormat {
public:
    using ptr = std::shared_ptr<NormalFormat>;

    std::string formatLog(LogLevel::Value level, const std::string& msg) override
    {
        // 默认格式化： [LEVEL] LoggerName: msg
        return fmt::format("[{:<5}] {}\n", LogLevel::toString(level), msg);
    }
};

class DetailFormat : public LoggerFormat {
public:
    using ptr = std::shared_ptr<DetailFormat>;

    DetailFormat(const std::string &name) : LoggerFormat(name) {}

    std::string formatLog(LogLevel::Value level, const std::string& msg) override
    {
        // 默认格式化： [time][logger][LEVEL] msg
        // Only keep second precision (no fractional seconds).
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);

        std::tm tm_local{};
        #ifdef _WIN32
            localtime_s(&tm_local, &tt);
        #else
            localtime_r(&tt, &tm_local);
        #endif

        return fmt::format("[{:%Y/%m/%d %H:%M:%S}][{}][{:<5}] {}\n",
                            tm_local,
                            _logName,
                            LogLevel::toString(level),
                            msg);
    }
};

}

#endif // __YLOG_LOGGER_FORMAT_H__