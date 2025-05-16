/**
 * @file engine.h
 * @brief Provides utility functions for string operations.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-13 02:24:04 (UTC)
 * Author: chenxu
 */

#ifndef NNGAME_COMMON_EXCEPTION_HPP
#define NNGAME_COMMON_EXCEPTION_HPP

#include "error.h"

#include <sstream>
#include <stdexcept>
#include <string>

namespace leigod {
namespace common {

class Exception : public std::runtime_error {
public:
    template <class T>
    explicit Exception(T code, const std::string& message)
        : std::runtime_error(message), code_(static_cast<int>(code)) {
        // 在构造时预先生成 JSON 错误信息并存储起来，防止悬空指针
        std::stringstream ss;
        ss << R"({"code": )" << code_ << R"(, "message": ")" << std::runtime_error::what()
           << R"("})";
        jsonMessage_ = ss.str();
    }

    [[nodiscard]] const char* what() const noexcept override {
        // 返回预先生成的 JSON 错误信息的 C 字符串指针
        return jsonMessage_.c_str();
    }

    [[nodiscard]] int code() const noexcept {
        return code_;
    }

    [[nodiscard]] const char* message() const noexcept {
        return std::runtime_error::what();
    }

private:
    int code_;
    std::string jsonMessage_;  // 存储 JSON 格式的错误信息
};

}  // namespace common
}  // namespace leigod

#endif  // NNGAME_COMMON_EXCEPTION_HPP
