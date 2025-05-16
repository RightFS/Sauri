#include "systemkit/systemkit.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

using namespace leigod::system_kit;

// 生成唯一的测试键名，避免测试冲突
std::string generateUniqueKeyName() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(10000, 99999);

    return "SystemKitTest_" + std::to_string(distrib(gen));
}

class RegistryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建注册表管理器
        regManager = IComponentFactory::getInstance()->createRegistryManager();
        permHandler = IComponentFactory::getInstance()->createPermissionHandler();

        // 生成唯一的测试键名
        testKeyName = generateUniqueKeyName();

        // 测试路径 - 使用当前用户区域避免权限问题
        testKeyPath = "Software\\SystemKitTest";
        fullTestKeyPath = testKeyPath + "\\" + testKeyName;

        // 检查是否有管理员权限
        ASSERT_NO_THROW(hasAdminRights = permHandler->isRunningAsAdministrator())
            << "Failed to check permission";
    }

    void TearDown() override {
        // 清理测试键
        cleanup();
    }

    // 清理测试创建的键和子键
    void cleanup() {
        // 删除测试键及其所有子键
        //        regManager->deleteKey(RegistryHive::CurrentUser, fullTestKeyPath);
        //        regManager->deleteKey(RegistryHive::CurrentUser, testKeyPath);
        //
        //        // 如果测试了本地机器键，也需要清理（仅当有管理员权限时）
        //        if (hasAdminRights) {
        //            regManager->deleteKey(RegistryHive::LocalMachine, fullTestKeyPath);
        //            regManager->deleteKey(RegistryHive::LocalMachine, testKeyPath);
        //        }
    }

    std::shared_ptr<IRegistryManager> regManager;
    std::shared_ptr<IPermissionHandler> permHandler;
    std::string testKeyName;
    std::string testKeyPath;
    std::string fullTestKeyPath;
    bool hasAdminRights;
};

// 测试创建注册表项
TEST_F(RegistryManagerTest, CreateKey) {
    // 创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create registry key";

    // 验证键是否存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists) << "Registry key should exist after creation";
}

// 测试创建子键
TEST_F(RegistryManagerTest, CreateSubKey) {
    // 先创建父键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create parent key";

    // 创建子键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to create sub key";

    // 验证子键是否存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to check if sub key exists";
    EXPECT_TRUE(keyExists) << "Registry sub key should exist after creation";
}

// 测试删除注册表项
TEST_F(RegistryManagerTest, DeleteKey) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key for deletion";

    // 删除键
    ASSERT_NO_THROW(regManager->deleteKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to delete registry key";

    // 验证键已不存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to check if key exists after deletion";
    EXPECT_FALSE(keyExists) << "Registry key should not exist after deletion";
}

// 测试键是否存在
TEST_F(RegistryManagerTest, KeyExists) {
    // 检查不存在的键
    bool keyExists = true;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to check if key exists";
    EXPECT_FALSE(keyExists) << "Non-existent registry key should not exist";

    // 创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key for existence check";

    // 检查存在的键
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists) << "Created registry key should exist";

    // 删除测试键
    ASSERT_NO_THROW(regManager->deleteKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to delete test key after existence check";
}

// 测试字符串值
TEST_F(RegistryManagerTest, StringValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key";

    // 测试字符串值
    std::string valueName = "StringValue";
    std::string testValue = "Test String Value 测试字符串值";

    // 设置字符串值
    ASSERT_NO_THROW(
        regManager->setString(RegistryHive::CurrentUser, fullTestKeyPath, valueName, testValue))
        << "Failed to set string value";

    // 获取字符串值
    RegistryValue value;
    ASSERT_NO_THROW(value =
                        regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath, valueName))
        << "Failed to get string value";
    EXPECT_EQ(value.asString(), testValue) << "Retrieved string value should match set value";

    // 验证值是否存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists) << "Registry value should exist after setting";

    // 删除测试键
    ASSERT_NO_THROW(regManager->deleteKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to delete test key after setting string value";
}

// 测试DWORD值
TEST_F(RegistryManagerTest, DwordValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key";

    // 测试DWORD值
    std::string valueName = "DwordValue";
    uint32_t testValue = 0x12345678;

    // 设置DWORD值
    ASSERT_NO_THROW(
        regManager->setDWord(RegistryHive::CurrentUser, fullTestKeyPath, valueName, testValue))
        << "Failed to set DWORD value";

    // 获取DWORD值
    RegistryValue value;
    ASSERT_NO_THROW(value =
                        regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath, valueName))
        << "Failed to get DWORD value";
    EXPECT_EQ(value.asDWord(), testValue) << "Retrieved DWORD value should match set value";

    // 验证值是否存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists) << "Registry value should exist after setting";

    // 删除测试键
    ASSERT_NO_THROW(regManager->deleteKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to delete test key after setting DWORD value";
}

// 测试QWORD值
TEST_F(RegistryManagerTest, QwordValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key";

    // 测试QWORD值
    std::string valueName = "QwordValue";
    uint64_t testValue = 0x1234567890ABCDEF;

    // 设置QWORD值
    ASSERT_NO_THROW(
        regManager->setQWord(RegistryHive::CurrentUser, fullTestKeyPath, valueName, testValue))
        << "Failed to set QWORD value";

    // 获取QWORD值
    RegistryValue value;
    ASSERT_NO_THROW(value =
                        regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath, valueName))
        << "Failed to get QWORD value";
    EXPECT_EQ(value.asQWord(), testValue) << "Retrieved QWORD value should match set value";

    // 验证值是否存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists) << "Registry value should exist after setting";

    // 删除测试键
    ASSERT_NO_THROW(regManager->deleteKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to delete test key after setting QWORD value";
}

// 测试扩展字符串值
TEST_F(RegistryManagerTest, ExpandStringValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key";

    // 测试扩展字符串值
    std::string valueName = "ExpandStringValue";
    std::string testValue = "%SystemRoot%\\system32";

    // 设置扩展字符串值
    ASSERT_NO_THROW(regManager->setString(RegistryHive::CurrentUser, fullTestKeyPath, valueName,
                                          testValue, true))
        << "Failed to set expand string value";

    // 获取扩展字符串值
    RegistryValue value;
    ASSERT_NO_THROW(
        value = regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath, valueName, false))
        << "Failed to get expand string value";

    EXPECT_EQ(value.asString(), testValue)
        << "Retrieved expand string value should match set value";

    // 验证值是否存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists) << "Registry value should exist after setting";

    // 删除测试键
    ASSERT_NO_THROW(regManager->deleteKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to delete test key after setting QWORD value";
    std::cout << "Expanded value: " << value.asString() << std::endl;
}

// 测试二进制数据
TEST_F(RegistryManagerTest, BinaryValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key";

    // 测试二进制值
    std::string valueName = "BinaryValue";
    std::vector<uint8_t> testValue = {0x01, 0x02, 0x03, 0x04, 0xFF, 0xFE, 0xFD, 0xFC};

    // 设置二进制值
    ASSERT_NO_THROW(
        regManager->setBinary(RegistryHive::CurrentUser, fullTestKeyPath, valueName, testValue))
        << "Failed to set binary value";

    // 获取二进制值
    RegistryValue value;
    ASSERT_NO_THROW(value =
                        regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath, valueName))
        << "Failed to get binary value";
    //    EXPECT_EQ(value.data(), testValue) << "Retrieved binary value should match set value";
}

// 测试多字符串值

TEST_F(RegistryManagerTest, MultiStringValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key";

    // 测试多字符串值
    std::string valueName = "MultiStringValue";
    std::vector<std::string> testValue = {"First String", "第二个字符串", "Third String",
                                          "第四个字符串"};

    // 设置多字符串值
    ASSERT_NO_THROW(regManager->setMultiString(RegistryHive::CurrentUser, fullTestKeyPath,
                                               valueName, testValue))
        << "Failed to set multi-string value";

    // 获取多字符串值
    RegistryValue value;
    ASSERT_NO_THROW(value =
                        regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath, valueName))
        << "Failed to get multi-string value";

    // 验证多字符串数据
    ASSERT_EQ(value.asMultiString().size(), testValue.size())
        << "Retrieved multi-string should have same size";

    for (size_t i = 0; i < testValue.size(); ++i) {
        EXPECT_EQ(value.asMultiString()[i], testValue[i])
            << "Multi-string element at index " << i << " should match";
    }
}

// 测试删除值

TEST_F(RegistryManagerTest, DeleteValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create test key";

    // 添加一个值
    std::string valueName = "ValueToDelete";
    ASSERT_NO_THROW(
        regManager->setString(RegistryHive::CurrentUser, fullTestKeyPath, valueName, "Test Value"))
        << "Failed to set test value";

    // 确认值存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to check if key exists";
    ASSERT_TRUE(keyExists) << "Value should exist before deletion";

    // 删除值
    ASSERT_NO_THROW(regManager->deleteValue(RegistryHive::CurrentUser, fullTestKeyPath, valueName))
        << "Failed to delete test value";

    // 验证值已不存在
    ASSERT_NO_THROW(
        keyExists = regManager->valueExists(RegistryHive::CurrentUser, fullTestKeyPath, valueName))
        << "Failed to check if deleted value exists";
    EXPECT_FALSE(keyExists) << "Registry value should not exist after deletion";
}

// 测试枚举子键

TEST_F(RegistryManagerTest, EnumerateSubKeys) {
    // 先创建父键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to create parent key";

    // 创建一些子键
    std::vector<std::string> subKeyNames = {"SubKey1", "SubKey2", "SubKey3", "子键4", "子键5"};

    for (const auto& subKeyName : subKeyNames) {
        std::string subKeyPath = testKeyPath + "\\" + subKeyName;
        ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, subKeyPath))
            << "Failed to create sub key: " << subKeyName;
    }

    // 枚举子键
    std::vector<std::string> subKeys;
    ASSERT_NO_THROW(subKeys = regManager->getSubKeys(RegistryHive::CurrentUser, testKeyPath))
        << "Failed to enumerate sub keys";

    // 验证返回的子键列表
    // 检查是否包含我们创建的所有子键（不考虑顺序）
    for (const auto& subKeyName : subKeyNames) {
        bool found = false;
        for (const auto& returnedSubKey : subKeys) {
            if (returnedSubKey == subKeyName) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Sub key '" << subKeyName << "' should be found in enumeration";
    }

    // 输出找到的子键
    std::cout << "Found " << subKeys.size() << " sub keys:" << std::endl;
    for (const auto& subKey : subKeys) {
        std::cout << "  " << subKey << std::endl;
    }
}

// 测试枚举值
TEST_F(RegistryManagerTest, EnumerateValues) {
    // Create the test key
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to create test key";

    // Create some values
    std::vector<std::pair<std::string, std::string>> values = {{"StringValue1", "String value 1"},
                                                               {"StringValue2", "String value 2"},
                                                               {"StringValue3", "String value 3"},
                                                               {"StringValue4", "String value 4"}};

    for (const auto& [name, content] : values) {
        ASSERT_NO_THROW(
            regManager->setString(RegistryHive::CurrentUser, fullTestKeyPath, name, content))
            << "Failed to set value: " << name;
    }

    // Enumerate values
    std::vector<RegistryItem> valueItems;
    ASSERT_NO_THROW(valueItems = regManager->getItems(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to enumerate values";

    // Verify the returned value list
    for (const auto& [name, _] : values) {
        bool found = false;
        for (const auto& returnedValue : valueItems) {
            if (returnedValue.name == name) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Value '" << name << "' should be found in enumeration";
    }

    // Output found values
    std::cout << "Found " << valueItems.size() << " values:" << std::endl;
    for (const auto& valueItem : valueItems) {
        std::cout << "  " << valueItem.name << std::endl;

        // Try to get value type
        RegistryValue value;
        ASSERT_NO_THROW(value = regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath,
                                                     valueItem.name))
            << "Failed to get value: " << valueItem.name;
        std::cout << "    Type: " << static_cast<int>(value.type) << std::endl;
    }
}

// 测试获取值类型
TEST_F(RegistryManagerTest, GetValueType) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to create test key";

    // 创建不同类型的值
    struct TestValue {
        std::string name;
        RegistryValueType expectedType;
    };

    std::vector<TestValue> testValues = {{"StringValue", RegistryValueType::String},
                                         {"DwordValue", RegistryValueType::DWord},
                                         {"QwordValue", RegistryValueType::QWord},
                                         {"ExpandStringValue", RegistryValueType::ExpandString},
                                         {"MultiStringValue", RegistryValueType::MultiString},
                                         {"BinaryValue", RegistryValueType::Binary}};

    // 设置各种类型的值
    regManager->setString(RegistryHive::CurrentUser, fullTestKeyPath, "StringValue", "Test String");
    regManager->setDWord(RegistryHive::CurrentUser, fullTestKeyPath, "DwordValue", 123456);
    regManager->setQWord(RegistryHive::CurrentUser, fullTestKeyPath, "QwordValue",
                         1234567890123456789);
    regManager->setString(RegistryHive::CurrentUser, fullTestKeyPath, "ExpandStringValue", "%TEMP%",
                          true);
    regManager->setMultiString(RegistryHive::CurrentUser, fullTestKeyPath, "MultiStringValue",
                               {"String1", "String2"});
    regManager->setBinary(RegistryHive::CurrentUser, fullTestKeyPath, "BinaryValue",
                          {0x01, 0x02, 0x03, 0x04});

    // 验证各个值的类型
    for (const auto& testValue : testValues) {
        RegistryValue value;
        ASSERT_NO_THROW(value = regManager->getValue(RegistryHive::CurrentUser, fullTestKeyPath,
                                                     testValue.name))
            << "Failed to get value: " << testValue.name;
        EXPECT_EQ(value.type, testValue.expectedType)
            << "Value '" << testValue.name << "' should have expected type";
    }
}

// 测试管理员权限功能 - 修改 HKLM

TEST_F(RegistryManagerTest, LocalMachineRegistry) {
    // 跳过如果没有管理员权限
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // 在 HKEY_LOCAL_MACHINE 中创建键
    std::string lmKeyPath = "SOFTWARE\\SystemKitTest_" + generateUniqueKeyName();
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::LocalMachine, lmKeyPath))
        << "Failed to create key in HKLM";

    // 设置字符串值
    std::string valueName = "AdminTest";
    std::string testValue = "Value requiring admin rights";
    ASSERT_NO_THROW(
        regManager->setString(RegistryHive::LocalMachine, lmKeyPath, valueName, testValue))
        << "Failed to set string value in HKLM";

    // 获取字符串值
    RegistryValue value;
    ASSERT_NO_THROW(value = regManager->getValue(RegistryHive::LocalMachine, lmKeyPath, valueName))
        << "Failed to get string value from HKLM";
    EXPECT_EQ(value.asString(), testValue) << "Value in HKLM should match set value";

    // 清理 - 删除测试键
    regManager->deleteKey(RegistryHive::LocalMachine, lmKeyPath);
}

// 测试错误情况 - 非法键路径
TEST_F(RegistryManagerTest, InvalidKeyPath) {
    // 尝试使用无效键路径
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, "InvalidKeyPath"))
        << "Failed to check if key exists with invalid path";
    EXPECT_FALSE(keyExists) << "Operation with invalid path should fail";
}

// 测试错误情况 - 访问不存在的值
TEST_F(RegistryManagerTest, NonExistentValue) {
    // 先创建测试键
    ASSERT_NO_THROW(regManager->createKey(RegistryHive::CurrentUser, fullTestKeyPath))
        << "Failed to create test key";

    // 尝试获取不存在的值
    bool valueExists = false;
    ASSERT_NO_THROW(valueExists = regManager->valueExists(RegistryHive::CurrentUser,
                                                          fullTestKeyPath, "NonExistentValue"))
        << "Failed to check if value exists";
}

/**
 * @brief Test for parsePath function
 */
TEST_F(RegistryManagerTest, ParsePath) {
    // 测试有效路径
    std::pair<RegistryHive, std::string> expected;
    ASSERT_NO_THROW(expected = regManager->parsePath("HKEY_CURRENT_USER\\Software\\LeigodTest"))
        << "Failed to parse valid path";
    EXPECT_EQ(expected.first, RegistryHive::CurrentUser);
    EXPECT_EQ(expected.second, "Software\\LeigodTest");

    // 测试其他根键
    ASSERT_NO_THROW(expected =
                        regManager->parsePath("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet"))
        << "Failed to parse valid path";
    EXPECT_EQ(expected.first, RegistryHive::LocalMachine);
    EXPECT_EQ(expected.second, "SYSTEM\\CurrentControlSet");

    // 测试无效根键
    ASSERT_THROW(expected = regManager->parsePath("INVALID_ROOT_KEY\\Software"),
                 RegistryManagerException)
        << "Failed to parse valid path";

    // 测试缺少分隔符的路径
    ASSERT_THROW(expected = regManager->parsePath("HKEY_CURRENT_USERSoftware\\LeigodTest"),
                 RegistryManagerException)
        << "Failed to parse valid path";

    // 测试空路径
    ASSERT_THROW(expected = regManager->parsePath(""), RegistryManagerException)
        << "Failed to parse valid path";
}

/**
 * @brief Test for automatic creation of registry keys
 */
TEST_F(RegistryManagerTest, AutoCreateKey) {
    // 深层次路径，不存在的键
    std::string deepPath = "Software\\LeigodTest\\Level1\\Level2\\Level3\\Level4";
    std::string valueName = "TestValue";
    std::string valueData = "TestData";

    // 1. 确认深层路径不存在
    bool keyExists = false;
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, deepPath,
                                                      RegistryView::Default))
        << "Failed to check if key exists";
    EXPECT_FALSE(keyExists);

    // 2. 设置深层路径的值，应自动创建所有键
    ASSERT_NO_THROW(regManager->setString(RegistryHive::CurrentUser, deepPath, valueName, valueData,
                                          false, RegistryView::Default))
        << "Failed to create registry key";

    // 3. 确认深层路径现在存在
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, deepPath,
                                                      RegistryView::Default))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists);

    // 4. 确认值正确设置
    RegistryValue value;
    ASSERT_NO_THROW(value = regManager->getValue(RegistryHive::CurrentUser, deepPath, valueName,
                                                 false, RegistryView::Default))
        << "Failed to get registry value";
    EXPECT_EQ(value.asString(), valueData);

    // 5. 测试中间路径也被成功创建
    std::string middlePath = "Software\\LeigodTest\\Level1\\Level2";
    ASSERT_NO_THROW(keyExists = regManager->keyExists(RegistryHive::CurrentUser, middlePath,
                                                      RegistryView::Default))
        << "Failed to check if key exists";
    EXPECT_TRUE(keyExists);

    // 6. 测试设置其他类型值时的自动创建功能
    std::string dwordPath = "Software\\LeigodTest\\Config\\Settings\\Performance";
    std::string dwordName = "MaxThreads";
    uint32_t dwordValue = 16;
    ASSERT_NO_THROW(regManager->setDWord(RegistryHive::CurrentUser, dwordPath, dwordName,
                                         dwordValue, RegistryView::Default))
        << "Failed to create registry key";

    ASSERT_NO_THROW(value = regManager->getValue(RegistryHive::CurrentUser, dwordPath, dwordName,
                                                 false, RegistryView::Default))
        << "Failed to get registry value";
    EXPECT_EQ(value.asDWord(), dwordValue);

    // 7. 边缘情况：非常长的路径
    std::string veryLongPath =
        "Software\\LeigodTest\\A\\B\\C\\D\\E\\F\\G\\H\\I\\J\\K\\L\\M\\N\\O\\P";
    ASSERT_NO_THROW(regManager->setString(RegistryHive::CurrentUser, veryLongPath, "LongPathTest",
                                          "Success", false, RegistryView::Default))
        << "Failed to create registry key";

    // 8. 清理所有创建的测试键（只需删除顶级键）
    regManager->deleteKey(RegistryHive::CurrentUser, "Software\\LeigodTest", RegistryView::Default);
}

// 增强版本测试：处理特殊字符路径
TEST_F(RegistryManagerTest, AutoCreateKeyWithSpecialChars) {
    // 包含空格和特殊字符的路径
    std::string specialPath = "Software\\LeigodTest\\Special Characters\\Path With Spaces\\#$&@!";
    std::string valueName = "Special!@#Value";
    std::string valueData = "Data with spaces and !@#$%^&*()";

    // 设置值应成功
    ASSERT_NO_THROW(regManager->setString(RegistryHive::CurrentUser, specialPath, valueName,
                                          valueData, false, RegistryView::Default))
        << "Failed to create path with special characters";

    // 确认值正确设置
    RegistryValue value;
    ASSERT_NO_THROW(value = regManager->getValue(RegistryHive::CurrentUser, specialPath, valueName,
                                                 false, RegistryView::Default))
        << "Failed to get registry value with special characters";
    EXPECT_EQ(value.asString(), valueData);

    // 清理
    regManager->deleteKey(RegistryHive::CurrentUser, "Software\\LeigodTest", RegistryView::Default);
}

// 性能测试：创建深度路径
TEST_F(RegistryManagerTest, PerformanceDeepPaths) {
    // 创建非常深层次的路径结构
    const int DEPTH = 20;  // 创建20层路径
    std::string basePath = "Software\\LeigodTest";
    std::string fullPath = basePath;

    for (int i = 1; i <= DEPTH; i++) {
        fullPath += "\\Level" + std::to_string(i);
    }

    // 记录开始时间
    auto startTime = std::chrono::high_resolution_clock::now();

    // 设置值，应自动创建所有键
    ASSERT_NO_THROW(regManager->setString(RegistryHive::CurrentUser, fullPath, "PerfTest",
                                          "TestValue", false, RegistryView::Default))
        << "Failed to create deep path";

    // 计算耗时
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "Time to create " << DEPTH << " level deep path: " << duration << "ms"
              << std::endl;

    // 清理（注意：Windows注册表API可能会限制键的深度）
    regManager->deleteKey(RegistryHive::CurrentUser, "Software\\LeigodTest", RegistryView::Default);
}

// 错误处理测试
TEST_F(RegistryManagerTest, ErrorHandlingInPathCreation) {
    // 测试访问受限制的路径（需要管理员权限）
    std::string restrictedPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File "
                                 "Execution Options\\RestrictedApp";
    ASSERT_NO_THROW(regManager->setString(RegistryHive::LocalMachine, restrictedPath, "TestValue",
                                          "Data", false, RegistryView::Default))
        << "Failed to set value";
    try {
        regManager->setString(RegistryHive::LocalMachine, restrictedPath, "TestValue", "Data",
                              false, RegistryView::Default);
    } catch (const RegistryManagerException& e) {
        EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::RegistryAccessDenied));
        return;
    }

    // 如果测试作为管理员运行，清理创建的键
    regManager->deleteKey(RegistryHive::LocalMachine, restrictedPath, RegistryView::Default);
}