#ifndef _YLOG_UTIL_HPP_
#define _YLOG_UTIL_HPP_

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdint>
#include <cassert>
#include <filesystem>

// Note: This project targets C++17. For path and filesystem operations,
// prefer std::filesystem over platform-specific syscalls.


namespace YLog
{
    namespace util
    {
        class file {
        public:
            // 判断文件或目录是否存在（跨平台）
            static bool exists(const std::string &name)
            {
                if (name.empty())
                    return false;

                return std::filesystem::exists(std::filesystem::u8path(name));
            }

            // 判断是否是目录
            static bool is_directory(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return false;

                return std::filesystem::is_directory(std::filesystem::u8path(name));
            }

            // 判断是否是普通文件
            static bool is_regular_file(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return false;

                return std::filesystem::is_regular_file(std::filesystem::u8path(name));
            }

            // 获取文件大小（字节）
            static size_t size(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return 0;

                std::error_code ec;
                auto s = std::filesystem::file_size(std::filesystem::u8path(name), ec);
                return ec ? 0 : static_cast<size_t>(s);
            }

            // 根据文件名获取文件所在目录路径
            static std::string path(const std::string &name)
            {
                if (name.empty())
                    return ".";

                auto p = std::filesystem::u8path(name);
                auto parent = p.parent_path();
                if (parent.empty())
                    return ".";

                // Keep previous behavior: return with a trailing separator.
                std::string s = parent.generic_string();
                if (!s.empty() && s.back() != '/')
                    s.push_back('/');
                return s;
            }

            // 获取文件名（不含路径）
            static std::string filename(const std::string &name)
            {
                if (name.empty())
                    return "";

                return std::filesystem::u8path(name).filename().u8string();
            }

            // 递归创建目录（跨平台）
            static void create_directory(const std::string &path)
            {
                if (path.empty())
                    return;

                // If caller passes a file path (e.g. ./logs/app.log), create its parent.
                auto p = std::filesystem::u8path(path);
                auto dir = p;
                if (p.has_filename() && p.has_extension())
                    dir = p.parent_path();

                if (dir.empty())
                    return;

                std::error_code ec;
                std::filesystem::create_directories(dir, ec);
            }

            // 删除文件
            static bool remove(const std::string &name)
            {
                if (name.empty() || !exists(name))
                    return false;

                std::error_code ec;
                return std::filesystem::remove(std::filesystem::u8path(name), ec) && !ec;
            }

            // 重命名文件
            static bool rename(const std::string &old_name, const std::string &new_name)
            {
                if (old_name.empty() || new_name.empty())
                    return false;

                std::error_code ec;
                // To mimic old Windows behavior, remove destination if it exists.
                if (std::filesystem::exists(std::filesystem::u8path(new_name)))
                {
                    std::filesystem::remove(std::filesystem::u8path(new_name), ec);
                    ec.clear();
                }
                std::filesystem::rename(std::filesystem::u8path(old_name), std::filesystem::u8path(new_name), ec);
                return !ec;
            }
        };
    }
}

#endif
