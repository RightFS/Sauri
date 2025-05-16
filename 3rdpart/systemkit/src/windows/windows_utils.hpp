/**
 * @file windows_utils.hpp
 * @brief Common Windows utilities for the SystemKit library
 *
 * @author UnixCodor
 * @date 2025-03-26
 */

#ifndef LEIGOD_SYSTEM_KIT_WINDOWS_UTILS_HPP
#define LEIGOD_SYSTEM_KIT_WINDOWS_UTILS_HPP

#include "common/utils/strings.h"

#include <Windows.h>
#include <algorithm>
#include <string>

namespace leigod {
namespace system_kit {
namespace utils {

/**
 * @brief Get error message for the last Windows error
 * @return String containing the error message
 */
inline std::string getLastErrorMessage() {
    DWORD errorCode = GetLastError();
    if (errorCode == 0) {
        return "No error";
    }

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_IGNORE_INSERTS,
                                 nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

    if (size == 0 || messageBuffer == nullptr) {
        return "Error code: " + std::to_string(errorCode);
    }

    std::wstring wMessage(messageBuffer, size);
    LocalFree(messageBuffer);

    // 去除尾部可能的换行符
    while (!wMessage.empty() && (wMessage.back() == L'\r' || wMessage.back() == L'\n')) {
        wMessage.pop_back();
    }

    return common::utils::wideToUtf8(wMessage) + " (Error code: " + std::to_string(errorCode) + ")";
}

}  // namespace utils
}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SYSTEM_KIT_WINDOWS_UTILS_HPP