/**
 * @file TaskManager.h
 * @brief TaskManager class for managing tasks and delayed tasks.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Common.
 * Created: 2025-04-01
 * Author: chenxu
 */

#ifndef LEIGOD_COMMON_TASKMANAGER_H
#define LEIGOD_COMMON_TASKMANAGER_H

#include "task.h"
#include "task_data.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace leigod {
namespace common {
class TaskManager : public std::enable_shared_from_this<TaskManager> {
public:
    enum class ManagerType { Normal = (1 << 0), Delayed = (1 << 1), Both = Normal | Delayed };

protected:
    struct DelayedTask {
        std::shared_ptr<Task> task{};
        std::chrono::milliseconds delay{};
    };

public:
    TaskManager(ManagerType type = ManagerType::Both) : type_(type){};

    virtual ~TaskManager();

    void enqueue(std::shared_ptr<Task> task);

    void delayedEnqueue(std::shared_ptr<Task> task, std::chrono::milliseconds delay);

    void start(int num_threads);

    void stop();

    void cancel(int64_t id);

    bool isRunning();

    ManagerType getType();

protected:
    /**
     * 执行任务
     * @param task
     * @return true: 任务被消费
     */
    virtual bool doTask(std::shared_ptr<Task> task, std::shared_ptr<TaskData> data) = 0;

    virtual bool doDelayedTask(std::shared_ptr<Task> task) = 0;

    // 从子类获取绑定到线程的数据
    virtual std::shared_ptr<TaskData> createWorkData() = 0;

private:
    void worker(int index, std::shared_ptr<TaskData> data);

    void delayedWorker();

protected:
    std::vector<std::thread> workers_;
    std::deque<std::shared_ptr<Task>> task_queue_;
    std::mutex queue_mutex_{};
    std::condition_variable condition_;
    std::mutex mutex_{};

    std::thread delayed_worker_;
    std::vector<DelayedTask> delayed_tasks_;
    std::mutex delayed_mutex_{};
    std::atomic_bool stop_ = false;

    std::unordered_map<int64_t, std::weak_ptr<Task>> all_tasks_{};  // 当前正在运行的任务
    std::mutex all_tasks_mutex_{};

    // 管理器类型
    ManagerType type_ = ManagerType::Both;

    std::atomic_bool running_ = false;  // 是否正在运行
};
}  // namespace common
}  // namespace leigod

#endif  // LEIGOD_COMMON_TASKMANAGER_H
