#ifndef __YLOG_LEVEL_H__
#define __YLOG_LEVEL_H__

// 日志等级类
namespace YLog {

class LogLevel {
public:
    // 定义日志级别枚举
    enum class Value
    {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF
    };

    // 将日志级别转换为字符串
    static constexpr const char *toString(LogLevel::Value v)
    {
        switch (v)
        {
        case LogLevel::Value::DEBUG: return "DEBUG";
        case LogLevel::Value::INFO:  return "INFO";
        case LogLevel::Value::WARN:  return "WARN";
        case LogLevel::Value::ERROR: return "ERROR";
        case LogLevel::Value::FATAL: return "FATAL";
        case LogLevel::Value::OFF:   return "OFF";
        default:                      return "UNKNOWN";
        }
    }
};

}

#endif