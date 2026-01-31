#ifndef _YLOG_LOGMACRO_H__
#define _YLOG_LOGMACRO_H__

#include "logger.hpp"
#include "loggerMgr.hpp"
#include "3rdparty/fmt/core.h"

#include <string>
#include <utility>

namespace YLog {
// ==================== 便捷函数 ====================
inline Logger::ptr getLogger(const std::string &name)
{
    return LoggerMgr::getInstance().getLogger(name);
}

inline Logger::ptr rootLogger()
{
    return LoggerMgr::getInstance().rootLogger();
}

// ==================== Root Logger 快捷函数 ====================
// DEBUG 级别日志
template <typename... Args>
void log_debug(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->debug(fmt, std::forward<Args>(args)...);
    }
}

// INFO 级别日志
template <typename... Args>
void log_info(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->info(fmt, std::forward<Args>(args)...);
    }
}

// WARN 级别日志
template <typename... Args>
void log_warn(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->warn(fmt, std::forward<Args>(args)...);
    }
}

// ERROR 级别日志
template <typename... Args>
void log_error(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->error(fmt, std::forward<Args>(args)...);
    }
}

// FATAL 级别日志
template <typename... Args>
void log_fatal(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->fatal(fmt, std::forward<Args>(args)...);
    }
}

// ==================== 带 Logger 参数的函数 ====================
template <typename... Args>
void log_debug(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->debug(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void log_info(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->info(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void log_warn(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->warn(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void log_error(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->error(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void log_fatal(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->fatal(fmt, std::forward<Args>(args)...);
    }
}

// ==================== 按 Logger 名字的函数 ====================
template <typename... Args>
void log_debug(const std::string &logger_name, fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = getLogger(logger_name);
    log_debug(logger, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_info(const std::string &logger_name, fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = getLogger(logger_name);
    log_info(logger, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_warn(const std::string &logger_name, fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = getLogger(logger_name);
    log_warn(logger, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_error(const std::string &logger_name, fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = getLogger(logger_name);
    log_error(logger, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_fatal(const std::string &logger_name, fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = getLogger(logger_name);
    log_fatal(logger, fmt, std::forward<Args>(args)...);
}

}


#endif 

