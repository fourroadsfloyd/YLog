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
void logd(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->debug(fmt, std::forward<Args>(args)...);
    }
}

// INFO 级别日志
template <typename... Args>
void logi(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->info(fmt, std::forward<Args>(args)...);
    }
}

// WARN 级别日志
template <typename... Args>
void logw(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->warn(fmt, std::forward<Args>(args)...);
    }
}

// ERROR 级别日志
template <typename... Args>
void loge(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->error(fmt, std::forward<Args>(args)...);
    }
}

// FATAL 级别日志
template <typename... Args>
void logf(fmt::format_string<Args...> fmt, Args &&...args)
{
    auto logger = rootLogger();
    if (logger) {
        logger->fatal(fmt, std::forward<Args>(args)...);
    }
}

// ==================== 带 Logger 参数的函数 ====================
template <typename... Args>
void logd(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->debug(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void logi(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->info(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void logw(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->warn(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void loge(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->error(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void logf(Logger::ptr logger, fmt::format_string<Args...> fmt, Args &&...args)
{
    if (logger) {
        logger->fatal(fmt, std::forward<Args>(args)...);
    }
}

}


#endif 

