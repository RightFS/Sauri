/**
 * @file ScriptTaskTest.cpp
 * @brief Unit tests for ScriptTask class
 */

#include "library/observer/scriptObserver.h"
#include "library/task/scriptTask.h"
#include "libraryManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace leigod {
namespace nngame {
namespace library {
namespace test {

class MockTaskListener : public TaskListener {
public:
    MockTaskListener(std::shared_ptr<LibraryListener> libraryListener)
        : TaskListener(libraryListener) {}
    MOCK_METHOD(void, onProgress, (int percentage, const std::string& message), (override));
    MOCK_METHOD(void, onSuccess, (const std::string& message), (override));
    MOCK_METHOD(void, onFailure, (int code, const std::string& message), (override));
    MOCK_METHOD(void, setAppId, (const std::string& appId), ());
};

class MockScriptEngine : public engine::IScriptEngine {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, execute,
                (const std::string& script, bool executeAll,
                 const std::shared_ptr<engine::ExecutionObserver>& observer),
                ());
};

class ScriptTaskTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockListener = std::make_shared<MockTaskListener>(nullptr);
        scriptObserver = std::make_shared<ScriptObserver>(mockListener);

        // Create the script task
        taskUnderTest = std::make_shared<ScriptTask>(INVALID_APP_ID, engine::ScriptType::Steam,
                                                     "echo 'test script'", scriptObserver, manager);
    }

    LibraryManager manager;
    std::shared_ptr<MockTaskListener> mockListener;
    std::shared_ptr<ScriptObserver> scriptObserver;
    std::shared_ptr<ScriptTask> taskUnderTest;
};

TEST_F(ScriptTaskTest, GetRetryInterval) {
    auto interval = taskUnderTest->getRetryInterval();
    EXPECT_EQ(interval, std::chrono::milliseconds(0));
}

TEST_F(ScriptTaskTest, OnError) {
    taskUnderTest->error(500, "Test Error");
    // No-op function, just verify it doesn't crash
}

TEST_F(ScriptTaskTest, OnRelease) {
    taskUnderTest->release();
    // No-op function, just verify it doesn't crash
}

TEST_F(ScriptTaskTest, OnCancel) {
    taskUnderTest->cancel();
    // No-op function, just verify it doesn't crash
}

}  // namespace test
}  // namespace library
}  // namespace nngame
}  // namespace leigod
