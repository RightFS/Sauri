/**
 * @file ITaskListener.h
 * @brief Interface for task listeners in the Leigod Common library.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Common.
 * Created: 2025-04-01
 * Author: chenxu
 */

#ifndef LEIDOG_COMMON_ITASKLISTENER_H
#define LEIDOG_COMMON_ITASKLISTENER_H

#include <functional>
#include <memory>
#include <string>

namespace leigod {
namespace common {
class ITaskListener {
public:
    virtual ~ITaskListener() = default;

    virtual void onSuccess(const std::string&) = 0;

    virtual void onFailure(int, const std::string&) = 0;
};
}  // namespace common
}  // namespace leigod

#endif  // LEIDOG_COMMON_ITASKLISTENER_H
