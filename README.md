# YLog

一个轻量级、头文件为主（header-only-ish）的 C++ 日志库示例工程，目标是：

- **同步/异步**两种日志器（`SyncLogger` / `AsyncLogger`）
- 多种 **Sink（落地方式）**：控制台、固定文件、按大小滚动、按天滚动
- 基于 **{fmt}** 的类型安全格式化与 `fmt::format_string<>` 编译期检查
- 简洁的宏/函数式 API（`logd/logi/logw/loge/logf`）

> 备注：本仓库把 fmt 源码 vendor 在 `3rdparty/`，通过 CMake 编译生成静态库并链接（避免依赖预编译 `libfmt.a` 带来的编译器/ABI 不匹配问题）。

---

## 目录结构速览

- `logger.hpp`：日志器基类 `Logger` + `SyncLogger`/`AsyncLogger`
- `loggerMgr.hpp`：`LoggerMgr`（单例）+ `LoggerBuilder`
- `loggerFormat.hpp`：格式化策略（`NormalFormat`/`DetailFormat`）
- `sink.hpp`：各种 Sink（`StdoutSink`/`FileSink`/`RollSink`/`DailyRollSink`）
- `looper.hpp` + `buffer.hpp`：异步后台线程 `AsyncWorker` 与缓冲区 `Buffer`
- `logMacro.hpp`：`logd/logi/...` 快捷函数（root logger 或指定 logger）
- `test.cpp`：示例与压测（多线程异步写文件）
- `3rdparty/`：fmt 源码（仅使用 `format.cc`、`os.cc`，不启用 module 版本 `fmt.cc`）

---

## 技术路线（核心设计与数据流）

### 1) 格式化（Formatter）

统一由 `LoggerFormat` 抽象：

```cpp
class LoggerFormat {
public:
  virtual std::string formatLog(LogLevel::Value level, const std::string& msg) = 0;
};
```

工程内置两种实现：

- `NormalFormat`：
  - 输出形如：`[INFO ] message\n`
  - 其中 `[{:<5}]` 固定宽度，让 `INFO/WARN` 对齐：

    ```text
    [DEBUG] xxx
    [INFO ] xxx
    [WARN ] xxx
    [ERROR] xxx
    [FATAL] xxx
    ```

- `DetailFormat`：
  - 输出形如：`[2026/02/02 15:53:23][root][INFO ] message\n`
  - 时间只保留到秒（不带小数）

> 为什么换行放在 formatter？
> - 这是“消息边界”的定义点：一条日志就是一行。
> - Sink 层只负责按字节写入，不再关心拼接规则。

---

### 2) 日志器（Logger）

`Logger` 负责：

1. 判断是否需要输出（level 过滤）
2. 用 fmt 格式化用户消息（`fmt::format(fmt, args...)`）
3. 交给 `LoggerFormat` 生成最终字符串（包含换行、时间、logger 名等）
4. 调用 `LogIt()` 将消息落地（同步或异步）

关键点：

- `Logger` 内部持有 `LoggerFormat::ptr _format`
- 如果构建时忘记设置 formatter，会 fallback 到 `NormalFormat`，避免空指针崩溃

---

### 3) 同步日志（SyncLogger）

`SyncLogger::LogIt()`：

- 使用互斥锁 `_mutex` 保护同一 logger 的多 sink 写入
- 逐个 sink 调用 `sink->log(msg.data(), msg.size())`

适用：
- 日志量不大
- 更关注“立刻可见”与简单性

---

### 4) 异步日志（AsyncLogger）

`AsyncLogger` 的核心是 `AsyncWorker`：

- `LogIt()` 只负责 `_looper->push(msg)`，写入内存 buffer
- 后台线程 `AsyncWorker::worker()`：
  - 等待数据
  - swap push/pop buffer
  - 回调 `realLog(Buffer&)` 批量写入 sink

适用：
- 高频日志
- 希望系统主线程更少被 IO 阻塞

> 当前实现说明：
> - `AsyncWorker` 析构会 `stop()` 并 `join()`，从而尽可能把缓冲区写完
> - `test.cpp` 里为了更稳，也额外 sleep 了一小段时间用于“排空”

---

### 5) Sink（落地方式）

`sink.hpp` 已做过一次“C 风格 -> C++ 风格”重构，技术点：

- 使用 `std::filesystem::create_directories()` 创建父目录
- 使用 RAII 的 `std::ofstream` 管理文件句柄
- 使用 `std::put_time` / `std::tm` 做时间格式化（用于滚动文件名等）
- 文件写入后 `flush()`，避免短进程退出时缓冲未落盘导致“日志不完整”

常见 sink：

- `StdoutSink`：`std::cout.write + flush`
- `FileSink`：固定文件追加
- `RollSink`：按大小滚动（超过阈值创建新文件）
- `DailyRollSink`：按天滚动

---

## 构建方式（CMake）

### 依赖

- C++17 编译器（GCC/Clang/MSVC）
- 无需系统安装 fmt（使用 vendored 版本）

### 构建步骤

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

产物：

- `build/ylog_test`

> 注意：本项目过程里你可能还会看到根目录下有一个旧的 `./test` 可执行文件，这可能是历史手工编译留下的，建议以 `./build/ylog_test` 为准。

---

## 使用方式大全

下面所有示例都在 `test.cpp` 里能找到对应用法。

### 1) root logger 快捷函数（logd/logi/...）

```cpp
logd("这是 DEBUG: {}", 42);
logi("这是 INFO");
logw("这是 WARN");
loge("这是 ERROR");
logf("这是 FATAL");
```

说明：

- 这些函数最终会调用 `LoggerMgr::getInstance().rootLogger()`
- root logger 的默认配置在 `loggerMgr.hpp` 的 `LoggerMgr()` 构造函数中

---

### 2) 指定 logger 版本（推荐在工程里更可控）

```cpp
LoggerBuilder builder;
builder.buildLoggerName("root");
builder.buildLoggerLevel(LogLevel::Value::DEBUG);
builder.buildLoggerType(Logger::Type::LOGGER_SYNC);
builder.buildLoggerFormat(LoggerFormat::FormatType::FORMAT_DETAIL);

builder.buildSink<StdoutSink>();
builder.buildSink<FileSink>("./logs/app.log");

auto logger = builder.build();

logi(logger, "start ok, pid={}", 1234);
```

---

### 3) 同步日志器（SyncLogger）

只需要 builder 里指定：

```cpp
builder.buildLoggerType(Logger::Type::LOGGER_SYNC);
```

特点：

- 每条日志在调用现场就写入 sink
- 简单直观

---

### 4) 异步日志器（AsyncLogger）

只需要 builder 里指定：

```cpp
builder.buildLoggerType(Logger::Type::LOGGER_ASYNC);
```

特点：

- 调用线程只做入队（push 到 buffer），写文件在后台线程进行
- 更适合高频场景

---

### 5) 多线程并发压测（异步日志）

`test.cpp` 已提供一个基础压测：

- 线程数：8
- 每线程写入：2000
- 总计：16000 行
- 输出：`./logs/async_mt.log`

核心代码片段：

```cpp
LoggerBuilder async_builder;
async_builder.buildLoggerName("async_mt");
async_builder.buildLoggerType(Logger::Type::LOGGER_ASYNC);
async_builder.buildLoggerFormat(LoggerFormat::FormatType::FORMAT_DETAIL);
async_builder.buildSink<FileSink>("./logs/async_mt.log");
auto async_logger = async_builder.build();

std::vector<std::thread> threads;
for (int t = 0; t < kThreads; ++t) {
  threads.emplace_back([async_logger, t]() {
    for (int i = 0; i < kPerThread; ++i)
      logi(async_logger, "tid={}, i={}, msg={}", t, i, "hello");
  });
}
for (auto& th : threads) th.join();
```

**注意事项（很重要）**：

- 异步 logger 需要时间让后台线程把 buffer 写完。
- 当前工程没有显式 `flush()` API，所以测试里加了一个短暂 sleep。
- 如果你要在真实业务中使用，建议后续加：
  - `Logger::flush()` / `AsyncLogger::stop()` 或 `LoggerMgr::shutdown()`

---

### 6) 按大小滚动（RollSink）

```cpp
builder.buildSink<RollSink>("./logs/test.log", 1024 * 1024 * 10); // 10MB
```

逻辑：

- 当前文件累计写入达到阈值则生成一个新文件名（带时间戳）

---

### 7) 按天滚动（DailyRollSink）

```cpp
builder.buildSink<DailyRollSink>("./logs/daily_");
```

逻辑：

- 每天一个文件

---

## 常见问题（FAQ）

### Q1: 为什么我设置了 detail，但文件里还是 `[INFO] ...`？

- 请确认你看的那一段日志是由哪个 logger 输出的：
  - `logi("...")` 走的是 **root logger**
  - `logi(logger, "...")` 走的是你手工 build 的 logger
- root logger 的配置在 `LoggerMgr` 构造中定义。

---

### Q2: 为什么会出现日志粘在一起（不换行）？

- 现在换行由 formatter 统一追加（`loggerFormat.hpp`），每条日志都会以 `\n` 结束。
- 如果你自定义 formatter，记得保证每条日志最终以换行结尾。

---

### Q3: 为什么 `./test` 会崩，但 `./build/ylog_test` 没事？

- 通常是因为 `./test` 是历史手工编译产物，链接/代码版本不一致。
- 建议统一使用 CMake 产物：`./build/ylog_test`。

---

## 下一步建议（可选增强）

如果你要把它做成更像真实可用的日志库，建议按这个顺序增强：

1. **显式 flush/shutdown**：保证异步队列可控排空
2. **LoggerMgr 注册/替换 root 的 API**：更符合用户注释 "builder 会自动注册" 的预期
3. **异步队列策略**：满了是阻塞等待还是丢弃/覆盖
4. **批量写入优化**：减少 flush 的频率、按批次刷盘
5. **单元测试**：至少包含多线程一致性与滚动文件边界测试
