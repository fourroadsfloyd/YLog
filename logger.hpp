#ifndef __YLOG_LOGGER_H__
#define __YLOG_LOGGER_H__

#include "3rdparty/fmt/core.h"
#include "util.hpp"
#include "level.hpp"
#include "looper.hpp"
#include "sink.hpp"
#include "loggerFormat.hpp"

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <iostream>
#include <list>
#include <atomic>
#include <unordered_map>
#include <cstdarg>
#include <type_traits>


namespace YLog {

class Logger {
public:
    enum class Type
    {
        LOGGER_SYNC = 0,
        LOGGER_ASYNC
    };
    using ptr = std::shared_ptr<Logger>;

    Logger(const std::string &name,
            std::vector<LogSink::ptr> &sinks,
            LogLevel::Value level = LogLevel::Value::DEBUG)
        : _name(name),
            _level(level),
            _sinks(sinks.begin(), sinks.end()) {}

    std::string loggerName() { return _name; }
    LogLevel::Value loggerLevel() { return _level; }
    
    template<typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::DEBUG) == false)
            return;

        auto msg = _format->formatLog(LogLevel::Value::DEBUG, fmt::format(fmt, std::forward<Args>(args)...));

        LogIt(std::move(msg));
    }

    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::INFO) == false)
            return;

        auto msg = _format->formatLog(LogLevel::Value::INFO, fmt::format(fmt, std::forward<Args>(args)...));

        LogIt(std::move(msg));
    }

    template<typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::WARN) == false)
            return;

        auto msg = _format->formatLog(LogLevel::Value::WARN, fmt::format(fmt, std::forward<Args>(args)...));

        LogIt(std::move(msg));
    }

    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::ERROR) == false)
            return;

        auto msg = _format->formatLog(LogLevel::Value::ERROR, fmt::format(fmt, std::forward<Args>(args)...));

        LogIt(std::move(msg));
    }

    template<typename... Args>
    void fatal(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::FATAL) == false)
            return;

        auto msg = _format->formatLog(LogLevel::Value::FATAL, fmt::format(fmt, std::forward<Args>(args)...));

        LogIt(std::move(msg));
    }

protected:
    bool shouldLog(LogLevel::Value level) { return level >= _level; }

    virtual void LogIt(const std::string &msg) = 0;

protected:
    LoggerFormat::ptr _format;
    std::mutex _mutex;
    std::string _name;
    std::atomic<LogLevel::Value> _level;
    std::vector<LogSink::ptr> _sinks;
    
};

class SyncLogger : public Logger {
public:
    using ptr = std::shared_ptr<SyncLogger>;
    SyncLogger(const std::string &name,
                std::vector<LogSink::ptr> &sinks,
                LogLevel::Value level = LogLevel::Value::DEBUG)
        : Logger(name, sinks, level)
    {
        std::cout << LogLevel::toString(level) << " 同步⽇志器: " << name << "创建成功...\n";
    }

private:
    virtual void LogIt(const std::string &msg)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_sinks.empty())
        {
            return;
        }
        for (auto &it : _sinks)
        {
            it->log(msg.c_str(), msg.size());
        }
    }
};

class AsyncLogger : public Logger {
public:
    using ptr = std::shared_ptr<AsyncLogger>;

    AsyncLogger(const std::string &name,
                std::vector<LogSink::ptr> &sinks,
                LogLevel::Value level = LogLevel::Value::DEBUG)
        : Logger(name, sinks, level),
            _looper(std::make_shared<AsyncWorker>([this](Buffer &msg){ this->realLog(msg); }))
    {
        std::cout << LogLevel::toString(level) << "异步⽇志器: " << name << "创建成功...\n ";
    }

protected:
    virtual void LogIt(const std::string &msg)
    {
        _looper->push(msg);
    }
    
    void realLog(Buffer &msg)
    {
        if (_sinks.empty())
        {
            return;
        }
        for (auto &it : _sinks)
        {
            it->log(msg.begin(), msg.ReadableSize());
        }
    }

protected:
    AsyncWorker::ptr _looper;
};

}

#endif