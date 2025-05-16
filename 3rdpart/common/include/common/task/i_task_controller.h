/**
 * @file ITaskController.h
 * @brief Interface for task controller
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Common.
 * Created: 2025-04-01
 * Author: chenxu
 */

#ifndef LEIGOD_COMMON_ITASKCONTROLLER_H
#define LEIGOD_COMMON_ITASKCONTROLLER_H

#include "Task.h"

#include <memory>
#include <string>

namespace leigod {
namespace common {
class ITaskController {
public:
    // 处理异步任务
    virtual void asyncTask(std::shared_ptr<Task> task) = 0;

    // 处理同步任务
    virtual void syncTask(std::shared_ptr<Task> task) = 0;
};
}  // namespace common
}  // namespace leigod

#endif  // LEIGOD_COMMON_ITASKCONTROLLER_H
