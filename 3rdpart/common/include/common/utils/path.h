/**
 * @file path_util.hpp
 * @brief Utility functions for working with filesystem paths
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 * @note
 * Created: 2025-03-19
 * Author: chenxu
 */

#ifndef LEIGOD_INSTALLER_UTIL_PATH_UTIL_HPP_
#define LEIGOD_INSTALLER_UTIL_PATH_UTIL_HPP_

#include "strings.h"

#include <filesystem>
#include <stack>
#include <string>

#ifdef LEIGOD_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace leigod {
namespace common {
namespace utils {

inline std::string pathToStr(const std::filesystem::path& path) {
#ifdef LEIGOD_PLATFORM_WINDOWS
    auto u8_string = path.u8string();

    int size = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(u8_string.data()),
                                   static_cast<int>(u8_string.size()), nullptr, 0);
    std::wstring wstr(size, 0);
    if (MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(u8_string.data()),
                            static_cast<int>(u8_string.size()), &wstr[0], size) <= 0) {
        throw std::runtime_error("Failed to convert UTF-8 to wide string");
    }

    size_t ansi_len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), static_cast<int>(wstr.length()),
                                       nullptr, 0, nullptr, nullptr);
    if (ansi_len <= 0) {
        throw std::runtime_error("Failed to calculate ANSI string length");
    }

    std::vector<char> ansi_str(ansi_len);
    if (WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), static_cast<int>(wstr.length()),
                            ansi_str.data(), static_cast<int>(ansi_len), nullptr, nullptr) <= 0) {
        throw std::runtime_error("Failed to convert wide string to ANSI");
    }

    return {ansi_str.data(), ansi_len};
#else
    return path.string();
#endif
}

// find file in the directory by name, subdirectories are searched
inline std::filesystem::path findFile(const std::filesystem::path& directory,
                                      const std::filesystem::path& name) {
    std::stack<std::filesystem::path> dirs_to_search;
    dirs_to_search.push(directory);

    while (!dirs_to_search.empty()) {
        std::filesystem::path current_dir = dirs_to_search.top();
        dirs_to_search.pop();

        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(current_dir, ec)) {
            if (ec) {
                continue;  // Skip inaccessible directories
            }
            auto filename = entry.path().filename();
            if (entry.is_regular_file() && entry.path().filename() == name) {
                return entry.path();
            } else if (entry.is_directory()) {
                dirs_to_search.push(entry.path());
            }
        }
    }

    return {};
}

/**
 * @brief Fixes the path for Windows by replacing '/' with '\\'
 * @param path The original path
 * @return The fixed path
 */
inline std::string fixPath(const std::string& path) {
    std::string fixed_path;
#ifdef LEIGOD_PLATFORM_WINDOWS
    fixed_path.reserve(path.size() * 2);  // 预留空间提升效率
    for (char c : path) {
        if (c == '\\') {
            fixed_path += '\\';
        }
        fixed_path += c;
    }
#else
    for (auto& c : fixed_path) {
        if (c == '\\') {
            fixed_path += '/';
        } else {
            fixed_path += c;
        }
    }
#endif

    return fixed_path;
}

}  // namespace utils
}  // namespace common
}  // namespace leigod

#endif  // LEIGOD_INSTALLER_UTIL_PATH_UTIL_HPP_
