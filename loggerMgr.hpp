#ifndef _YLOG_LOGGERMANAGER_H__
#define _YLOG_LOGGERMANAGER_H__

#include "logger.hpp"
#include <mutex>
#include <cassert>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

namespace YLog {

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

    void buildLoggerFormat(LoggerFormat::FormatType format)
    {
        if(format == LoggerFormat::FormatType::FORMAT_NORMAL)
        {
            _format = std::make_shared<NormalFormat>();
        }
        else if(format == LoggerFormat::FormatType::FORMAT_DETAIL)
        {
            _format = std::make_shared<DetailFormat>(_logger_name);
        }
    }

    template <typename SinkType, typename... Args>
    void buildSink(Args &&...args)
    {
        auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
        _sinks.push_back(sink);
    }

    virtual Logger::ptr build() = 0;

protected:
    LoggerFormat::ptr _format;
    Logger::Type _logger_type;
    std::string _logger_name;
    LogLevel::Value _level;
    std::vector<LogSink::ptr> _sinks;
};

class LoggerBuilder : public Builder {
public:
    virtual Logger::ptr build()
    {
        if (_logger_name.empty())
        {
            std::cout << "⽇志器名称不能为空！！";
            abort();
        }
        if (_sinks.empty())
        {
            std::cout << "当前⽇志器：" << _logger_name << " 未检测到落地⽅向，默认为标准输出!\n";
            _sinks.push_back(std::make_shared<StdoutSink>());
        }
        Logger::ptr lp;

        if (_logger_type == Logger::Type::LOGGER_ASYNC)
        {
            lp = std::make_shared<AsyncLogger>(_logger_name, _sinks, _level, _format);
        }
        else
        {
            lp = std::make_shared<SyncLogger>(_logger_name, _sinks, _level, _format);
        }
        return lp;
    }
};

class LoggerMgr {       //管理所有的logger实例
private:
    LoggerMgr()
    {
        std::unique_ptr<LoggerBuilder> slb(new LoggerBuilder());
        slb->buildLoggerName("root");
        slb->buildLoggerType(Logger::Type::LOGGER_ASYNC);
        slb->buildLoggerLevel(LogLevel::Value::DEBUG);
        slb->buildLoggerFormat(LoggerFormat::FormatType::FORMAT_DETAIL);
        slb->buildSink<FileSink>("./logs/test.log");
        _root_logger = slb->build();
        assert(_root_logger && "Failed to initialize root logger");
        _loggers.insert({"root", _root_logger});
    }

    ~LoggerMgr() = default;

    LoggerMgr(const LoggerMgr &) = delete;
    LoggerMgr &operator=(const LoggerMgr &) = delete;

public:
    // 获取loggerManager实例的静态方法
    static LoggerMgr &getInstance()
    {
        static LoggerMgr lm;
        return lm;
    }

    bool hasLogger(const std::string &name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(name);
        if (it == _loggers.end())
        {
            return false;
        }
        return true;
    }

    void addLogger(const std::string &name, const Logger::ptr logger)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_loggers.find(name) != _loggers.end())
        {
            throw std::runtime_error("Logger with the same name already exists");
        }
        _loggers.insert({name, logger});
    }

    Logger::ptr getLogger(const std::string &name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(name);
        if (it == _loggers.end())
        {
            throw std::runtime_error("Logger not found: " + name);
        }
        return it->second;
    }

    Logger::ptr rootLogger()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _root_logger;
    }

private:
    std::mutex _mutex;
    Logger::ptr _root_logger;
    std::unordered_map<std::string, Logger::ptr> _loggers;
};

}

#endif // LOGGERMANAGER_H