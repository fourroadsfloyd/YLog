#include "logMacro.hpp"
#include "loggerMgr.hpp"
#include "sink.hpp"
#include "loggerFormat.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

using namespace YLog;

int main()
{
    // ==================== 多线程异步日志测试 ====================
    // 目标：验证 AsyncLogger 在多线程并发写日志时不会崩溃、日志不丢失、每条日志一行。
    {
        LoggerBuilder async_builder;
        async_builder.buildLoggerName("async_mt");
        async_builder.buildLoggerLevel(LogLevel::Value::DEBUG);
        async_builder.buildLoggerType(Logger::Type::LOGGER_ASYNC);
        async_builder.buildLoggerFormat(LoggerFormat::FormatType::FORMAT_DETAIL);
        async_builder.buildSink<FileSink>("./logs/async_mt.log");
        auto async_logger = async_builder.build();

        constexpr int kThreads = 8;
        constexpr int kPerThread = 2000;

        std::vector<std::thread> threads;
        threads.reserve(kThreads);

        for (int t = 0; t < kThreads; ++t)
        {
            threads.emplace_back([async_logger, t]() {
                for (int i = 0; i < kPerThread; ++i)
                {
                    logi(async_logger, "tid={}, i={}, msg={}", t, i, "hello");
                }
            });
        }

        for (auto &th : threads)
            th.join();

        // AsyncWorker runs in a background thread; give it a brief moment to drain.
        // (In a real library you'd expose flush()/stop() on the logger.)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 创建 LoggerBuilder（会自动注册到 LoggerMgr）
    LoggerBuilder builder;

    // 配置 logger
    builder.buildLoggerName("root");
    builder.buildLoggerLevel(LogLevel::Value::DEBUG);
    builder.buildLoggerType(Logger::Type::LOGGER_SYNC);
    builder.buildLoggerFormat(LoggerFormat::FormatType::FORMAT_DETAIL);

    // 添加输出目标：控制台和文件
    builder.buildSink<RollSink>("./logs/test.log", 1024 * 1024 * 10); // 10 MB
    //builder.buildSink<FileSink>("./logs/test.log");

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
    logi("浮点数: {:.2f}, 百分比: {:.1f}%", 3.14159, 0.856 * 100.0);
    logw("字符串: {}, 字符: {}", "hello", 'A');
    loge("指针: {:p}", (const void*)&main);

    std::cout << "\n========== 测试完成 ==========\n" << std::endl;

    return 0;
}
