/**
 * @file dbTaskManagerTest.cpp
 * @brief DBTaskManager单元测试
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Library Manager.
 * Created: 2025-04-14 10:30:00 (UTC)
 * Author: chenxu
 */
#include "db/task/dbTaskManager.h"

#include <atomic>
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <thread>

namespace leigod {
namespace nngame {
namespace library {
namespace db {
namespace test {

// 创建一个测试用Task类
class TestTask : public common::Task {
public:
    TestTask(common::TaskResult result, std::atomic<int>& counter, int delayMs = 0)
        : result_(result), counter_(counter), delayMs_(delayMs) {}

    common::TaskResult onRun() override {
        if (delayMs_ > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs_));
        }
        counter_++;
        return result_;
    }

    std::chrono::milliseconds getRetryInterval() const override {
        return std::chrono::milliseconds(0);
    }

    void onError(int code, const std::string& error) override {}

    void onRelease() override {}

    void onCancel() override {}

private:
    common::TaskResult result_;
    std::atomic<int>& counter_;
    int delayMs_;
};

class DBTaskManagerT : public DBTaskManager {
public:
    bool execTask(std::shared_ptr<common::Task> task, std::shared_ptr<common::TaskData> data) {
        return doTask(task, data);
    }

    bool execDelayedTask(std::shared_ptr<common::Task> task) {
        return doDelayedTask(task);
    }

    std::shared_ptr<common::TaskData> getWorkData() {
        return createWorkData();
    }
};

class DBTaskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        taskManager_ = std::make_unique<DBTaskManagerT>();
    }

    void TearDown() override {
        taskManager_->stop();
        taskManager_.reset();
    }

    std::unique_ptr<DBTaskManagerT> taskManager_;
};

// 测试构造函数
TEST_F(DBTaskManagerTest, Constructor) {
    EXPECT_FALSE(taskManager_->isRunning());
    EXPECT_EQ(taskManager_->getType(), common::TaskManager::ManagerType::Normal);
}

// 测试doTask方法 - 成功场景
TEST_F(DBTaskManagerTest, DoTaskSuccess) {
    std::atomic<int> counter(0);
    auto task = std::make_shared<TestTask>(common::TaskResult::TASK_RESULT_SUCCESS, counter);

    bool result = taskManager_->execTask(task, nullptr);
    EXPECT_FALSE(result);   // doTask返回!TASK_RESULT_SUCCESS
    EXPECT_EQ(counter, 1);  // 确认任务执行了一次
}

// 测试doTask方法 - 失败场景
TEST_F(DBTaskManagerTest, DoTaskFailure) {
    std::atomic<int> counter(0);
    auto task = std::make_shared<TestTask>(common::TaskResult::TASK_RESULT_FAILED, counter);

    bool result = taskManager_->execTask(task, nullptr);
    EXPECT_TRUE(result);    // doTask返回!TASK_RESULT_SUCCESS
    EXPECT_EQ(counter, 1);  // 确认任务执行了一次
}

// 测试doDelayedTask方法
TEST_F(DBTaskManagerTest, DoDelayedTask) {
    std::atomic<int> counter(0);
    auto task = std::make_shared<TestTask>(common::TaskResult::TASK_RESULT_SUCCESS, counter);

    bool result = taskManager_->execDelayedTask(task);
    EXPECT_TRUE(result);    // 应该总是返true
    EXPECT_EQ(counter, 0);  // 不应该执行task的run方法
}

// 测试createWorkData方法
TEST_F(DBTaskManagerTest, CreateWorkData) {
    auto data = taskManager_->getWorkData();
    EXPECT_EQ(data, nullptr);  // 应该返回nullptr
}

// 测试任务执行流程
TEST_F(DBTaskManagerTest, TaskExecution) {
    std::atomic<int> counter(0);
    taskManager_->start(2);  // 启动2个工作线程

    EXPECT_TRUE(taskManager_->isRunning());

    auto task1 = std::make_shared<TestTask>(common::TaskResult::TASK_RESULT_SUCCESS, counter);
    auto task2 = std::make_shared<TestTask>(common::TaskResult::TASK_RESULT_SUCCESS, counter);

    taskManager_->enqueue(task1);
    taskManager_->enqueue(task2);

    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(counter, 2);

    taskManager_->stop();
    EXPECT_FALSE(taskManager_->isRunning());
}

// 测试并发任务执行
TEST_F(DBTaskManagerTest, ConcurrentTasks) {
    std::atomic<int> counter(0);
    taskManager_->start(4);  // 启动4个工作线程

    constexpr int NUM_TASKS = 100;
    std::vector<std::shared_ptr<TestTask>> tasks;

    for (int i = 0; i < NUM_TASKS; i++) {
        auto task = std::make_shared<TestTask>(common::TaskResult::TASK_RESULT_SUCCESS, counter, 5);
        tasks.push_back(task);
        taskManager_->enqueue(task);
    }

    // 等待足够长的时间以确保所有任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_EQ(counter, NUM_TASKS);
}

}  // namespace test
}  // namespace db
}  // namespace library
}  // namespace nngame
}  // namespace leigod
