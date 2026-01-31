#ifndef __YLOG_LOGGER_H__
#define __YLOG_LOGGER_H__

#include "util.hpp"
#include "level.hpp"
#include "looper.hpp"
#include "sink.hpp"
#include "3rdparty/fmt/core.h"

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

        log(LogLevel::Value::DEBUG, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::INFO) == false)
            return;

        log(LogLevel::Value::INFO, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::WARN) == false)
            return;

        log(LogLevel::Value::WARN, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::FATAL) == false)
            return;

        log(LogLevel::Value::FATAL, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::ERROR) == false)
            return;

        log(LogLevel::Value::ERROR, fmt, std::forward<Args>(args)...);
    }

    //========================================================================

    template<typename... Args>
    void debug(const char* file, size_t line, fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::DEBUG) == false)
            return;

        log(LogLevel::Value::DEBUG, file, line, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const char *file, size_t line, fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::INFO) == false)
            return;

        log(LogLevel::Value::INFO, file, line, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const char *file, size_t line, fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::WARN) == false)
            return;

        log(LogLevel::Value::WARN, file, line, fmt,  std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(const char *file, size_t line, fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::FATAL) == false)
            return;

        log(LogLevel::Value::FATAL, file, line, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const char *file, size_t line, fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (shouldLog(LogLevel::Value::ERROR) == false)
            return;

        log(LogLevel::Value::ERROR, file, line, fmt, std::forward<Args>(args)...);
    }

protected:
    bool shouldLog(LogLevel::Value level) { return level >= _level; }

    template <typename... Args>
    void log(LogLevel::Value level, fmt::format_string<Args...> fmt, Args &&...args) 
    {
        // fmt 可以直接格式化多个参数
        auto msg = fmt::format(fmt::format(fmt, std::forward<Args>(args)...));
        LogIt(std::move(msg));
    }

    template <typename... Args>
    void log(LogLevel::Value level, const char *file, size_t line,
                fmt::format_string<Args...> fmt, Args &&...args) 
    {
        // fmt 可以直接格式化多个参数
        auto msg = fmt::format("[{}:{}] {}", file, line, 
                                fmt::format(fmt, std::forward<Args>(args)...));
        LogIt(std::move(msg));
    }

    virtual void LogIt(const std::string &msg) = 0;

protected:
    std::mutex _mutex;
    std::string _name;
    std::atomic<LogLevel::Value> _level;
    std::vector<LogSink::ptr> _sinks;

public:
    class Builder {
    public:
        using ptr = std::shared_ptr<Builder>;
        Builder()
            : _level(LogLevel::Value::DEBUG),
                _logger_type(Logger::Type::LOGGER_SYNC) {}
        void buildLoggerName(const std::string &name)
        {
            _logger_name = name;
        }
        void buildLoggerLevel(LogLevel::Value level)
        {
            _level = level;
        }
        void buildLoggerType(Logger::Type type)
        {
            _logger_type = type;
        }

        template <typename SinkType, typename... Args>
        void buildSink(Args &&...args)
        {
            auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
            _sinks.push_back(sink);
        }

        virtual Logger::ptr build() = 0;

    protected:
        Logger::Type _logger_type;
        std::string _logger_name;
        LogLevel::Value _level;
        std::vector<LogSink::ptr> _sinks;
    };
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