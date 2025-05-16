/**
 * @file Task.h
 * @brief Task listener interface for handling task results.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Common.
 * Created: 2025-04-01
 * Author: chenxu
 */

#ifndef LEIGOD_COMMON_TASK_H
#define LEIGOD_COMMON_TASK_H

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace leigod {
namespace common {

enum TaskStatus {
    TASK_STATUS_NONE,
    TASK_STATUS_RUNNING,
    TASK_STATUS_SUCCESS,
    TASK_STATUS_FAILED,
    TASK_STATUS_CANCELED,
    TASK_STATUS_RELEASED
};

enum TaskResult {
    TASK_RESULT_SUCCESS,
    TASK_RESULT_FAILED,
    TASK_RESULT_CANCELED,
    TASK_RESULT_ERROR_MANAGER,
    TASK_RESULT_ERROR_STATUS,
    TASK_RESULT_ERROR_UNKNOWN,
    TASK_RESULT_ERROR_RETRY
};

class TaskManager;

class Task : public std::enable_shared_from_this<Task> {
public:
    Task() {
        id_ = nextId_.fetch_add(1, std::memory_order_relaxed);
    }

    virtual ~Task() = default;

    void error(int code, const std::string& err);

    void release();

    TaskResult run();

    void reset();

    int64_t getId();

    void setTag(int64_t tag);

    int64_t getTag() const;

    void cancel();

    bool isCanceled() const;

    virtual std::chrono::milliseconds getRetryInterval() const = 0;

protected:
    virtual TaskResult onRun() = 0;

    virtual void onError(int code, const std::string& error) = 0;

    virtual void onRelease() = 0;

    virtual void onCancel() = 0;

protected:
    int64_t id_ = 0;   // the id for this task
    int64_t tag_ = 0;  // the tag for this task, use for controller this task
    std::atomic<TaskStatus> status_ = TASK_STATUS_NONE;
    std::atomic_bool is_cancel_ = false;

private:
    static std::atomic<int64_t> nextId_;
};
}  // namespace common
}  // namespace leigod

#endif  // LEIGOD_COMMON_TASK_H
