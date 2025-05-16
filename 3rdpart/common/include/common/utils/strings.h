/**
 * @file string.h
 * @brief Provides utility functions for string operations.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-13 02:24:04 (UTC)
 * Author: chenxu
 */

#ifndef LEIGOD_COMMON_UTILS_STRING_HPP
#define LEIGOD_COMMON_UTILS_STRING_HPP

#include "common/error.h"
#include "common/exception.h"

#include <filesystem>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef LEIGOD_PLATFORM_WINDOWS
#include <Windows.h>
#include <algorithm>
#include <sstream>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#include <codecvt>
#include <cstring>
#include <locale>
#endif

namespace leigod {
namespace common {
namespace utils {

/**
 * @brief Checks if the given string starts with the specified prefix.
 *
 * @param str The string to check.
 * @param prefix The prefix to check for.
 * @return true If the string starts with the prefix.
 * @return false If the string does not start with the prefix.
 */
inline bool startsWith(const std::string& str, const std::string& prefix) {
    if (str.empty() || prefix.empty()) {
        return false;
    }
    if (prefix.length() > str.length()) {
        return false;
    }
    return str.compare(0, prefix.length(), prefix) == 0;
}

/**
 * @brief Replaces all occurrences of a substring with another substring.
 *
 * @param str The original string.
 * @param from The substring to be replaced.
 * @param to The substring to replace with.
 * @return A new string with all occurrences of from replaced by to.
 */
inline std::string replaceAll(const std::string& str, const std::string& from,
                              const std::string& to) {
    if (from.empty()) {
        return str;
    }
    std::string result = str;
    size_t startPos = 0;
    while ((startPos = result.find(from, startPos)) != std::string::npos) {
        result.replace(startPos, from.length(), to);
        startPos += to.length();
    }
    return result;
}

/**
 * @brief Trims whitespace characters from the start and end of the string.
 *
 * @param str The string to trim.
 * @return A new string with leading and trailing whitespace removed.
 */
inline std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    size_t end = str.find_last_not_of(" \t\n\r\f\v");

    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

/**
 * @brief Converts a UTF-8 encoded string to a wide string.
 * @param str
 * @return
 */
inline std::wstring utf82wstring(const std::string& str) {
#ifdef LEIGOD_PLATFORM_WINDOWS
    if (str.empty()) {
        return {};
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr_to(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr_to[0], size_needed);
    return wstr_to;
#else
    if (str.empty()) {
        return {};
    }
    size_t size_needed = mbstowcs(NULL, str.c_str(), 0) + 1;
    std::wstring wstr_to(size_needed, 0);
    mbstowcs(&wstr_to[0], str.c_str(), size_needed);
    return wstr_to;
#endif
}
/**
 * @brief Converts a wide string to a UTF-8 encoded string.
 * @param wstr
 * @return
 */
inline std::string wstring2utf8(const std::wstring& wstr) {
#ifdef LEIGOD_PLATFORM_WINDOWS
    if (wstr.empty()) {
        return {};
    }
    int size_needed =
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str_to(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str_to[0], size_needed, NULL,
                        NULL);
    return str_to;
#else
    if (wstr.empty()) {
        return std::string();
    }
    size_t size_needed = wcstombs(NULL, wstr.c_str(), 0) + 1;
    std::string str_to(size_needed, 0);
    wcstombs(&str_to[0], wstr.c_str(), size_needed);
    return str_to;
#endif
}

/**
 * @brief Convert wide string to UTF-8 string with cross-platform support
 * @param wstr Wide string to convert
 * @return UTF-8 string
 * @throws std::runtime_error if conversion fails
 */
inline std::string wideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }

#ifdef LEIGOD_PLATFORM_WINDOWS
    // Windows implementation using WinAPI
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to convert wide string to UTF-8");
    }

    std::string result(size_needed, 0);
    if (WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &result[0],
                            size_needed, nullptr, nullptr) <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to convert wide string to UTF-8");
    }

    return result;
#else
    // POSIX implementation using C++ standard library
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(wstr);
    } catch (const std::engine& e) {
        throw Exception(ErrorCode::InvalidArgument,
                        "Failed to convert wide string to UTF-8: " + std::string(e.what()));
    }
#endif
}

/**
 * @brief Convert UTF-8 string to wide string with cross-platform support
 * @param str UTF-8 string to convert
 * @return Wide string
 * @throws std::runtime_error if conversion fails
 */
inline std::wstring utf8ToWide(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

#ifdef LEIGOD_PLATFORM_WINDOWS
    // Windows implementation using WinAPI
    int size_needed =
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (size_needed <= 0) {
        throw Exception(ErrorCode::InvalidArgument,
                        "Failed to convert UTF-8 string to wide string");
    }

    std::wstring result(size_needed, 0);
    if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0],
                            size_needed) <= 0) {
        throw Exception(ErrorCode::InvalidArgument,
                        "Failed to convert UTF-8 string to wide string");
    }

    return result;
#else
    // POSIX implementation using C++ standard library
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.from_bytes(str);
    } catch (const std::engine& e) {
        throw Exception(ErrorCode::InvalidArgument,
                        "Failed to convert UTF-8 string to wide string: " + std::string(e.what()));
    }
#endif
}

/**
 * @brief Convert a string to lowercase
 * @param str String to convert
 * @return Lowercase version of the string
 */
inline std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
    return result;
}

/**
 * @brief Convert a string to uppercase
 * @param str String to convert
 * @return Uppercase version of the string
 */
inline std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<unsigned char>(std::toupper(c)); });
    return result;
}

/**
 * @brief Split a string by delimiter
 * @param str String to split
 * @param delimiter Character to split by
 * @return Vector of split strings
 */
inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

/**
 * @brief Converts a string to int64_t.
 *
 * @param str The string to convert.
 * @return int64_t The converted int64_t value.
 * @throws common::Exception if the string cannot be converted.
 */
inline int64_t toInt64(const std::string& str) {
    try {
        size_t pos;
        int64_t value = std::stoll(str, &pos);
        if (pos != str.length()) {
            throw Exception(ErrorCode::ConvertErrorNotNumber, str + " is not a number");
        }
        return value;
    } catch (const std::invalid_argument& e) {
        throw Exception(ErrorCode::InvalidArgument, str + ":" + e.what());
    } catch (const std::out_of_range& e) {
        throw Exception(ErrorCode::ConvertErrorOutOfRange, str + ":" + e.what());
    } catch (const std::exception& e) {
        throw Exception(ErrorCode::ConvertError, str + ":" + e.what());
    }
}

/**
 * @brief Converts a string to double.
 *
 * @param str The string to convert.
 * @return double The converted double value.
 * @throws common::Exception if the string cannot be converted.
 */
inline double toDouble(const std::string& str) {
    try {
        size_t pos;
        double value = std::stod(str, &pos);
        if (pos != str.length()) {
            throw Exception(ErrorCode::ConvertErrorNotNumber, str + " is not a double");
        }
        return value;
    } catch (const std::invalid_argument& e) {
        throw Exception(ErrorCode::InvalidArgument, str + " " + e.what());
    } catch (const std::out_of_range& e) {
        throw Exception(ErrorCode::ConvertErrorOutOfRange, str + " " + e.what());
    } catch (const std::exception& e) {
        throw Exception(ErrorCode::ConvertError, str + " " + e.what());
    }
}

/**
 * @brief Converts a string to std::filesystem::path.
 * @param path The string to convert.
 * @return std::filesystem::path The converted path.
 */
inline std::filesystem::path toPath(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    // On Windows, convert UTF-8 to wide string (UTF-16)
#if defined(LEIGOD_PLATFORM_WINDOWS)
    int wide_size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wide_size > 0) {
        std::wstring wide_path(wide_size - 1, 0);  // -1 to exclude null terminator
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wide_path[0], wide_size);
        return wide_path;
    } else {
        // Fallback to direct assignment if conversion fails
        return path;
    }
#else
    // On POSIX systems, direct assignment should work
    return path;
#endif
}

inline std::string toString(const std::filesystem::path& path) {
    if (path.empty()) {
        return {};
    }
    // On Windows, convert wide string (UTF-16) to UTF-8
#if defined(LEIGOD_PLATFORM_WINDOWS)
    // Convert wide string to UTF-8
    auto wide = path.wstring();
    int size_needed =
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed > 0) {
        std::string str(size_needed - 1, 0);  // -1 to exclude null terminator
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
        return str;
    } else {
        // Fallback to direct assignment if conversion fails
        return path.string();
    }
#else
    // On POSIX systems, direct assignment should work
    return path.string();
#endif
}

inline std::string u8ToString(const std::u8string& u8_string) {
    if (u8_string.empty()) {
        return {};
    }
    try {
// 将u8string转换为标准string
#if defined(_MSC_VER) && _MSC_VER >= 1900 && (defined(_MSVC_LANG) && _MSVC_LANG > 201703L)
        // 对于支持C++20的MSVC编译器
        return std::string(u8_string.begin(), u8_string.end());
#else
        // 对于C++17及更早的编译器
        return std::string(reinterpret_cast<const char*>(u8path.c_str()));
#endif
    } catch ([[maybe_unused]] const std::exception& e) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to convert u8string to string");
    }
}

inline std::wstring u8ToWide(const std::u8string& utf8) {
    int size = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(utf8.data()),
                                   static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(utf8.data()), static_cast<int>(utf8.size()),
                        &wstr[0], size);
    return wstr;
}

/**
 * @brief Convert UTF-8 string to ANSI (Windows system code page)
 * @param utf8_str The UTF-8 encoded string to convert
 * @return ANSI encoded string
 * @throw std::runtime_error if conversion fails
 */
inline std::string utf8ToAnsi(const std::string& utf8_str) {
#ifdef LEIGOD_PLATFORM_WINDOWS
    // 先将 UTF-8 转换为 wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
        throw Exception(ErrorCode::InvalidArgument,
                        "Failed to calculate wide string length for UTF-8");
    }

    std::vector<wchar_t> wstr(wlen);
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wstr.data(), wlen) <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to convert UTF-8 to wide string");
    }

    // 然后将 wide string 转换为 ANSI
    int ansi_len = WideCharToMultiByte(CP_ACP, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr);
    if (ansi_len <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to calculate ANSI string length");
    }

    std::vector<char> ansi_str(ansi_len);
    if (WideCharToMultiByte(CP_ACP, 0, wstr.data(), -1, ansi_str.data(), ansi_len, nullptr,
                            nullptr) <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to convert wide string to ANSI");
    }

    return std::string(ansi_str.data());
#else
    return utf8_str;  // Unix systems usually use UTF-8 by default
#endif
}

/**
 * @brief Convert ANSI string to UTF-8
 * @param ansi_str The ANSI encoded string to convert
 * @return UTF-8 encoded string
 * @throw std::runtime_error if conversion fails
 */
inline std::string ansiToUtf8(const std::string& ansi_str) {
#ifdef LEIGOD_PLATFORM_WINDOWS
    // 先将 ANSI 转换为 wide string
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
        throw Exception(ErrorCode::InvalidArgument,
                        "Failed to calculate wide string length for ANSI");
    }

    std::vector<wchar_t> wstr(wlen);
    if (MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, wstr.data(), wlen) <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to convert ANSI to wide string");
    }

    // 然后将 wide string 转换为 UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to calculate UTF-8 string length");
    }

    std::vector<char> utf8_str(utf8_len);
    if (WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, utf8_str.data(), utf8_len, nullptr,
                            nullptr) <= 0) {
        throw Exception(ErrorCode::InvalidArgument, "Failed to convert wide string to UTF-8");
    }

    return std::string(utf8_str.data());
#else
    return ansi_str;  // Unix systems usually use UTF-8 by default
#endif
}

/**
 * @brief Convert ANSI string to wide string
 * @param ansi_str The ANSI encoded string to convert
 * @return Wide string (wstring)
 * @throw std::runtime_error if conversion fails
 */
inline std::wstring ansiToWide(const std::string& ansi_str) {
    if (ansi_str.empty()) {
        return {};
    }

#ifdef LEIGOD_PLATFORM_WINDOWS
    // Windows: Use MultiByteToWideChar
    int required_length = MultiByteToWideChar(CP_ACP,            // Source code page: CP_ACP
                                              0,                 // No special handling
                                              ansi_str.c_str(),  // Source string
                                              -1,                // Null-terminated string
                                              nullptr,           // No output buffer yet
                                              0                  // Query required size
    );

    if (required_length <= 0) {
        throw Exception(
            ErrorCode::InvalidArgument,
            "Failed to calculate required buffer size for wide string conversion. Error code: " +
                std::to_string(GetLastError()));
    }

    std::vector<wchar_t> buffer(required_length);

    int result = MultiByteToWideChar(CP_ACP,            // Source code page: CP_ACP
                                     0,                 // No special handling
                                     ansi_str.c_str(),  // Source string
                                     -1,                // Null-terminated string
                                     buffer.data(),     // Output buffer
                                     required_length    // Buffer size
    );

    if (result <= 0) {
        throw Exception(ErrorCode::InvalidArgument,
                        "Failed to convert UTF-8 to wide string. Error code: " +
                            std::to_string(GetLastError()));
    }

    // Remove null terminator before creating wstring
    return std::wstring(buffer.data(), buffer.data() + result - 1);

#else
    // Unix-like systems: Use standard C++ facilities
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(utf8_str);
    } catch (const std::engine& e) {
        throw std::runtime_error(std::string("Failed to convert UTF-8 to wide string: ") +
                                 e.what());
    }
#endif
}

}  // namespace utils
}  // namespace common
}  // namespace leigod

#endif  // LEIGOD_COMMON_UTILS_STRING_HPP