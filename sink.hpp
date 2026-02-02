#ifndef _YLOG_SINK_HPP__
#define _YLOG_SINK_HPP__

#include "util.hpp"
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <iomanip>

namespace YLog {

namespace detail {
inline void ensure_parent_dir_exists(const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::path p(filename);
    auto parent = p.parent_path();
    if (!parent.empty())
    {
        std::error_code ec;
        fs::create_directories(parent, ec);
        if (ec)
        {
            throw std::runtime_error("Failed to create directory: " + parent.string() + ", " + ec.message());
        }
    }
}

inline std::tm local_tm(std::time_t t)
{
    std::tm out{};
#ifdef _WIN32
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
    return out;
}

inline std::string format_time(const std::tm& tm, const char* fmt)
{
    std::ostringstream oss;
    oss << std::put_time(&tm, fmt);
    return oss.str();
}
}
// 抽象日志落地类
class LogSink {
public:
    using ptr = std::shared_ptr<LogSink>;
    LogSink() = default;
    virtual ~LogSink() = default;
    virtual void log(const char *data, size_t len) = 0;
};

// 标准输出落地（控制台）
class StdoutSink : public LogSink {
public:
    using ptr = std::shared_ptr<StdoutSink>;
    StdoutSink() = default;
    ~StdoutSink() override = default;

    void log(const char *msg, size_t len) override
    {
        std::cout.write(msg, len);
        std::cout.flush();  // 确保立即输出
    }
};

// 标准错误输出落地
class StderrSink : public LogSink {
public:
    using ptr = std::shared_ptr<StderrSink>;
    StderrSink() = default;
    ~StderrSink() override = default;

    void log(const char *msg, size_t len) override
    {
        std::cerr.write(msg, len);
        std::cerr.flush();  // 确保立即输出
    }
};

// 文件落地（固定文件名）
class FileSink : public LogSink {
public:
    using ptr = std::shared_ptr<FileSink>;
    FileSink(const std::string &filename) : _filename(filename)
    {
        // C++17: create parent directories via std::filesystem
        detail::ensure_parent_dir_exists(_filename);
        // 以二进制追加模式打开
        _ofs.open(_filename, std::ios::binary | std::ios::app);

        if (!_ofs.is_open()) 
        {
            throw std::runtime_error("Failed to open log file: " + _filename);
        }
    }

    ~FileSink() override
    {
        if (_ofs.is_open()) 
        {
            _ofs.close();
        }
    }

    const std::string &file() const { return _filename; }

    void log(const char *msg, size_t len) override
    {
        if (_ofs.good())
        {
            _ofs.write(msg, len);
            // Make log durable/visible immediately. This avoids "partial" output
            // in short-lived programs or when only file sink is enabled.
            _ofs.flush();
        }
        else
        {
            std::cerr << "LogSink: Failed to write to file: " << _filename << std::endl;
        }
    }

    // 手动刷新缓冲区
    void flush()
    {
        if (_ofs.is_open()) 
        {
            _ofs.flush();
        }
    }

private:
    std::string _filename;
    std::ofstream _ofs;
};

// 滚动文件落地（按大小滚动）
class RollSink : public LogSink {
public:
    using ptr = std::shared_ptr<RollSink>;
    RollSink(const std::string &basename, size_t max_size)
        : _basename(basename),
            _max_fsize(max_size),
            _cur_fsize(0)
    {
        // Create parent dir from basename (basename may include a directory prefix)
        detail::ensure_parent_dir_exists(_basename);
    }

    ~RollSink() override
    {
        if (_ofs.is_open()) 
        {
            _ofs.close();
        }
    }

    void log(const char *data, size_t len) override
    {
        initLogFile();

        if (_ofs.good())
        {
            _ofs.write(data, len);
            _cur_fsize += len;
            _ofs.flush();
        }
        else
        {
            std::cerr << "RollSink: Failed to write to log file" << std::endl;
        }
    }

    // 手动刷新缓冲区
    void flush()
    {
        if (_ofs.is_open()) 
        {
            _ofs.flush();
        }
    }

private:
    void initLogFile()
    {
        // 文件未打开 或 当前大小超过限制
        if (!_ofs.is_open() || _cur_fsize >= _max_fsize)
        {
            if (_ofs.is_open()) {
                _ofs.close();
            }

            std::string name = createFilename();
            _ofs.open(name, std::ios::binary | std::ios::app);

            if (!_ofs.is_open()) {
                std::cerr << "RollSink: Failed to open file: " << name << std::endl;
                return;
            }

            _cur_fsize = 0;
        }
    }

    // 生成带时间戳的文件名
    std::string createFilename()
    {
        namespace fs = std::filesystem;
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        auto tm = detail::local_tm(t);

        // Preserve directory + base filename prefix in _basename, append timestamp + .log
        fs::path base(_basename);
        std::string stem = base.filename().string();
        fs::path parent = base.parent_path();

        std::string stamp = detail::format_time(tm, "%Y%m%d%H%M%S");
        fs::path filename = parent / (stem + stamp + ".log");
        return filename.string();
    }

    std::string _basename;
    std::ofstream _ofs;
    size_t _max_fsize;
    size_t _cur_fsize;
};

// 按时间滚动的文件落地（每日一个文件）
class DailyRollSink : public LogSink {
public:
    using ptr = std::shared_ptr<DailyRollSink>;
    DailyRollSink(const std::string &basename)
        : _basename(basename),
            _current_day(0)
    {
        detail::ensure_parent_dir_exists(_basename);
        initLogFile();
    }

    ~DailyRollSink() override
    {
        if (_ofs.is_open()) 
        {
            _ofs.close();
        }
    }

    void log(const char *data, size_t len) override
    {
        checkRoll();
        if (_ofs.good())
        {
            _ofs.write(data, len);
            _ofs.flush();
        }
    }

    void flush()
    {
        if (_ofs.is_open()) 
        {
            _ofs.flush();
        }
    }

private:
    void checkRoll()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        auto tm = detail::local_tm(t);

        // 计算一年中的第几天
        int current_day = tm.tm_yday;

        // 日期改变，创建新文件
        if (current_day != _current_day)
        {
            if (_ofs.is_open()) 
            {
                _ofs.close();
            }
            _current_day = current_day;
            initLogFile();
        }
    }

    void initLogFile()
    {
        namespace fs = std::filesystem;
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        auto tm = detail::local_tm(t);

        fs::path base(_basename);
        std::string stem = base.filename().string();
        fs::path parent = base.parent_path();

        std::string day = detail::format_time(tm, "%Y%m%d");
        fs::path filename = parent / (stem + day + ".log");

        _ofs.open(filename.string(), std::ios::binary | std::ios::app);

        if (!_ofs.is_open()) 
        {
            std::cerr << "DailyRollSink: Failed to open file: " << filename.string() << std::endl;
        }
    }

    std::string _basename;
    std::ofstream _ofs;
    int _current_day;
};

// 工厂模式：创建不同类型的 Sink
class SinkFactory {
public:
    template <typename SinkType, typename... Args>
    static LogSink::ptr create(Args&& ...args)
    {
        return std::make_shared<SinkType>(std::forward<Args>(args)...);
    }
};

}

#endif
