/**
 * @file Task.cpp
 * @brief Task class implementation
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Common.
 * Created: 2025-04-01
 * Author: chenxu
 */

#include "common/task/task.h"

namespace leigod {
namespace common {

std::atomic<int64_t> Task::nextId_(0);

void Task::error(int code, const std::string& err) {
    TaskStatus status = TASK_STATUS_RUNNING;
    if (!status_.compare_exchange_strong(status, TASK_STATUS_FAILED)) {
        return;
    }

    onError(code, err);
}

void Task::release() {
    TaskStatus status = TASK_STATUS_NONE;
    if (!status_.compare_exchange_strong(status, TASK_STATUS_RELEASED)) {
        return;
    }
    onRelease();
}

TaskResult Task::run() {
    TaskStatus status = TASK_STATUS_NONE;
    if (!status_.compare_exchange_strong(status, TASK_STATUS_RUNNING)) {
        return TASK_RESULT_ERROR_STATUS;
    }
    return onRun();
}

void Task::reset() {
    status_ = TASK_STATUS_NONE;
}

int64_t Task::getId() {
    return id_;
}

void Task::setTag(int64_t tag) {
    tag_ = tag;
}

int64_t Task::getTag() const {
    return tag_;
}

void Task::cancel() {
    status_ = TASK_STATUS_CANCELED;
    is_cancel_ = true;
    onCancel();
}

bool Task::isCanceled() const {
    return is_cancel_;
}
}  // namespace common
}  // namespace leigod
