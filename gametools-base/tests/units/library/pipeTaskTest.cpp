/**
 * @file PipeTaskTest.cpp
 * @brief Unit tests for PipeTask class
 */

#include "library/task/pipeTask.h"
#include "libraryManager.h"
#include "taskListener.h"

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

class MockTaskListener : public TaskListener {
public:
    explicit MockTaskListener(std::shared_ptr<LibraryListener> libraryListener)
        : TaskListener(std::move(libraryListener)) {}
    MOCK_METHOD(void, onProgress, (int percentage, const std::string& message), (override));
    MOCK_METHOD(void, onSuccess, (const std::string& message), (override));
    MOCK_METHOD(void, onFailure, (int code, const std::string& message), (override));
    MOCK_METHOD(void, setAppId, (const std::string& appId), ());
};

class MockPipeTask : public PipeTask {
public:
    explicit MockPipeTask(std::shared_ptr<TaskListener> listener)
        : PipeTask(std::move(listener), manager){};

    std::shared_ptr<common::Task> next() {
        return PipeTask::getNextTask();
    }

    // Check if the task queue is empty
    bool empty() const {
        return PipeTask::isEmpty();
    }

    // Get the number of tasks in the queue
    size_t count() const {
        return PipeTask::getTaskCount();
    }

private:
    LibraryManager manager;  ///< LibraryManager instance
};

class PipeTaskTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockListener = std::make_shared<MockTaskListener>(nullptr);
        pipeTask = std::make_shared<MockPipeTask>(mockListener);
        mockTask1 = std::make_shared<MockTask>();
        mockTask2 = std::make_shared<MockTask>();
    }

    std::shared_ptr<MockTaskListener> mockListener;
    std::shared_ptr<MockPipeTask> pipeTask;
    std::shared_ptr<MockTask> mockTask1;
    std::shared_ptr<MockTask> mockTask2;
};

TEST_F(PipeTaskTest, AddTask) {
    EXPECT_EQ(pipeTask->count(), 0);

    pipeTask->addTask(mockTask1);
    EXPECT_EQ(pipeTask->count(), 1);

    pipeTask->addTask(mockTask2);
    EXPECT_EQ(pipeTask->count(), 2);

    // Test adding nullptr does nothing
    pipeTask->addTask(nullptr);
    EXPECT_EQ(pipeTask->count(), 2);
}

TEST_F(PipeTaskTest, GetNextTask) {
    pipeTask->addTask(mockTask1);
    pipeTask->addTask(mockTask2);

    auto task = pipeTask->next();
    EXPECT_EQ(task, mockTask1);
    EXPECT_EQ(pipeTask->count(), 1);

    task = pipeTask->next();
    EXPECT_EQ(task, mockTask2);
    EXPECT_EQ(pipeTask->count(), 0);

    // Should return nullptr when empty
    task = pipeTask->next();
    EXPECT_EQ(task, nullptr);
}

TEST_F(PipeTaskTest, IsEmpty) {
    EXPECT_TRUE(pipeTask->empty());

    pipeTask->addTask(mockTask1);
    EXPECT_FALSE(pipeTask->empty());

    pipeTask->next();
    EXPECT_TRUE(pipeTask->empty());
}

TEST_F(PipeTaskTest, OnRunSuccess) {
    pipeTask->addTask(mockTask1);
    pipeTask->addTask(mockTask2);

    EXPECT_CALL(*mockTask1, onRun())
        .WillOnce(::testing::Return(common::TaskResult::TASK_RESULT_SUCCESS));
    EXPECT_CALL(*mockTask2, onRun())
        .WillOnce(::testing::Return(common::TaskResult::TASK_RESULT_SUCCESS));

    auto result = pipeTask->run();
    EXPECT_EQ(result, common::TaskResult::TASK_RESULT_SUCCESS);
}

TEST_F(PipeTaskTest, OnRunFailure) {
    pipeTask->addTask(mockTask1);
    pipeTask->addTask(mockTask2);

    EXPECT_CALL(*mockTask1, onRun())
        .WillOnce(::testing::Return(common::TaskResult::TASK_RESULT_FAILED));
    EXPECT_CALL(*mockTask2, onRun()).Times(0);  // Second task should not be called

    auto result = pipeTask->run();
    EXPECT_EQ(result, common::TaskResult::TASK_RESULT_FAILED);
}

TEST_F(PipeTaskTest, OnRunEmptyQueue) {
    auto result = pipeTask->run();
    EXPECT_EQ(result, common::TaskResult::TASK_RESULT_SUCCESS);
}

TEST_F(PipeTaskTest, OnError) {
    pipeTask->error(500, "Test Error");
    // No-op function, just verify it doesn't crash
}

TEST_F(PipeTaskTest, OnRelease) {
    pipeTask->release();
    // No-op function, just verify it doesn't crash
}

TEST_F(PipeTaskTest, OnCancel) {
    pipeTask->cancel();
    // No-op function, just verify it doesn't crash
}

}  // namespace test
}  // namespace library
}  // namespace nngame
}  // namespace leigod