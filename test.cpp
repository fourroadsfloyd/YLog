#include "logMacro.hpp"
#include "loggerMgr.hpp"
#include "sink.hpp"
#include "loggerFormat.hpp"
#include <iostream>

using namespace YLog;

int main()
{
    // 创建 GlobalLoggerBuilder（会自动注册到 LoggerMgr）
    GlobalLoggerBuilder builder;

    // 配置 logger
    builder.buildLoggerName("root");
    builder.buildLoggerLevel(LogLevel::Value::DEBUG);
    builder.buildLoggerType(Logger::Type::LOGGER_SYNC);
    builder.buildLoggerFormat(LoggerFormat::FormatType::FORMAT_DETAIL);

    // 添加输出目标：控制台和文件
    builder.buildSink<StdoutSink>();
    builder.buildSink<FileSink>("./logs/test.log");

    // 构建 logger（自动注册到 LoggerMgr）
    auto logger = builder.build();

    std::cout << "========== YLog 测试 ==========\n" << std::endl;

    // 测试各个级别日志 (root logger)
    logd("这是 DEBUG 日志: value = {}", 42);
    logi("这是 INFO 日志: 程序启动成功");
    logw("这是 WARN 日志: 警告 - 配置文件未找到，使用默认配置");
    loge("这是 ERROR 日志: 错误代码 = {}", 404);
    logf("这是 FATAL 日志: 致命错误，程序即将退出");

    std::cout << "\n========== 指定 Logger 测试 ==========\n" << std::endl;

    // 使用指定 logger
    logd(logger, "使用指定 logger 的 DEBUG 日志");
    logi(logger, "使用指定 logger 的 INFO 日志");
    logw(logger, "使用指定 logger 的 WARN 日志");
    loge(logger, "使用指定 logger 的 ERROR 日志");
    logf(logger, "使用指定 logger 的 FATAL 日志");

    std::cout << "\n========== 格式化测试 ==========\n" << std::endl;

    // 测试各种格式化
    logd("整数: {}, 十六进制: {:#x}", 255, 255);
    logi("浮点数: {:.2f}, 百分比: {:.1%}", 3.14159, 0.856);
    logw("字符串: {}, 字符: {}", "hello", 'A');
    loge("指针: 0x{:x}", (void*)&main);

    std::cout << "\n========== 测试完成 ==========\n" << std::endl;

    return 0;
}
