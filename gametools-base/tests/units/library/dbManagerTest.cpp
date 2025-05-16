/**
 * @file dbManagerTest.cpp
 * @brief DbManager单元测试
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod Library Manager.
 * Created: 2025-04-15 09:00:00 (UTC)
 * Author: chenxu
 */

#include "db/dbManager.h"

#include <filesystem>
#include <fstream>
#include <future>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <thread>

namespace leigod {
namespace nngame {
namespace library {
namespace db {
namespace test {

// 测试辅助类，暴露一些内部方法便于测试
class TestableDbManager : public DbManager {
public:
    bool callValidateDatabasePath(const std::string& path) {
        return validateDatabasePath(path);
    }

    bool callCreateTables() {
        return createTables();
    }

    void callSetupEncryption(const std::string& key) {
        setupEncryption(key);
    }
};

// 创建测试用的配置
nlohmann::json createTestConfig(const std::string& path) {
    nlohmann::json config;
    config["path"] = path;
    config["encryptionKey"] = "test_key";
    config["enableAutoBackup"] = true;
    config["enableSecureDelete"] = true;
    config["enableCompression"] = true;
    config["compressFields"] = {"custom_attributes", "dependencies"};
    return config;
}

class DbManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        tempDir_ = std::filesystem::temp_directory_path() / "leigod_db_test";
        std::filesystem::create_directories(tempDir_);

        // 初始化测试对象
        dbManager_ = std::make_unique<TestableDbManager>();
        testConfig_ = createTestConfig(tempDir_.string());
    }

    void TearDown() override {
        // 清理资源
        if (dbManager_->isInitialized()) {
            dbManager_->deinitialize();
        }

        // 删除临时目录
        try {
            std::filesystem::remove_all(tempDir_);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "清理临时目录失败: " << e.what() << std::endl;
        }
    }

    // 创建测试用元数据
    LibraryDbMetaData createTestMetaData(int64_t appId) {
        LibraryDbMetaData metaData;
        metaData.isAutoIncrement = true;
        metaData.appId = appId;
        metaData.name = "测试应用" + std::to_string(appId);
        metaData.version = "1.0.0";
        metaData.publisher = "测试发布者";
        metaData.description = "这是一个测试应用";
        metaData.installer_type = installer::InstallerType::kArchive;
        metaData.main_executable = "app.exe";
        metaData.minimum_os_version = "10.0";
        metaData.target_architecture = "x64";
        metaData.silent = true;
        metaData.requires_admin = false;
        metaData.custom_attributes = {{"key", "value"}};
        metaData.install_path = "TestApp";
        return metaData;
    }

    // 创建测试用脚本
    LibraryDbScript createTestScript(int64_t appId) {
        LibraryDbScript script;
        script.isAutoIncrement = true;
        script.appId = appId;
        script.script = "console.log('安装成功');";
        script.type = engine::ScriptType::Steam;
        return script;
    }

    std::filesystem::path tempDir_;
    std::unique_ptr<TestableDbManager> dbManager_;
    nlohmann::json testConfig_;
};

// 测试初始化方法
TEST_F(DbManagerTest, Initialize) {
    EXPECT_TRUE(dbManager_->initialize(testConfig_));
    EXPECT_TRUE(dbManager_->isInitialized());
    EXPECT_FALSE(dbManager_->getDbPath().empty());

    // 数据库文件应该存在
    std::filesystem::path dbFilePath = tempDir_ / "nngame-library.db";
    EXPECT_TRUE(std::filesystem::exists(dbFilePath));

    // 重复初始化应该失败
    EXPECT_FALSE(dbManager_->initialize(testConfig_));
}

// 测试数据库路径验证
TEST_F(DbManagerTest, ValidateDatabasePath) {
    // 有效路径
    EXPECT_TRUE(dbManager_->callValidateDatabasePath(tempDir_.string()));

    // 无效路径（假设没有权限创建此目录）
    std::string invalidPath = "L:\\Windows\\168n";
    EXPECT_FALSE(dbManager_->callValidateDatabasePath(invalidPath));
}

// 测试元数据和脚本插入
TEST_F(DbManagerTest, InsertMetaDataAndScript) {
    ASSERT_TRUE(dbManager_->initialize(testConfig_));

    LibraryDbMetaData metaData = createTestMetaData(1);
    LibraryDbScript script = createTestScript(1);

    EXPECT_TRUE(dbManager_->insertMetaDataAndScript(metaData, script));

    // 验证插入后是否可以查询到
    auto metaDataManager = dbManager_->getMetaDataManager();
    auto appsOp = metaDataManager->getAppsByName(metaData.name);
    ASSERT_TRUE(appsOp.succeed());
    auto apps = appsOp.value();
    EXPECT_EQ(apps.size(), 1);
    EXPECT_EQ(apps[0].appId, 1);
    EXPECT_EQ(apps[0].name, metaData.name);
}

// 测试异步插入
TEST_F(DbManagerTest, AsyncInsertMetaDataAndScript) {
    ASSERT_TRUE(dbManager_->initialize(testConfig_));

    LibraryDbMetaData metaData = createTestMetaData(2);
    LibraryDbScript script = createTestScript(2);

    std::promise<bool> resultPromise;
    auto resultFuture = resultPromise.get_future();

    EXPECT_TRUE(dbManager_->asyncInsertMetaDataAndScript(
        metaData, script, [&](bool result) { resultPromise.set_value(result); }));

    EXPECT_EQ(resultFuture.wait_for(std::chrono::seconds(30)), std::future_status::ready);

    // 等待异步任务完成
    bool result = resultFuture.get();
    EXPECT_TRUE(result) << "异步插入失败 " << dbManager_->getDatabase()->getError().getMessage();

    // 验证插入结果
    auto metaDataManager = dbManager_->getMetaDataManager();
    auto appsOp = metaDataManager->getAppsByName(metaData.name);
    ASSERT_TRUE(appsOp.succeed());
    auto apps = appsOp.value();
    EXPECT_EQ(apps.size(), 1);
    EXPECT_EQ(apps[0].appId, 2);
}

// 测试删除应用
TEST_F(DbManagerTest, DeleteApp) {
    ASSERT_TRUE(dbManager_->initialize(testConfig_));

    // 先插入数据
    LibraryDbMetaData metaData = createTestMetaData(3);
    LibraryDbScript script = createTestScript(3);

    ASSERT_TRUE(dbManager_->insertMetaDataAndScript(metaData, script));

    // 删除应用
    EXPECT_TRUE(dbManager_->deleteApp(3));

    // 验证删除结果
    auto metaDataManager = dbManager_->getMetaDataManager();
    auto appsOp = metaDataManager->getAppsByName(metaData.name);
    ASSERT_FALSE(appsOp.succeed() && appsOp.value().size() > 0)
        << "删除应用后仍然可以查询到应用 " << dbManager_->getDatabase()->getError().getMessage();
}

// 测试批量操作
TEST_F(DbManagerTest, BatchOperations) {
    ASSERT_TRUE(dbManager_->initialize(testConfig_)) << "初始化失败";

    // 批量插入
    std::vector<std::pair<LibraryDbMetaData, LibraryDbScript>> apps;
    for (int64_t i = 10; i < 15; i++) {
        apps.emplace_back(createTestMetaData(i), createTestScript(i));
    }

    EXPECT_TRUE(dbManager_->batchInsert(apps))
        << "批量插入失败 " << dbManager_->getDatabase()->getError().getMessage();

    // 验证批量插入结果
    auto metaDataManager = dbManager_->getMetaDataManager();
    EXPECT_EQ(metaDataManager->getAppCount(), 5)
        << "批量插入后应用数量不正确" << dbManager_->getDatabase()->getError().getMessage();

    // 批量删除
    std::vector<int64_t> appIds = {10, 12, 14};
    EXPECT_TRUE(dbManager_->batchDelete(appIds))
        << "批量删除失败 " << dbManager_->getDatabase()->getError().getMessage();

    // 验证批量删除结果
    EXPECT_EQ(metaDataManager->getAppCount(), 2)
        << "批量删除后应用数量不正确" << dbManager_->getDatabase()->getError().getMessage();
}

// 测试数据库维护操作
TEST_F(DbManagerTest, DatabaseMaintenance) {
    ASSERT_TRUE(dbManager_->initialize(testConfig_));

    // 备份测试
    std::string backupPath = (tempDir_ / "backup.db").string();
    EXPECT_TRUE(dbManager_->backupDatabase(backupPath));

    // 清理测试
    EXPECT_TRUE(dbManager_->vacuumDatabase());

    // 恢复测试通常需要先创建备份文件，此处只测试方法返回结果
    EXPECT_TRUE(dbManager_->restoreDatabase(backupPath));
}

// 测试事务支持
TEST_F(DbManagerTest, TransactionSupport) {
    ASSERT_TRUE(dbManager_->initialize(testConfig_));

    // 在事务中执行多个操作
    bool result = dbManager_->runInTransaction([&](WCDB::Handle& handle) {
        // 执行多个操作
        LibraryDbMetaData metaData1 = createTestMetaData(100);
        LibraryDbScript script1 = createTestScript(100);

        LibraryDbMetaData metaData2 = createTestMetaData(101);
        LibraryDbScript script2 = createTestScript(101);

        bool success = dbManager_->insertMetaDataAndScript(metaData1, script1);
        success &= dbManager_->insertMetaDataAndScript(metaData2, script2);

        return success;
    });

    EXPECT_TRUE(result);

    // 验证事务中的操作是否全部成功
    auto metaDataManager = dbManager_->getMetaDataManager();
    EXPECT_EQ(metaDataManager->getAppCount(), 2);
}

}  // namespace test
}  // namespace db
}  // namespace library
}  // namespace nngame
}  // namespace leigod
