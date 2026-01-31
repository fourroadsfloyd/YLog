#ifndef _YLOG_SINK_HPP__
#define _YLOG_SINK_HPP__

#include "util.hpp"
#include "message.hpp"
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <ctime>

namespace YLog
{
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
            // 跨平台创建目录
            util::file::create_directory(util::file::path(_filename));
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
            // 跨平台创建目录
            util::file::create_directory(util::file::path(basename));
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
            time_t t = time(nullptr);
            struct tm lt;

            // 跨平台时间转换
            #ifdef _WIN32
                        localtime_s(&lt, &t);  // Windows: localtime_s(结果, 输入)
            #else
                        localtime_r(&t, &lt);  // Linux: localtime_r(输入, 结果)
            #endif

            std::stringstream ss;
            ss << _basename;
            ss << lt.tm_year + 1900;  // 年（tm_year 是从 1900 开始的）
            ss << (lt.tm_mon + 1) / 10 << (lt.tm_mon + 1) % 10;  // 月（补零）
            ss << lt.tm_mday / 10 << lt.tm_mday % 10;            // 日（补零）
            ss << lt.tm_hour / 10 << lt.tm_hour % 10;            // 时（补零）
            ss << lt.tm_min / 10 << lt.tm_min % 10;              // 分（补零）
            ss << lt.tm_sec / 10 << lt.tm_sec % 10;              // 秒（补零）
            ss << ".log";

            return ss.str();
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
            util::file::create_directory(util::file::path(basename));
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
            time_t t = time(nullptr);
            struct tm lt;

            #ifdef _WIN32
                        localtime_s(&lt, &t);
            #else
                        localtime_r(&t, &lt);
            #endif

            // 计算一年中的第几天
            int current_day = lt.tm_yday;

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
            time_t t = time(nullptr);
            struct tm lt;

            #ifdef _WIN32
                        localtime_s(&lt, &t);
            #else
                        localtime_r(&t, &lt);
            #endif

            std::stringstream ss;
            ss << _basename;
            ss << lt.tm_year + 1900;
            ss << (lt.tm_mon + 1) / 10 << (lt.tm_mon + 1) % 10;
            ss << lt.tm_mday / 10 << lt.tm_mday % 10;
            ss << ".log";

            std::string filename = ss.str();
            _ofs.open(filename, std::ios::binary | std::ios::app);

            if (!_ofs.is_open()) 
            {
                std::cerr << "DailyRollSink: Failed to open file: " << filename << std::endl;
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
