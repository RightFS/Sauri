/**
 * @file LibraryManagerTest.cpp
 * @brief Unit tests for LibraryManager class
 */

#include "library/libraryManager.h"
#include "library/task/taskListener.h"

#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <random>

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
    MOCK_METHOD(void, setAppId, (int64_t appid), ());
};

class LibraryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 随机生成一个 appId(真随机数)
        std::mt19937 generator(static_cast<unsigned int>(time(NULL)));
        appId = generator() % 1000000 + 100000;

        appId = 100000;

        mockListener = std::make_shared<MockTaskListener>(nullptr);
        validMetadata = R"({
            "appId": )" +
                        std::to_string(appId) + R"(,
            "installer_type": 2,
            "main_executable": "PlantsVsZombies.exe",
            "name": "植物大战僵尸中文版",
            "version": "1.0.0",
            "publisher": "Test Publisher"
        })";
        validOption = R"({
            "install_path": "install",
            "silent": true
        })";
        validScript = R"(
            "InstallScript"
            {
                "Registry"
                {
                    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Test\\PlantsVsZombies"
                    {
                        "string"
                        {
                            "Install_Path"      "%INSTALLDIR%"
                            "Exe_Path"          "%INSTALLDIR%\\植物大战僵尸中文版\\PlantsVsZombies.exe"
                        }

                        "dword"
                        {
                            "PatchVersion"      "1"
                        }
                    }
                }

                "Firewall"
                {
                    "植物大战僵尸中文版" "%INSTALLDIR%\\PlantsVsZombies.exe"
                }
            }
        )";

        std::string configJson = R"({
            "path" : "🐸\\db",
            "encryptionKey" : "",
            "enableAutoBackup" : true,
            "enableCompression" : false
        })";
        try {
            auto config = nlohmann::json::parse(configJson);
            manager.initialize(config);
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize LibraryManager: " << e.what() << std::endl;
        }
    }

    void TearDown() override {
        // 清理工作
        manager.deinitialize();
    }

    LibraryManager manager;
    std::shared_ptr<MockTaskListener> mockListener;
    std::string validMetadata;
    std::string validOption;
    std::string validScript;
    std::string validPath = "🐸\\植物大战僵尸中文版.zip";

    std::promise<void> installPromise;
    std::future<void> installFuture;

    static int64_t appId;
};

int64_t LibraryManagerTest::appId = 100000;

TEST_F(LibraryManagerTest, InstallWithScript) {
    // 重置 future/promise
    installPromise = std::promise<void>();
    installFuture = installPromise.get_future();

    // 添加计数器
    int onSuccessCallCount = 0;

    EXPECT_CALL(*mockListener, onSuccess(::testing::_))
        .Times(::testing::AtLeast(2))
        .WillRepeatedly(::testing::Invoke([this, &onSuccessCallCount](const std::string& message) {
            onSuccessCallCount++;
            if (onSuccessCallCount == 1) {  // 只在第二次调用时设置promise
                installPromise.set_value();
            }
        }));

    EXPECT_CALL(*mockListener, onProgress(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));

    int result = manager.install(validPath, validMetadata, validOption, validScript,
                                 engine::ScriptType::Steam, mockListener);

    EXPECT_EQ(result, 0);

    // 等待安装完成或超时
    EXPECT_EQ(installFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);
}

TEST_F(LibraryManagerTest, InstallWithoutScript) {
    // 重置 future/promise
    installPromise = std::promise<void>();
    installFuture = installPromise.get_future();

    EXPECT_CALL(*mockListener, onProgress(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));
    // 设置当 onSuccess 被调用时完成 promise
    EXPECT_CALL(*mockListener, onSuccess(::testing::_))
        .Times(::testing::AtLeast(1))
        .WillOnce(
            ::testing::Invoke([this](const std::string& message) { installPromise.set_value(); }));

    int result = manager.install(validPath, validMetadata, validOption, mockListener);

    EXPECT_EQ(result, LNG_SUCC);

    // 等待安装完成或超时
    EXPECT_EQ(installFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);
}

TEST_F(LibraryManagerTest, RunScriptWithContent) {
    installPromise = std::promise<void>();
    installFuture = installPromise.get_future();

    EXPECT_CALL(*mockListener, onProgress(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));
    EXPECT_CALL(*mockListener, onSuccess(::testing::_))
        .Times(::testing::AtLeast(1))
        .WillOnce(
            ::testing::Invoke([this](const std::string& message) { installPromise.set_value(); }));
    int result = manager.runScript(appId, validScript, engine::ScriptType::Steam, mockListener);
    EXPECT_EQ(result, LNG_SUCC);

    // 等待安装完成或超时
    EXPECT_EQ(installFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);
}

TEST_F(LibraryManagerTest, RunScriptWithEmptyContent) {
    EXPECT_CALL(*mockListener, onFailure(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));
    int result = manager.runScript(appId, "", engine::ScriptType::Steam, mockListener);
    EXPECT_EQ(result, LNG_ERR_LIBRARY_INVALID_PARAM_SCRIPT);
}

TEST_F(LibraryManagerTest, RunScriptWithAppId) {
    installPromise = std::promise<void>();
    installFuture = installPromise.get_future();

    EXPECT_CALL(*mockListener, onProgress(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));
    EXPECT_CALL(*mockListener, onSuccess(::testing::_))
        .Times(::testing::AtLeast(1))
        .WillOnce(
            ::testing::Invoke([this](const std::string& message) { installPromise.set_value(); }));
    int result = manager.runScript(appId, mockListener);
    EXPECT_EQ(result, LNG_SUCC);
    // 这里可以添加对脚本执行结果的验证
    EXPECT_EQ(installFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);
}

TEST_F(LibraryManagerTest, LaunchGame) {
    // 重置 future/promise
    installPromise = std::promise<void>();
    installFuture = installPromise.get_future();
    int result = manager.lunch(appId, mockListener);
    EXPECT_EQ(result, LNG_SUCC);
    EXPECT_CALL(*mockListener, onSuccess(::testing::_))
        .Times(::testing::AtLeast(1))
        .WillOnce(
            ::testing::Invoke([this](const std::string& message) { installPromise.set_value(); }));

    // 等待安装完成或超时
    EXPECT_EQ(installFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);

    // --------------------关闭游戏--------------------

    // 重置 future/promise
    installPromise = std::promise<void>();
    installFuture = installPromise.get_future();
    
    EXPECT_CALL(*mockListener, onProgress(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));
    EXPECT_CALL(*mockListener, onSuccess(::testing::_))
        .Times(::testing::AtLeast(1))
        .WillOnce(
            ::testing::Invoke([this](const std::string& message) { installPromise.set_value(); }));

    result = manager.terminate(appId, mockListener);

    EXPECT_EQ(result, LNG_SUCC);

    // 等待关闭完成或超时
    EXPECT_EQ(installFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);
}

TEST_F(LibraryManagerTest, UninstallGame) {
    // 重置 future/promise
    installPromise = std::promise<void>();
    installFuture = installPromise.get_future();

    EXPECT_CALL(*mockListener, onProgress(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));
    EXPECT_CALL(*mockListener, onSuccess(::testing::_))
        .Times(::testing::AtLeast(1))
        .WillOnce(::testing::Invoke([this](const std::string& message) {
            [[maybe_unused]] int a = 0;
            installPromise.set_value();
        }));
    int result = manager.uninstall(appId, mockListener);

    EXPECT_EQ(result, LNG_SUCC);

    // 等待关闭完成或超时
    EXPECT_EQ(installFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);
}

}  // namespace test
}  // namespace library
}  // namespace nngame
}  // namespace leigod
