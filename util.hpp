#ifndef _YLOG_UTIL_HPP_
#define _YLOG_UTIL_HPP_

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdint>
#include <cassert>

// 跨平台处理
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN  // 减少 Windows.h 包含的内容
    #endif
    #include <windows.h>     // Windows: FILETIME, GetSystemTimeAsFileTime
    #include <direct.h>      // Windows: _mkdir()
    #include <io.h>          // Windows: _access()
    #include <sys/stat.h>    // Windows: struct _stat
    #ifndef access
        #define access _access
    #endif
    #ifndef mkdir
        #define mkdir(path, mode) _mkdir(path)  // Windows 忽略 mode 参数
    #endif
#else
    #include <sys/stat.h>    // Linux/Unix: mkdir(), stat()
    #include <unistd.h>      // Linux/Unix: access()
    #include <sys/time.h>    // Linux/Unix: gettimeofday()
#endif


// 现代 {fmt} 方式 - 直接传 chrono 时间点
// #include <chrono>
// using namespace std::chrono;

// auto now = system_clock::now();
// fmt::format("{:%Y-%m-%d %H:%M:%S}", now);

namespace YLog
{
    namespace util
    {
        class date {
        public:
            // 获取当前时间戳（秒级）
            static time_t now()
            {
                return time(nullptr);
            }

            // 获取当前时间戳（毫秒级）
            static uint64_t now_ms()
            {
#ifdef _WIN32
                // Windows 实现
                FILETIME ft;
                GetSystemTimeAsFileTime(&ft);
                ULARGE_INTEGER ui;
                ui.LowPart = ft.dwLowDateTime;
                ui.HighPart = ft.dwHighDateTime;
                return (ui.QuadPart - 116444736000000000ULL) / 10000;
#else
                // Linux/Unix 实现
                struct timeval tv;
                gettimeofday(&tv, nullptr);
                return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
            }
        };

        class file {
        public:
            // 判断文件或目录是否存在（跨平台）
            static bool exists(const std::string &name)
            {
                if (name.empty())
                    return false;

#ifdef _WIN32
                // Windows: 使用 _access
                struct _stat st;
                return _stat(name.c_str(), &st) == 0;
#else
                // Linux/Unix: 使用 stat 或 access
                struct stat st;
                return stat(name.c_str(), &st) == 0;
#endif
            }

            // 判断是否是目录
            static bool is_directory(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return false;

#ifdef _WIN32
                struct _stat st;
                if (_stat(name.c_str(), &st) != 0)
                    return false;
                return (st.st_mode & _S_IFDIR) != 0;
#else
                struct stat st;
                if (stat(name.c_str(), &st) != 0)
                    return false;
                return S_ISDIR(st.st_mode);
#endif
            }

            // 判断是否是普通文件
            static bool is_regular_file(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return false;

#ifdef _WIN32
                struct _stat st;
                if (_stat(name.c_str(), &st) != 0)
                    return false;
                return (st.st_mode & _S_IFREG) != 0;
#else
                struct stat st;
                if (stat(name.c_str(), &st) != 0)
                    return false;
                return S_ISREG(st.st_mode);
#endif
            }

            // 获取文件大小（字节）
            static size_t size(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return 0;

#ifdef _WIN32
                struct _stati64 st;
                if (_stati64(name.c_str(), &st) != 0)
                    return 0;
                return (size_t)st.st_size;
#else
                struct stat st;
                if (stat(name.c_str(), &st) != 0)
                    return 0;
                return st.st_size;
#endif
            }

            // 根据文件名获取文件所在目录路径
            static std::string path(const std::string &name)
            {
                // 空路径返回当前目录
                if (name.empty())
                    return ".";

                // 查找最后一个斜杠或反斜杠的位置
                size_t pos = name.find_last_of("/\\");
                // 没有找到路径分隔符，返回当前目录
                if (pos == std::string::npos)
                    return ".";

                // 返回目录部分（包含最后的分隔符）
                return name.substr(0, pos + 1);
            }

            // 获取文件名（不含路径）
            static std::string filename(const std::string &name)
            {
                if (name.empty())
                    return "";

                size_t pos = name.find_last_of("/\\");
                if (pos == std::string::npos)
                    return name;

                return name.substr(pos + 1);
            }

            // 递归创建目录（跨平台）
            static void create_directory(const std::string &path)
            {
                if (path.empty())
                    return;

                // 如果已存在，直接返回
                if (exists(path))
                    return;

                size_t pos, idx = 0;
                while (idx < path.size())
                {
                    // 查找下一个路径分隔符
                    pos = path.find_first_of("/\\", idx);

                    // 没有更多分隔符，创建最终路径
                    if (pos == std::string::npos)
                    {
                        // 跳过最后的文件名部分
                        size_t last_sep = path.find_last_of("/\\");
                        if (last_sep != std::string::npos)
                        {
                            std::string dir = path.substr(0, last_sep);
                            if (!exists(dir))
                                mkdir(dir.c_str(), 0755);
                        }
                        return;
                    }

                    // 处理连续的分隔符 //
                    if (pos == idx)
                    {
                        idx = pos + 1;
                        continue;
                    }

                    // 提取子目录
                    std::string subdir = path.substr(0, pos);

                    // 跳过 . 和 .. 以及已存在的目录
                    if (subdir == "." || subdir == ".." || exists(subdir))
                    {
                        idx = pos + 1;
                        continue;
                    }

                    // 创建子目录
                    mkdir(subdir.c_str(), 0755);
                    idx = pos + 1;
                }
            }

            // 删除文件
            static bool remove(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return false;

#ifdef _WIN32
                    return _unlink(name.c_str()) == 0;
#else
                    return ::remove(name.c_str()) == 0;
#endif
            }

            // 重命名文件
            static bool rename(const std::string &old_name, const std::string &new_name)
            {
                if (old_name.empty() || new_name.empty())
                    return false;

#ifdef _WIN32
                // Windows 需要先删除目标文件（如果存在）
                if (exists(new_name))
                    remove(new_name);
#endif

                return std::rename(old_name.c_str(), new_name.c_str()) == 0;
            }
        };
    }
}

#endif
