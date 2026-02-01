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

class LoggerBuilder : public Logger::Builder {
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
            lp = std::make_shared<AsyncLogger>(_logger_name, _sinks, _level);
        }
        else
        {
            lp = std::make_shared<SyncLogger>(_logger_name, _sinks, _level);
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
        slb->buildLoggerFormat(LoggerFormat::FormatType::FORMAT_NORMAL);
        slb->buildSink<StdoutSink>();
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