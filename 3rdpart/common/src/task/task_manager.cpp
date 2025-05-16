/**
 * @file Task.cpp
 * @brief Task class implementation
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Common.
 * Created: 2025-04-01
 * Author: chenxu
 */

#include "common/task/task_manager.h"

#include "common/task/task.h"

namespace leigod {
namespace common {

// 检查延时队列间隔时间
constexpr int kCheckDelayQueueInterval = 100;  // 毫秒

TaskManager::~TaskManager() {
    stop();
}

void TaskManager::enqueue(std::shared_ptr<Task> task) {
    {
        std::lock_guard<std::mutex> lock(all_tasks_mutex_);
        all_tasks_[task->getId()] = task;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push_back(task);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        condition_.notify_one();
    }
}

void TaskManager::delayedEnqueue(std::shared_ptr<Task> task, std::chrono::milliseconds delay) {
    std::lock_guard<std::mutex> lock(delayed_mutex_);
    delayed_tasks_.emplace_back(DelayedTask{task, delay});
}

void TaskManager::start(int num_threads) {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return;
    }

    // 清空停止标记
    stop_ = false;
    int type_normal = static_cast<int>(ManagerType::Normal);
    int type_delayed = static_cast<int>(ManagerType::Delayed);
    int type = static_cast<int>(type_);

    if ((type_delayed & type) == type_delayed) {
        // 启动延时任务工作线程
        delayed_worker_ = std::thread(&TaskManager::delayedWorker, this);
    }

    if ((type_normal & type) == type_normal) {
        // 启动任务工作线程
        for (int i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&TaskManager::worker, this, i, createWorkData());
        }
    }
}

void TaskManager::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
        condition_.notify_all();
    }

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    if (delayed_worker_.joinable()) {
        delayed_worker_.join();
    }

    {
        std::lock_guard<std::mutex> lock(delayed_mutex_);
        // 处理剩余的延迟任务
        while (!delayed_tasks_.empty()) {
            std::shared_ptr<Task> task = delayed_tasks_.front().task;
            delayed_tasks_.erase(delayed_tasks_.begin());
            task->release();
        }
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        // 处理剩余的任务
        while (!task_queue_.empty()) {
            std::shared_ptr<Task> task = task_queue_.front();
            task_queue_.pop_back();
            task->release();
        }
    }
}

void TaskManager::worker([[maybe_unused]] int index, std::shared_ptr<TaskData> data) {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] { return stop_ || !task_queue_.empty(); });
        }

        std::shared_ptr<Task> task;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_) {
                break;
            }
            if (task_queue_.empty()) {
                continue;
            }
            task = task_queue_.front();
            task_queue_.pop_front();
        }

        if (!task) {
            continue;
        }

        // 是否消费
        if (doTask(task, data)) {
            std::lock_guard<std::mutex> lock(all_tasks_mutex_);
            all_tasks_.erase(task->getId());
        }
    }
}

void TaskManager::delayedWorker() {
    while (!stop_) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(kCheckDelayQueueInterval));  // 检查延迟队列
        std::vector<DelayedTask> tasks_to_process;
        {
            std::lock_guard<std::mutex> lock(delayed_mutex_);
            auto it = delayed_tasks_.begin();
            while (it != delayed_tasks_.end()) {
                if (it->delay.count() <= 0) {
                    auto task = it->task;
                    tasks_to_process.push_back(std::move(*it));
                    it = delayed_tasks_.erase(it);
                } else {
                    it->delay -= std::chrono::milliseconds(kCheckDelayQueueInterval);
                    ++it;
                }
            }
        }
        for (auto& delayedTask : tasks_to_process) {
            if (doDelayedTask(delayedTask.task)) {
                std::lock_guard<std::mutex> lock(all_tasks_mutex_);
                all_tasks_.erase(delayedTask.task->getId());
            }
        }
    }
}

void TaskManager::cancel(int64_t id) {
    {
        std::lock_guard<std::mutex> lock(all_tasks_mutex_);
        auto taskIt = all_tasks_.find(id);
        if (taskIt != all_tasks_.end()) {
            auto task = taskIt->second.lock();
            if (task) {
                task->cancel();
            }
            all_tasks_.erase(taskIt);
            return;
        }
    }
}

bool TaskManager::isRunning() {
    return running_;
}

TaskManager::ManagerType TaskManager::getType() {
    return type_;
}

}  // namespace common
}  // namespace leigod