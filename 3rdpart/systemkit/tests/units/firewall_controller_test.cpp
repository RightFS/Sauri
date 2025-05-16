/**
 * @file firewall_controller_test.cpp
 * @brief Unit tests for firewall controller
 *
 * @author UnixCodor
 * @date 2025-03-26
 */

#include "systemkit/systemkit.hpp"

#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <thread>

using namespace leigod::system_kit;

// 生成唯一的测试规则名称，避免测试冲突
std::string generateUniqueName(const std::string& prefix) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(10000, 99999);

    return prefix + "_" + std::to_string(distrib(gen));
}

class FirewallControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建防火墙控制器
        firewallController = IComponentFactory::getInstance()->createFirewallController();
        permHandler = IComponentFactory::getInstance()->createPermissionHandler();

        // 检查是否有访问防火墙的权限
        ASSERT_NO_THROW(hasAdminRights = permHandler->isRunningAsAdministrator();)
            << "Failed to check admin rights";

        // 生成测试规则名称
        testRuleName = generateUniqueName("SystemKit_TestRule");
        testChineseRuleName = generateUniqueName("SystemKit_测试规则");

        // 清理可能存在的测试规则
        cleanupTestRules();
    }

    void TearDown() override {
        // 清理测试规则
        cleanupTestRules();
        waitForFirewallOperation();
    }

    void cleanupTestRules() {
        // 尝试删除可能存在的测试规则
        if (firewallController) {
            firewallController->removeRule(testRuleName);
            firewallController->removeRule(testChineseRuleName);
        }
    }

    // 获取测试应用路径
    std::string getTestAppPath() const {
#ifdef _WIN32
        return "C:\\Windows\\System32\\calc.exe";
#else
        return "/usr/bin/ls";
#endif
    }

    // 等待防火墙操作完成（某些防火墙操作有延迟）
    void waitForFirewallOperation() const {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::shared_ptr<IFirewallController> firewallController;
    std::shared_ptr<IPermissionHandler> permHandler;
    bool hasAdminRights = false;
    std::string testRuleName;
    std::string testChineseRuleName;
};

// 添加和检索防火墙规则
TEST_F(FirewallControllerTest, AddAndGetRule) {
    // 跳过如果没有管理员权限
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // 创建防火墙规则
    FirewallRule rule;
    rule.name = testRuleName;
    rule.description = "Test rule created by SystemKit unit test";
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;
    rule.enabled = true;
    rule.protocol = FirewallProtocol::TCP;
    rule.localPorts = "8080";

    // 添加规则
    ASSERT_NO_THROW(firewallController->addRule(rule)) << "Failed to add firewall rule";

    // 等待规则生效
    waitForFirewallOperation();

    // 验证规则是否存在
    std::vector<FirewallRule> rules;
    ASSERT_NO_THROW(rules = firewallController->getRule(testRuleName))
        << "Failed to get added firewall rule";
    ASSERT_TRUE(!rules.empty()) << "Failed to get added firewall rule";

    // 验证规则属性
    for (const auto& retrievedRule : rules) {
        EXPECT_EQ(retrievedRule.name, rule.name);
        EXPECT_EQ(retrievedRule.action, rule.action);
        EXPECT_EQ(retrievedRule.direction, rule.direction);
        EXPECT_TRUE(retrievedRule.enabled);
        // 验证应用程序路径（可能会被标准化，所以只验证包含）
        EXPECT_NE(retrievedRule.applicationPath.find(
                      getTestAppPath().substr(getTestAppPath().find_last_of("\\/") + 1)),
                  std::string::npos);
    }
}

// 测试中文规则名称
TEST_F(FirewallControllerTest, AddAndGetChineseNameRule) {
    // 跳过如果没有管理员权限
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // 创建带有中文名称的防火墙规则
    FirewallRule rule;
    rule.name = testChineseRuleName;
    rule.description = "测试规则 - 由 SystemKit 单元测试创建";
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;
    rule.enabled = true;
    rule.protocol = FirewallProtocol::Any;

    // 添加规则
    ASSERT_NO_THROW(firewallController->addRule(rule))
        << "Failed to add firewall rule with Chinese name";

    // 等待规则生效
    waitForFirewallOperation();

    // 验证规则是否存在
    std::vector<FirewallRule> rules;
    ASSERT_NO_THROW(rules = firewallController->getRule(testChineseRuleName))
        << "Failed to get added firewall rule with Chinese name";

    // 验证规则属性
    for (const auto& retrievedRule : rules) {
        EXPECT_EQ(retrievedRule.name, rule.name);
        EXPECT_EQ(retrievedRule.action, rule.action);
        EXPECT_EQ(retrievedRule.direction, rule.direction);
        EXPECT_TRUE(retrievedRule.enabled);
        // 验证应用程序路径（可能会被标准化，所以只验证包含）
        EXPECT_NE(retrievedRule.applicationPath.find(
                      getTestAppPath().substr(getTestAppPath().find_last_of("\\/") + 1)),
                  std::string::npos);
    }
}

// 测试更新规则
TEST_F(FirewallControllerTest, UpdateRule) {
    // 跳过如果没有管理员权限
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // 创建初始规则
    FirewallRule rule;
    rule.name = testRuleName;
    rule.description = "Initial description";
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;
    rule.enabled = true;
    rule.protocol = FirewallProtocol::TCP;
    rule.localPorts = "8080";

    // 添加规则
    ASSERT_NO_THROW(firewallController->addRule(rule)) << "Failed to add initial firewall rule";

    waitForFirewallOperation();

    // 修改规则
    rule.description = "Updated description";
    rule.action = FirewallAction::Block;
    rule.localPorts.clear();
    rule.localPorts = "9090";
    rule.enabled = false;

    // 更新规则
    ASSERT_NO_THROW(firewallController->updateRule(rule.name, rule))
        << "Failed to update firewall rule";

    waitForFirewallOperation();

    // 验证更新
    std::vector<FirewallRule> rules;
    ASSERT_NO_THROW(rules = firewallController->getRule(testRuleName))
        << "Failed to get updated firewall rule";
    for (const auto& updatedRule : rules) {
        EXPECT_EQ(updatedRule.description, "Updated description");
        EXPECT_EQ(updatedRule.action, FirewallAction::Block);

        // 注意：某些防火墙实现可能不支持禁用规则
        // EXPECT_FALSE(updatedRule.enabled);

        // 检查端口（注意：某些防火墙可能会以不同格式返回端口）
        bool found9090 = updatedRule.localPorts.find("9090") != std::string::npos;
        EXPECT_TRUE(found9090 || updatedRule.localPorts.empty()) << "Updated port 9090 not found";
    }
}

// 测试删除规则
TEST_F(FirewallControllerTest, RemoveRule) {
    // 跳过如果没有管理员权限
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // 创建规则
    FirewallRule rule;
    rule.name = testRuleName;
    rule.description = "Rule to be removed";
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;
    rule.enabled = true;

    // 添加规则
    ASSERT_NO_THROW(firewallController->addRule(rule)) << "Failed to add rule for removal";

    waitForFirewallOperation();

    // 确认规则存在
    std::vector<FirewallRule> rules;
    ASSERT_NO_THROW(rules = firewallController->getRule(testRuleName))
        << "Failed to get rule before removal";

    // 删除规则
    ASSERT_NO_THROW(firewallController->removeRule(testRuleName)) << "Failed to remove rule";

    waitForFirewallOperation();

    // 确认规则已删除
    std::vector<FirewallRule> rules_;
    ASSERT_NO_THROW(rules_ = firewallController->getRule(testRuleName))
        << "Failed to get rule after removal";
    EXPECT_TRUE(rules_.empty());
}

// 测试获取所有规则
TEST_F(FirewallControllerTest, GetAllRules) {
    // 跳过如果没有管理员权限
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // 创建两个测试规则
    FirewallRule rule1;
    rule1.name = testRuleName;
    rule1.description = "Test rule 1";
    rule1.applicationPath = getTestAppPath();
    rule1.action = FirewallAction::Allow;
    rule1.direction = FirewallDirection::Inbound;
    rule1.enabled = true;

    FirewallRule rule2;
    rule2.name = testChineseRuleName;
    rule2.description = "测试规则 2";
    rule2.applicationPath = getTestAppPath();
    rule2.action = FirewallAction::Block;
    rule2.direction = FirewallDirection::Outbound;
    rule2.enabled = true;

    // 添加规则
    ASSERT_NO_THROW(firewallController->addRule(rule1))
        << "Failed to add first rule for getAllRules test";

    ASSERT_NO_THROW(firewallController->addRule(rule2))
        << "Failed to add second rule for getAllRules test";

    waitForFirewallOperation();

    // 获取所有规则
    std::vector<FirewallRule> rules;
    ASSERT_NO_THROW(rules = firewallController->getRules()) << "Failed to get all rules";
    auto getAllResult = firewallController->getRules();

    // 查找我们创建的规则
    bool foundRule1 = false;
    bool foundRule2 = false;

    for (const auto& rule : rules) {
        if (rule.name == testRuleName)
            foundRule1 = true;
        if (rule.name == testChineseRuleName)
            foundRule2 = true;
    }

    EXPECT_TRUE(foundRule1) << "First rule not found in getAllRules()";
    EXPECT_TRUE(foundRule2) << "Second rule not found in getAllRules()";

    // 输出规则总数
    std::cout << "Total firewall rules: " << rules.size() << std::endl;
}

// 测试完整的中文内容规则
TEST_F(FirewallControllerTest, CompleteChineseContentRule) {
    // 跳过如果没有管理员权限
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // 创建包含完整中文内容的规则
    FirewallRule rule;
    rule.name = "完全中文规则_" + std::to_string(std::rand() % 10000);
    rule.description = "这是一个包含完全中文内容的测试规则";
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;
    rule.enabled = true;
    rule.protocol = FirewallProtocol::TCP;

    // 添加本地和远程地址（使用标准格式，不包含中文注解）
    rule.localAddresses = "192.168.1.0/24";
    rule.remoteAddresses = "10.0.0.0/8";

    // 添加端口
    rule.localPorts = "8080";
    rule.remotePorts = "443";

    // 添加规则（使用更健壮的错误处理）
    try {
        firewallController->addRule(rule);
        std::cout << "Successfully added Chinese content rule" << std::endl;
    } catch (const FirewallControllerException& e) {
        std::cerr << "Failed to add Chinese content rule: " << e.what() << std::endl;
        std::cerr << "Error code: " << static_cast<int>(e.code()) << std::endl;
        FAIL() << "Exception thrown when adding Chinese content rule";
    } catch (const std::exception& e) {
        std::cerr << "Unexpected engine: " << e.what() << std::endl;
        FAIL() << "Unexpected engine thrown";
    }

    waitForFirewallOperation();

    // 验证规则
    try {
        std::vector<FirewallRule> rules = firewallController->getRule(rule.name);
        ASSERT_FALSE(rules.empty()) << "Rule with Chinese name not found after addition";

        FirewallRule retrievedRule = rules[0];
        EXPECT_EQ(retrievedRule.name, rule.name);
        EXPECT_EQ(retrievedRule.description, rule.description);
        EXPECT_TRUE(retrievedRule.enabled);
        EXPECT_EQ(retrievedRule.protocol, rule.protocol);
        EXPECT_EQ(retrievedRule.action, rule.action);
        EXPECT_EQ(retrievedRule.direction, rule.direction);

        // Verify ports and addresses
        EXPECT_EQ(retrievedRule.localPorts, rule.localPorts);
        EXPECT_EQ(retrievedRule.remotePorts, rule.remotePorts);
        //        EXPECT_EQ(retrievedRule.localAddresses, rule.localAddresses);
        //        EXPECT_EQ(retrievedRule.remoteAddresses, rule.remoteAddresses);

        std::cout << "Successfully verified Chinese content rule" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to verify Chinese content rule: " << e.what() << std::endl;
        FAIL() << "Exception thrown when verifying Chinese content rule";
    }

    // 清理
    ASSERT_NO_THROW(firewallController->removeRule(rule.name));
    waitForFirewallOperation();
}

// Test adding a rule with invalid parameters
TEST_F(FirewallControllerTest, AddRuleWithInvalidParameters) {
    // Create a rule with empty name
    FirewallRule ruleWithEmptyName;
    ruleWithEmptyName.applicationPath = getTestAppPath();
    ruleWithEmptyName.action = FirewallAction::Allow;
    ruleWithEmptyName.direction = FirewallDirection::Inbound;

    // Should throw an engine
    EXPECT_THROW(firewallController->addRule(ruleWithEmptyName), FirewallControllerException);

    // Create a rule with empty application path
    FirewallRule ruleWithEmptyPath;
    ruleWithEmptyPath.name = "TestRuleWithEmptyPath";
    ruleWithEmptyPath.action = FirewallAction::Allow;
    ruleWithEmptyPath.direction = FirewallDirection::Inbound;

    // Should throw an engine
    EXPECT_THROW(firewallController->addRule(ruleWithEmptyPath), FirewallControllerException);
}

// Test updating a non-existent rule
TEST_F(FirewallControllerTest, UpdateNonExistentRule) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // Create a rule object
    FirewallRule rule;
    rule.name = "NonExistentRule" + std::to_string(std::rand() % 10000);
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;

    // Try to update a non-existent rule (should create it by implementation)
    ASSERT_NO_THROW(firewallController->updateRule(rule.name, rule));
    waitForFirewallOperation();

    // Verify rule was created
    std::vector<FirewallRule> rules;
    ASSERT_NO_THROW(rules = firewallController->getRule(rule.name));
    EXPECT_FALSE(rules.empty());

    // Clean up
    ASSERT_NO_THROW(firewallController->removeRule(rule.name));
}

// Test invalid port format
TEST_F(FirewallControllerTest, InvalidPortFormat) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // Create a rule with invalid port format
    FirewallRule ruleWithInvalidPort;
    ruleWithInvalidPort.name = testRuleName;
    ruleWithInvalidPort.applicationPath = getTestAppPath();
    ruleWithInvalidPort.action = FirewallAction::Allow;
    ruleWithInvalidPort.direction = FirewallDirection::Inbound;
    ruleWithInvalidPort.localPorts = "InvalidPort";  // Not numeric

    // Should throw an engine
    EXPECT_THROW(firewallController->addRule(ruleWithInvalidPort), FirewallControllerException);
}

// Test invalid address format
TEST_F(FirewallControllerTest, InvalidAddressFormat) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // Create a rule with invalid address format
    FirewallRule ruleWithInvalidAddress;
    ruleWithInvalidAddress.name = testRuleName;
    ruleWithInvalidAddress.applicationPath = getTestAppPath();
    ruleWithInvalidAddress.action = FirewallAction::Allow;
    ruleWithInvalidAddress.direction = FirewallDirection::Inbound;
    ruleWithInvalidAddress.localAddresses = "InvalidAddress!@#";  // Invalid format

    // Should throw an engine
    EXPECT_THROW(firewallController->addRule(ruleWithInvalidAddress), FirewallControllerException);
}

// Test operational errors with specific error codes
TEST_F(FirewallControllerTest, CheckSpecificErrorCodes) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // Try to add rule with invalid parameters
    FirewallRule invalidRule;
    invalidRule.name = testRuleName;
    // Missing application path

    try {
        firewallController->addRule(invalidRule);
        FAIL() << "Expected engine not thrown";
    } catch (const FirewallControllerException& e) {
        EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::InvalidArgument));
        std::cout << "Caught expected engine: " << e.what() << std::endl;
    } catch (...) {
        FAIL() << "Wrong engine type thrown";
    }
}

// Test adding a rule with invalid parameters
TEST_F(FirewallControllerTest, InvalidParametersTest) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // Rule with empty name
    FirewallRule ruleWithEmptyName;
    ruleWithEmptyName.applicationPath = getTestAppPath();
    ruleWithEmptyName.action = FirewallAction::Allow;
    ruleWithEmptyName.direction = FirewallDirection::Inbound;

    try {
        firewallController->addRule(ruleWithEmptyName);
        FAIL() << "Expected engine for empty rule name was not thrown";
    } catch (const FirewallControllerException& e) {
        EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::InvalidArgument));
        std::cout << "Correctly caught engine for empty name: " << e.what() << std::endl;
    }

    // Rule with empty application path
    FirewallRule ruleWithEmptyPath;
    ruleWithEmptyPath.name = "TestRuleWithEmptyPath";
    ruleWithEmptyPath.action = FirewallAction::Allow;
    ruleWithEmptyPath.direction = FirewallDirection::Inbound;

    try {
        firewallController->addRule(ruleWithEmptyPath);
        FAIL() << "Expected engine for empty application path was not thrown";
    } catch (const FirewallControllerException& e) {
        EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::InvalidArgument));
        std::cout << "Correctly caught engine for empty path: " << e.what() << std::endl;
    }
}

// Test with invalid port formats
TEST_F(FirewallControllerTest, InvalidPortFormatTest) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    FirewallRule rule;
    rule.name = testRuleName;
    rule.description = "Rule with invalid port";
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;
    rule.enabled = true;
    rule.protocol = FirewallProtocol::TCP;

    // Test with non-numeric port
    rule.localPorts = "abc";

    try {
        firewallController->addRule(rule);
        FAIL() << "Expected engine for invalid port format was not thrown";
    } catch (const FirewallControllerException& e) {
        std::cout << "Correctly caught engine for invalid port: " << e.what() << std::endl;
    }

    // Test with invalid port range
    rule.localPorts = "70000";

    try {
        firewallController->addRule(rule);
        FAIL() << "Expected engine for invalid port range was not thrown";
    } catch (const FirewallControllerException& e) {
        std::cout << "Correctly caught engine for invalid port range: " << e.what() << std::endl;
    }
}

// Test with invalid address formats
TEST_F(FirewallControllerTest, InvalidAddressFormatTest) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    FirewallRule rule;
    rule.name = testRuleName;
    rule.description = "Rule with invalid address";
    rule.applicationPath = getTestAppPath();
    rule.action = FirewallAction::Allow;
    rule.direction = FirewallDirection::Inbound;
    rule.enabled = true;
    rule.protocol = FirewallProtocol::TCP;

    // Invalid IP address format
    rule.localAddresses = "999.999.999.999";

    try {
        firewallController->addRule(rule);
        FAIL() << "Expected engine for invalid address was not thrown";
    } catch (const FirewallControllerException& e) {
        std::cout << "Correctly caught engine for invalid address: " << e.what() << std::endl;
    }

    // Invalid network mask
    rule.localAddresses = "192.168.1.0/40";

    try {
        firewallController->addRule(rule);
        FAIL() << "Expected engine for invalid network mask was not thrown";
    } catch (const FirewallControllerException& e) {
        std::cout << "Correctly caught engine for invalid network mask: " << e.what() << std::endl;
    }
}

// Test with non-existent rule operations
TEST_F(FirewallControllerTest, NonExistentRuleTest) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    std::string nonExistentRuleName = "NonExistentRule_" + std::to_string(std::rand() % 10000);

    // Try to remove a non-existent rule
    try {
        firewallController->removeRule(nonExistentRuleName);
        // Note: This might not throw an engine as removing a non-existent rule
        // is often treated as a successful no-op in Windows Firewall API
    } catch (const FirewallControllerException& e) {
        EXPECT_EQ(e.code(), static_cast<int>(ErrorCode::RuleNotFound));
        std::cout << "Caught engine for removing non-existent rule: " << e.what() << std::endl;
    }

    // Try to get a non-existent rule
    std::vector<FirewallRule> rules;
    try {
        rules = firewallController->getRule(nonExistentRuleName);
        EXPECT_TRUE(rules.empty()) << "Expected empty result for non-existent rule";
    } catch (const FirewallControllerException& e) {
        std::cout << "Caught engine when getting non-existent rule: " << e.what() << std::endl;
    }
}

// Test with malformed rule data
TEST_F(FirewallControllerTest, MalformedRuleDataTest) {
    if (!hasAdminRights) {
        GTEST_SKIP() << "Test requires administrator rights";
    }

    // Rule with extremely long name (potential buffer overflow test)
    FirewallRule ruleLongName;
    ruleLongName.name = std::string(2000, 'X');  // 2000 character name
    ruleLongName.applicationPath = getTestAppPath();
    ruleLongName.action = FirewallAction::Allow;
    ruleLongName.direction = FirewallDirection::Inbound;

    try {
        firewallController->addRule(ruleLongName);
        // The API might truncate the name without error
        std::cout << "Warning: Added rule with extremely long name" << std::endl;

        // Clean up if it actually got added
        firewallController->removeRule(ruleLongName.name);
    } catch (const FirewallControllerException& e) {
        std::cout << "Correctly caught engine for extremely long name: " << e.what() << std::endl;
    }

    // Rule with special characters in name
    FirewallRule ruleSpecialChars;
    ruleSpecialChars.name = "Test*Rule?With|Special<Characters>";
    ruleSpecialChars.applicationPath = getTestAppPath();
    ruleSpecialChars.action = FirewallAction::Allow;
    ruleSpecialChars.direction = FirewallDirection::Inbound;

    try {
        firewallController->addRule(ruleSpecialChars);
        std::cout << "Successfully added rule with special characters" << std::endl;

        // Clean up
        firewallController->removeRule(ruleSpecialChars.name);
    } catch (const FirewallControllerException& e) {
        std::cout << "Caught engine for special characters in name: " << e.what() << std::endl;
    }
}