/**
 * @file TaskManagerTest.cpp
 * @brief Unit tests for TaskManager class
 */

#include "common/task/Task.h"
#include "library/task/taskManager.h"
#include "libraryListener.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace leigod {
namespace nngame {
namespace library {
namespace test {

class MockTask : public common::Task {
public:
    std::chrono::milliseconds getRetryInterval() const override {
        return std::chrono::milliseconds(100);
    }

    MOCK_METHOD(common::TaskResult, onRun, (), (override));
    MOCK_METHOD(void, onError, (int, const std::string&), (override));
    MOCK_METHOD(void, onRelease, (), (override));
    MOCK_METHOD(void, onCancel, (), (override));
};

class MockTaskManager : public TaskManager {
public:
    MockTaskManager() : TaskManager() {}

    bool doTask(std::shared_ptr<common::Task> task,
                std::shared_ptr<common::TaskData> data) override {
        return TaskManager::doTask(task, data);
    }

    bool doDelayedTask(std::shared_ptr<common::Task> task) override {
        return TaskManager::doDelayedTask(task);
    }

    std::shared_ptr<common::TaskData> createWorkData() override {
        return nullptr;
    }
};

class TaskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockTask = std::make_shared<MockTask>();
    }

    MockTaskManager taskManager;
    std::shared_ptr<MockTask> mockTask;
};

TEST_F(TaskManagerTest, DoTaskSucceeds) {
    EXPECT_CALL(*mockTask, onRun())
        .WillOnce(::testing::Return(common::TaskResult::TASK_RESULT_SUCCESS));

    bool result = taskManager.doTask(mockTask, nullptr);

    // doTask returns true if the task result is NOT success
    EXPECT_FALSE(result);
}

TEST_F(TaskManagerTest, DoTaskFails) {
    EXPECT_CALL(*mockTask, onRun())
        .WillOnce(::testing::Return(common::TaskResult::TASK_RESULT_FAILED));

    bool result = taskManager.doTask(mockTask, nullptr);

    // doTask returns true if the task result is NOT success
    EXPECT_TRUE(result);
}

TEST_F(TaskManagerTest, DoDelayedTask) {
    bool result = taskManager.doDelayedTask(mockTask);

    EXPECT_TRUE(result);
}

TEST_F(TaskManagerTest, CreateWorkData) {
    auto data = taskManager.createWorkData();

    EXPECT_EQ(data, nullptr);
}

TEST_F(TaskManagerTest, EnqueueTask) {
    // This test requires overrides of the base TaskManager methods to verify
    // that enqueue calls the base class's enqueue method
    EXPECT_CALL(*mockTask, onRun()).Times(0);

    taskManager.enqueue(mockTask);

    // We can't easily verify that the task was enqueued in the base class
    // without modifying the TaskManager class for testing
}

}  // namespace test
}  // namespace library
}  // namespace nngame
}  // namespace leigod
