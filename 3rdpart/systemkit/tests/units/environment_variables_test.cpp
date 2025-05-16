#include "systemkit/systemkit.hpp"

#include <algorithm>
#include <cstdlib>
#include <gtest/gtest.h>
#include <string>

using namespace leigod::system_kit;

class EnvironmentVariablesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建环境变量管理器
        envManager = IComponentFactory::getInstance()->createEnvironmentVariables();

        // 创建权限处理器
        permHandler = IComponentFactory::getInstance()->createPermissionHandler();

        // 保存测试前的环境变量状态，便于后续清理
        testVarName = "SYSTEMKIT_TEST_VAR";
        testVarValue = "test_value_123";
        systemTestVarName = "SYSTEMKIT_SYSTEM_TEST_VAR";
        systemTestVarValue = "system_test_value_456";

        // 清理可能存在的测试环境变量
        envManager->remove(testVarName);
        envManager->remove(systemTestVarName);
    }

    void TearDown() override {
        // 清理测试环境变量
        envManager->remove(testVarName);
        envManager->remove(systemTestVarName);
    }

    // 检查字符串中是否包含特定子串，忽略大小写
    bool containsIgnoreCase(const std::string& str, const std::string& substr) {
        std::string lowerStr = str;
        std::string lowerSubstr = substr;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        std::transform(lowerSubstr.begin(), lowerSubstr.end(), lowerSubstr.begin(), ::tolower);
        return lowerStr.find(lowerSubstr) != std::string::npos;
    }

    std::shared_ptr<IEnvironmentVariables> envManager;
    std::shared_ptr<IPermissionHandler> permHandler;
    std::string testVarName;
    std::string testVarValue;
    std::string systemTestVarName;
    std::string systemTestVarValue;
};

// 基本功能测试

TEST_F(EnvironmentVariablesTest, SetAndGetLocalVariable) {
    // 设置本地用户级环境变量
    ASSERT_NO_THROW(envManager->set(testVarName, testVarValue, EnvVarScope::User))
        << "Failed to set user environment variable";

    // 获取环境变量
    std::string value;
    ASSERT_NO_THROW(value = envManager->get(testVarName, EnvVarScope::User))
        << "Failed to get user environment variable";

    // 验证值
    EXPECT_EQ(value, testVarValue);
}

TEST_F(EnvironmentVariablesTest, CheckVariableExists) {
    // 初始应该不存在
    bool exists = false;
    ASSERT_NO_THROW(exists = envManager->exists(testVarName, EnvVarScope::Process))
        << "Failed to check existence of environment variable";
    EXPECT_FALSE(exists);

    // 设置后应该存在
    ASSERT_NO_THROW(envManager->set(testVarName, testVarValue, EnvVarScope::Process))
        << "Failed to set environment variable";

    ASSERT_NO_THROW(exists = envManager->exists(testVarName, EnvVarScope::Process))
        << "Failed to check existence of environment variable";
    EXPECT_TRUE(exists);

    // 移除后不应该存在
    ASSERT_NO_THROW(envManager->remove(testVarName, EnvVarScope::Process))
        << "Failed to remove environment variable";
    ASSERT_NO_THROW(exists = envManager->exists(testVarName, EnvVarScope::Process))
        << "Failed to check existence of environment variable";
    EXPECT_FALSE(exists);
}

TEST_F(EnvironmentVariablesTest, GetAllEnvironmentVariables) {
    // 设置一个测试变量确保至少有一个已知变量
    ASSERT_NO_THROW(envManager->set(testVarName, testVarValue, EnvVarScope::Process))
        << "Failed to set environment variable";

    // 获取所有环境变量
    std::string value;
    ASSERT_NO_THROW(value = envManager->get(testVarName, EnvVarScope::Process))
        << "Failed to get environment variable";

    std::map<std::string, std::string> allVars;
    ASSERT_NO_THROW(allVars = envManager->getAll(EnvVarScope::Process))
        << "Failed to get all environment variables";
    // 检查结果不为空且包含我们设置的变量
    EXPECT_FALSE(allVars.empty());

    // 查找测试变量
    bool foundTestVar = false;
    std::string foundValue;

    for (const auto& var : allVars) {
        if (var.first == testVarName) {
            foundTestVar = true;
            foundValue = var.second;
            break;
        }
    }

    EXPECT_TRUE(foundTestVar) << "Test variable not found in environment";
    if (foundTestVar) {
        EXPECT_EQ(foundValue, testVarValue);
    }

    // 常见系统变量检查
    bool foundSystemVar = false;
    for (const auto& var : allVars) {
        if (containsIgnoreCase(var.first, "PATH") || containsIgnoreCase(var.first, "HOME") ||
            containsIgnoreCase(var.first, "USER")) {
            foundSystemVar = true;
            break;
        }
    }

    EXPECT_TRUE(foundSystemVar) << "No common system variables found";
}

TEST_F(EnvironmentVariablesTest, RemoveEnvironmentVariable) {
    // 先设置变量
    ASSERT_NO_THROW(envManager->set(testVarName, testVarValue, EnvVarScope::Process))
        << "Failed to set environment variable";

    // 确认变量已设置
    bool exists = false;
    ASSERT_NO_THROW(exists = envManager->exists(testVarName, EnvVarScope::Process))
        << "Failed to check existence of environment variable";
    ASSERT_TRUE(exists);

    // 移除变量
    ASSERT_NO_THROW(envManager->remove(testVarName, EnvVarScope::Process))
        << "Failed to remove environment variable";

    // 确认变量已移除
    ASSERT_NO_THROW(exists = envManager->exists(testVarName, EnvVarScope::Process))
        << "Failed to check existence of environment variable";
    EXPECT_FALSE(exists);
}

TEST_F(EnvironmentVariablesTest, ExpandEnvironmentVariables) {
    // 设置测试变量
    envManager->set(testVarName, testVarValue, EnvVarScope::Process);

    // 构造包含环境变量引用的字符串
#ifdef _WIN32
    std::string testString = "Value is %SYSTEMKIT_TEST_VAR%";
#else
    std::string testString = "Value is $SYSTEMKIT_TEST_VAR";
#endif

    // 展开变量
    std::string value;
    ASSERT_NO_THROW(value = envManager->expand(testString))
        << "Failed to expand environment variables";

    // 验证结果
    EXPECT_EQ(value, "Value is " + testVarValue);
}

// 边缘情况测试
TEST_F(EnvironmentVariablesTest, HandleEmptyVariableName) {
    // 尝试获取名称为空的环境变量
    ASSERT_THROW(
        {
            try {
                envManager->get("", EnvVarScope::Process);
            } catch (const EnvironmentVariableException& e) {
                EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::InvalidArgument));
                throw;
            }
        },
        EnvironmentVariableException)
        << "Failed to set environment variable";

    // 尝试设置名称为空的环境变量
    ASSERT_THROW(
        {
            try {
                envManager->set("", testVarValue, EnvVarScope::Process);
            } catch (const EnvironmentVariableException& e) {
                EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::InvalidArgument));
                throw;
            }
        },
        EnvironmentVariableException)
        << "Failed to set environment variable";
}

TEST_F(EnvironmentVariablesTest, HandleNonExistentVariable) {
    // 尝试获取不存在的环境变量
    ASSERT_THROW(
        {
            try {
                std::string nonExistentVar =
                    "SYSTEMKIT_NONEXISTENT_VAR_" + std::to_string(std::rand());
                envManager->get(nonExistentVar, EnvVarScope::Process);
            } catch (const EnvironmentVariableException& e) {
                EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::EnvironmentVariableNotFound));
                throw;
            }
        },
        EnvironmentVariableException)
        << "Failed to set environment variable";
}

TEST_F(EnvironmentVariablesTest, HandleSpecialCharacters) {
    // 测试包含特殊字符的环境变量
    std::string specialVarName = "SYSTEMKIT_SPECIAL_VAR";
    std::string specialVarValue = "Value with spaces, 特殊字符, and $ymbols!";

    // 设置变量
    ASSERT_NO_THROW(envManager->set(specialVarName, specialVarValue, EnvVarScope::Process))
        << "Failed to set environment variable";

    // 获取变量
    std::string value;
    ASSERT_NO_THROW(value = envManager->get(specialVarName, EnvVarScope::Process))
        << "Failed to get environment variable";

    // 验证值完全匹配
    EXPECT_EQ(value, specialVarValue);

    // 清理
    ASSERT_NO_THROW(envManager->remove(specialVarName, EnvVarScope::Process))
        << "Failed to remove environment variable";
}

// 系统范围环境变量测试 (需要提升权限)
TEST_F(EnvironmentVariablesTest, SystemVariableRequiresElevation) {
    // 检查是否有管理员权限
    bool isAdmin = false;
    ASSERT_NO_THROW(isAdmin = permHandler->isRunningAsAdministrator())
        << "Failed to check administrator status";

    // 记录当前权限状态
    std::cout << "Current process is " << (isAdmin ? "running" : "not running")
              << " with administrator privileges." << std::endl;

    if (!isAdmin) {
        // 输出测试说明
        std::cout << "NOTE: To fully test system environment variables, run this test with "
                     "administrator privileges."
                  << std::endl;
        return;
    }

    // 尝试设置系统环境变量
    ASSERT_NO_THROW(envManager->set(systemTestVarName, systemTestVarValue, EnvVarScope::System))
        << "Failed to set system environment variable";

    // 验证变量已设置
    std::string value;
    ASSERT_NO_THROW(value = envManager->get(systemTestVarName, EnvVarScope::System))
        << "Failed to get system environment variable";
    EXPECT_EQ(value, systemTestVarValue);

    // 清理（确保运行）
    ASSERT_NO_THROW(envManager->remove(systemTestVarName, EnvVarScope::System))
        << "Failed to remove system environment variable";
}

// 权限提升功能测试
TEST_F(EnvironmentVariablesTest, CheckPermissionElevation) {
    // 检查权限提升功能
    bool isAdmin = false;
    ASSERT_NO_THROW(isAdmin = permHandler->isRunningAsAdministrator())
        << "Failed to check administrator status";

    // 记录当前权限状态
    std::cout << "Current process is " << (isAdmin ? "running" : "not running")
              << " with administrator privileges." << std::endl;

    // 测试调整进程权限功能（如果支持）
    if (isAdmin) {
        // 管理员应该能够调整权限
        bool isPrivilegeEnabled = false;
        ASSERT_NO_THROW(isPrivilegeEnabled = permHandler->adjustPrivilege("SeDebugPrivilege", true))
            << "Failed to check privilege status";

        EXPECT_TRUE(isPrivilegeEnabled)
            << "Failed to enable SeDebugPrivilege despite having admin rights";

        // 测试完成后禁用
        permHandler->adjustPrivilege("SeDebugPrivilege", false);
    } else {
        // 非管理员可能无法调整权限
        std::cout << "NOTE: Adjusting privileges requires administrator rights as expected."
                  << std::endl;
    }
}

// 测试无效UTF-8序列
TEST_F(EnvironmentVariablesTest, InvalidUTF8Sequences) {
    // 创建明确无效的UTF-8序列
    std::vector<std::string> invalidSequences = {
        std::string("\xFE\xFF\xFE"),      // 非 UTF-8 字节序标记加无效字节
        std::string("\xFF\xFF\xFF\xFF"),  // 完全无效的 UTF-8 序列
    };

    for (const auto& seq : invalidSequences) {
        // 测试设置无效字符串 - 可能抛出异常但不要求必须抛出
        try {
            envManager->set(testVarName, seq, EnvVarScope::Process);

            // 如果设置成功，需要清理
            envManager->remove(testVarName, EnvVarScope::Process);
        } catch (const std::exception& e) {
            // 捕获异常但不做断言 - 可能是 std::engine 或其派生类
            std::cout << "设置无效 UTF-8 序列时出现异常: " << e.what() << std::endl;
        }
    }
}

// 测试嵌入式空字符
TEST_F(EnvironmentVariablesTest, EmbeddedNullCharacters) {
    // 创建包含空字符的字符串
    std::string withNulls = "Test\0String\0With\0Nulls";
    withNulls.resize(22);  // 确保包含所有空字符

    ASSERT_NO_THROW({
        // 尝试设置带空字符的环境变量
        envManager->set(testVarName, withNulls, EnvVarScope::Process);

        // 获取并验证
        std::string result = envManager->get(testVarName, EnvVarScope::Process);

        // Windows环境变量API会在首个空字符处截断字符串
        EXPECT_EQ(result, std::string("Test"));

        // 清理
        envManager->remove(testVarName, EnvVarScope::Process);
    });
}

// 测试过长变量名
TEST_F(EnvironmentVariablesTest, ExcessivelyLongVariableName) {
    // 创建一个非常长的变量名 - Windows 限制环境变量名在 32767 字符以内
    // 但实际可能更小，我们测试一个适中的长度
    std::string longName(260, 'A');  // 260 是 MAX_PATH 值

    try {
        // 尝试设置较长的名称 - 可能成功或失败
        envManager->set(longName, "test", EnvVarScope::Process);
        // 如果成功，需要清理
        envManager->remove(longName, EnvVarScope::Process);
    } catch (const EnvironmentVariableException& e) {
        // 捕获异常但不断言必须抛出
        std::cout << "设置过长变量名时出现异常: " << e.what() << std::endl;
    }
}

// 测试格式错误的环境变量展开
TEST_F(EnvironmentVariablesTest, MalformedVariableExpansion) {
    // 创建各种格式错误的环境变量引用
    std::vector<std::string> malformedRefs = {
        "%PATH",          // 未闭合的引用
        "%%",             // 空引用
        "%NonExistent%",  // 不存在的变量
        "%PA%TH%",        // 嵌套错误
        "%PATH%%PATH"     // 连续引用错误
    };

    for (const auto& ref : malformedRefs) {
        // 展开应该不会崩溃，但可能不会如预期展开
        ASSERT_NO_THROW(envManager->expand(ref));
    }
}

// 测试系统环境变量无权限访问
TEST_F(EnvironmentVariablesTest, SystemVariablePermissionDenied) {
    // 检查是否有管理员权限
    bool isAdmin = false;
    ASSERT_NO_THROW(isAdmin = permHandler->isRunningAsAdministrator());

    if (isAdmin) {
        // 已有管理员权限时跳过测试
        std::cout << "跳过权限拒绝测试：当前已是管理员权限" << std::endl;
        return;
    }

    // 尝试在没有管理员权限的情况下设置系统环境变量
    ASSERT_THROW(envManager->set(systemTestVarName, systemTestVarValue, EnvVarScope::System),
                 EnvironmentVariableException);
}

// 测试无效的枚举值作为作用域
TEST_F(EnvironmentVariablesTest, InvalidScopeEnum) {
    // 创建一个无效的作用域枚举值
    EnvVarScope invalidScope = static_cast<EnvVarScope>(999);

    // 测试各种操作使用无效作用域时的行为
    ASSERT_THROW(envManager->get(testVarName, invalidScope), EnvironmentVariableException);

    ASSERT_THROW(envManager->set(testVarName, testVarValue, invalidScope),
                 EnvironmentVariableException);

    ASSERT_THROW(envManager->remove(testVarName, invalidScope), EnvironmentVariableException);

    ASSERT_THROW(envManager->exists(testVarName, invalidScope), EnvironmentVariableException);

    ASSERT_THROW(envManager->getAll(invalidScope), EnvironmentVariableException);
}

// 测试混合字符编码
TEST_F(EnvironmentVariablesTest, MixedEncodingTest) {
    // 创建混合了有效和无效UTF-8的字符串
    std::string mixedEncoding = "Valid UTF-8 " + std::string("\xE4\xB8\xAD") +  // 中文字符"中"
                                " mixed with invalid " +
                                std::string("\xE4\xB8");  // 不完整的UTF-8序列

    try {
        // 尝试设置混合编码字符串 - 可能成功也可能失败
        envManager->set(testVarName, mixedEncoding, EnvVarScope::Process);
        // 如果成功，需要清理
        std::string result = envManager->get(testVarName, EnvVarScope::Process);
        std::cout << "混合编码测试结果: " << result << std::endl;
        envManager->remove(testVarName, EnvVarScope::Process);
    } catch (const std::exception& e) {
        // 捕获异常但不断言必须抛出
        std::cout << "设置混合编码字符串时出现异常: " << e.what() << std::endl;
    }
}

// 测试极长UTF-8字符串
// 测试极长UTF-8字符串
TEST_F(EnvironmentVariablesTest, VeryLongUTF8String) {
    // 创建一个适当长度的UTF-8字符串
    std::string longUtf8String;
    const std::string repeatedText = "中文测试字符串";  // 每个中文字符在UTF-8中占3字节

    // 创建大约30KB的字符串 - 比Windows 32KB限制略小
    for (int i = 0; i < 1500; i++) {
        longUtf8String += repeatedText;
    }

    try {
        // 尝试设置长字符串 - 可能成功也可能失败
        envManager->set(testVarName, longUtf8String, EnvVarScope::Process);
        // 如果成功，需要清理
        envManager->remove(testVarName, EnvVarScope::Process);
    } catch (const std::exception& e) {
        // 捕获异常但不断言必须抛出
        std::cout << "设置长UTF-8字符串时出现异常: " << e.what() << std::endl;
    }
}

// 测试非法路径字符
TEST_F(EnvironmentVariablesTest, IllegalPathCharacters) {
    // Windows环境变量名称限制比文件名少
    // 主要关注问号和星号，其他字符在环境变量名中可能允许
    std::vector<std::string> illegalNames = {"VAR?NAME", "VAR*NAME"};

    for (const auto& name : illegalNames) {
        try {
            // 尝试使用可能非法的名称
            envManager->set(name, "test", EnvVarScope::Process);
            // 如果成功，需要清理
            envManager->remove(name, EnvVarScope::Process);
        } catch (const std::exception& e) {
            // 捕获异常但不断言必须抛出
            std::cout << "使用可能非法的变量名时出现异常: " << e.what() << std::endl;
        }
    }
}

// 测试Unicode边界情况
TEST_F(EnvironmentVariablesTest, UnicodeBoundaryTest) {
    try {
        // 测试有效但边界的Unicode字符
        std::string extremeChar = "Unicode boundary test: \xF0\x9F\x98\x8A";  // 笑脸emoji U+1F60A

        // 尝试设置包含边界Unicode字符的环境变量
        envManager->set(testVarName, extremeChar, EnvVarScope::Process);
        // 如果成功，需要清理
        std::string result = envManager->get(testVarName, EnvVarScope::Process);
        std::cout << "边界Unicode字符测试结果: " << result << std::endl;
        envManager->remove(testVarName, EnvVarScope::Process);
    } catch (const std::exception& e) {
        // 捕获异常但不断言必须抛出
        std::cout << "设置边界Unicode字符时出现异常: " << e.what() << std::endl;
    }
}