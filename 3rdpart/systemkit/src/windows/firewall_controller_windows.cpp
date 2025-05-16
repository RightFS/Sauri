/**
 * @file firewall_controller_windows.cpp
 * @brief Implementation of IFirewallController for Windows platform.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#include "firewall_controller_windows.hpp"

#include "common/utils/strings.h"
#include "windows_utils.hpp"

#include <Windows.h>
#include <atlbase.h>
#include <comdef.h>
#include <netfw.h>
#include <stdexcept>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comsuppw.lib")

namespace leigod {
namespace system_kit {

using namespace utils;
using namespace common;
using namespace common::utils;

/**
 * @brief RAII wrapper for COM initialization
 */
class ComInitializer {
public:
    ComInitializer() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        m_initialized = SUCCEEDED(hr);
        if (!m_initialized && hr != RPC_E_CHANGED_MODE) {
            throw FirewallControllerException(ErrorCode::ComInitError, "Failed to initialize COM");
        }
    }

    ~ComInitializer() {
        if (m_initialized) {
            CoUninitialize();
        }
    }

    bool isInitialized() const {
        return m_initialized;
    }

private:
    bool m_initialized;
};

/**
 * @brief Helper class for managing firewall policy objects
 */
class FirewallPolicyHelper {
public:
    FirewallPolicyHelper() {
        // Initialize COM if not already initialized
        m_comInit = std::make_unique<ComInitializer>();

        // Create the firewall policy object
        HRESULT hr =
            CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                             __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&m_pNetFwPolicy2));

        if (FAILED(hr)) {
            throw FirewallControllerException(ErrorCode::FireWallInitError,
                                              "Failed to create firewall policy object" +
                                                  getLastErrorMessage());
        }
    }

    ~FirewallPolicyHelper() {
        if (m_pNetFwPolicy2) {
            m_pNetFwPolicy2->Release();
        }
    }

    INetFwPolicy2* getPolicy() const {
        return m_pNetFwPolicy2;
    }

private:
    std::unique_ptr<ComInitializer> m_comInit;
    INetFwPolicy2* m_pNetFwPolicy2 = nullptr;
};

/**
 * @brief Helper function to check and handle HRESULT values
 * @param hr HRESULT value to check
 * @param errorMsg Error message to include if hr indicates failure
 * @return Success if hr indicates success, otherwise Failure with error
 * @throws FirewallControllerException if hr indicates failure
 */
static void checkHResult(HRESULT hr, const std::string& errorMsg) {
    if (SUCCEEDED(hr)) {
        return;
    }

    // Same error handling as above
    ErrorCode errorCode = ErrorCode::FireWallError;
    if (hr == E_ACCESSDENIED) {
        errorCode = ErrorCode::FireWallAccessDenied;
    } else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        errorCode = ErrorCode::FireWallRuleNotFound;
    }

    throw FirewallControllerException(errorCode,
                                      errorMsg + " (HRESULT: 0x" +
                                          std::to_string(static_cast<unsigned long>(hr)) + ")");
}

/**
 * @brief Helper function to convert FirewallRule to INetFwRule
 */
CComPtr<INetFwRule> createNetFwRule(const FirewallRule& rule) {
    // 验证输入参数
    if (rule.name.empty()) {
        throw FirewallControllerException(ErrorCode::InvalidArgument,
                                          "Firewall rule name cannot be empty");
    }
    if (rule.applicationPath.empty()) {
        throw FirewallControllerException(ErrorCode::InvalidArgument,
                                          "Firewall rule application path cannot be empty");
    }

    CComPtr<INetFwRule> pFwRule;
    HRESULT hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER,
                                  __uuidof(INetFwRule), reinterpret_cast<void**>(&pFwRule));

    if (FAILED(hr)) {
        throw FirewallControllerException(ErrorCode::FireWallInitFwRuleError,
                                          "Failed to create firewall rule object: " +
                                              getLastErrorMessage());
    }

    try {
        std::wstring name = utf82wstring(rule.name);
        std::wstring applicationPath = utf82wstring(rule.applicationPath);

        // Set rule properties with error checking
        hr = pFwRule->put_Name(_bstr_t(name.c_str()));
        checkHResult(hr, "Failed to set rule name");

        hr = pFwRule->put_ApplicationName(_bstr_t(applicationPath.c_str()));
        checkHResult(hr, "Failed to set application path");

        // Set protocol
        hr = pFwRule->put_Protocol(rule.protocol);
        checkHResult(hr, "Failed to set protocol");

        // Set local ports if specified
        if (!rule.localPorts.empty()) {
            std::wstring localPorts = utf82wstring(rule.localPorts);
            hr = pFwRule->put_LocalPorts(_bstr_t(localPorts.c_str()));
            checkHResult(hr, "Failed to set local ports");
        }

        // Set remote ports if specified
        if (!rule.remotePorts.empty()) {
            std::wstring remotePorts = utf82wstring(rule.remotePorts);
            hr = pFwRule->put_RemotePorts(_bstr_t(remotePorts.c_str()));
            checkHResult(hr, "Failed to set remote ports");
        }

        // Set local addresses if specified
        if (!rule.localAddresses.empty()) {
            std::wstring localAddresses = utf82wstring(rule.localAddresses);
            hr = pFwRule->put_LocalAddresses(_bstr_t(localAddresses.c_str()));
            checkHResult(hr, "Failed to set local addresses");
        }

        // Set remote addresses if specified
        if (!rule.remoteAddresses.empty()) {
            std::wstring remoteAddresses = utf82wstring(rule.remoteAddresses);
            hr = pFwRule->put_RemoteAddresses(_bstr_t(remoteAddresses.c_str()));
            checkHResult(hr, "Failed to set remote addresses");
        }

        // Set description if specified
        if (!rule.description.empty()) {
            std::wstring description = utf82wstring(rule.description);
            hr = pFwRule->put_Description(_bstr_t(description.c_str()));
            checkHResult(hr, "Failed to set description");
        }

        // Set action (allow/block)
        NET_FW_ACTION action =
            (rule.action == FirewallAction::Allow) ? NET_FW_ACTION_ALLOW : NET_FW_ACTION_BLOCK;
        hr = pFwRule->put_Action(action);
        checkHResult(hr, "Failed to set action");

        // Set direction (inbound/outbound)
        NET_FW_RULE_DIRECTION direction = (rule.direction == FirewallDirection::Inbound) ?
                                              NET_FW_RULE_DIR_IN :
                                              NET_FW_RULE_DIR_OUT;
        hr = pFwRule->put_Direction(direction);
        checkHResult(hr, "Failed to set direction");

        // Enable the rule
        hr = pFwRule->put_Enabled(VARIANT_TRUE);
        checkHResult(hr, "Failed to enable rule");

        return pFwRule;
    } catch (const std::exception& e) {
        throw FirewallControllerException(
            ErrorCode::FireWallError, "Failed to create firewall rule: " + std::string(e.what()));
    }
}

void FirewallControllerWindows::addRule(const FirewallRule& rule) {
    try {
        // Initialize COM and get firewall policy
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // Create rule object
        CComPtr<INetFwRule> pFwRule = createNetFwRule(rule);

        // Get rules collection
        CComPtr<INetFwRules> pFwRules;
        HRESULT hr = pNetFwPolicy2->get_Rules(&pFwRules);

        checkHResult(hr, "Failed to get firewall rules collection");

        // Add the rule
        hr = pFwRules->Add(pFwRule);
        checkHResult(hr, "Failed to add firewall rule");
    } catch (const FirewallControllerException&) {
        throw;  // Rethrow the engine
    } catch (const _com_error& e) {
        throw FirewallControllerException(ErrorCode::FireWallComError,
                                          std::string("COM error in addRule: ") +
                                              std::string(e.ErrorMessage()));
    } catch (const std::exception& ex) {
        throw FirewallControllerException(ErrorCode::FireWallError,
                                          std::string("Exception in addRule: ") + ex.what());
    }
}

void FirewallControllerWindows::updateRule(const std::string& ruleName,
                                           const FirewallRule& updatedRule) {
    try {
        // 验证输入
        if (updatedRule.name.empty()) {
            throw FirewallControllerException(ErrorCode::InvalidArgument,
                                              "Updated rule must have a name");
        }
        if (updatedRule.applicationPath.empty()) {
            throw FirewallControllerException(ErrorCode::InvalidArgument,
                                              "Application path cannot be empty");
        }

        // Initialize COM and get firewall policy
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // Get rules collection
        CComPtr<INetFwRules> pFwRules;
        HRESULT hr = pNetFwPolicy2->get_Rules(&pFwRules);
        checkHResult(hr, "Failed to get firewall rules collection");

        // 查找要更新的规则
        CComPtr<INetFwRule> pFwRule;
        hr = pFwRules->Item(_bstr_t(ruleName.c_str()), &pFwRule);

        if (FAILED(hr)) {
            // 如果规则不存在，创建新规则
            removeRule(ruleName);
            return addRule(updatedRule);
        }

        // 直接更新规则属性
        std::wstring newName = utf82wstring(updatedRule.name);
        hr = pFwRule->put_Name(_bstr_t(newName.c_str()));
        checkHResult(hr, "Failed to update rule name");

        std::wstring appPath = utf82wstring(updatedRule.applicationPath);
        hr = pFwRule->put_ApplicationName(_bstr_t(appPath.c_str()));
        checkHResult(hr, "Failed to update application path");

        hr = pFwRule->put_Protocol(updatedRule.protocol);
        checkHResult(hr, "Failed to update protocol");

        // 更新端口、地址等其他属性...（类似createNetFwRule中的代码）
        if (!updatedRule.localPorts.empty()) {
            std::wstring localPorts = utf82wstring(updatedRule.localPorts);
            hr = pFwRule->put_LocalPorts(_bstr_t(localPorts.c_str()));
            checkHResult(hr, "Failed to update local ports");
        }

        // 更新动作和方向
        NET_FW_ACTION action = (updatedRule.action == FirewallAction::Allow) ? NET_FW_ACTION_ALLOW :
                                                                               NET_FW_ACTION_BLOCK;
        hr = pFwRule->put_Action(action);
        checkHResult(hr, "Failed to update action");

        NET_FW_RULE_DIRECTION direction = (updatedRule.direction == FirewallDirection::Inbound) ?
                                              NET_FW_RULE_DIR_IN :
                                              NET_FW_RULE_DIR_OUT;
        hr = pFwRule->put_Direction(direction);
        checkHResult(hr, "Failed to update direction");

        // 更新描述字段
        if (!updatedRule.description.empty()) {
            std::wstring description = utf82wstring(updatedRule.description);
            hr = pFwRule->put_Description(_bstr_t(description.c_str()));
            checkHResult(hr, "Failed to update description");
        }

        // 更新已启用状态
        hr = pFwRule->put_Enabled(updatedRule.enabled ? VARIANT_TRUE : VARIANT_FALSE);
        checkHResult(hr, "Failed to update enabled status");

        // 更新远程端口
        if (!updatedRule.remotePorts.empty()) {
            std::wstring remotePorts = utf82wstring(updatedRule.remotePorts);
            hr = pFwRule->put_RemotePorts(_bstr_t(remotePorts.c_str()));
            checkHResult(hr, "Failed to update remote ports");
        }

        // 更新本地地址
        if (!updatedRule.localAddresses.empty()) {
            std::wstring localAddresses = utf82wstring(updatedRule.localAddresses);
            hr = pFwRule->put_LocalAddresses(_bstr_t(localAddresses.c_str()));
            checkHResult(hr, "Failed to update local addresses");
        }

        // 更新远程地址
        if (!updatedRule.remoteAddresses.empty()) {
            std::wstring remoteAddresses = utf82wstring(updatedRule.remoteAddresses);
            hr = pFwRule->put_RemoteAddresses(_bstr_t(remoteAddresses.c_str()));
            checkHResult(hr, "Failed to update remote addresses");
        }

    } catch (const _com_error& e) {
        throw FirewallControllerException(ErrorCode::FireWallComError,
                                          std::string("COM error in updateRule: ") +
                                              std::string(e.ErrorMessage()));
    } catch (const std::exception& ex) {
        throw FirewallControllerException(ErrorCode::FireWallError,
                                          std::string("Exception in updateRule: ") + ex.what());
    }
}

void FirewallControllerWindows::removeRule(const std::string& ruleName) {
    try {
        // Initialize COM and get firewall policy
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // Get rules collection
        CComPtr<INetFwRules> pFwRules;
        HRESULT hr = pNetFwPolicy2->get_Rules(&pFwRules);

        checkHResult(hr, "Failed to get firewall rules collection");

        std::wstring name = utf82wstring(ruleName);
        // Remove the rule
        hr = pFwRules->Remove(_bstr_t(name.c_str()));
        checkHResult(hr, "Failed to remove firewall rule");

        // 即使直接删除返回成功，仍检查规则是否真的被删除
        if (!ruleExists(ruleName)) {
            return;  // 真的删除成功了
        }

        // 如果规则仍存在，使用枚举方式找到并删除
        CComPtr<IEnumVARIANT> pEnum;
        hr = pFwRules->get__NewEnum(reinterpret_cast<IUnknown**>(&pEnum));
        checkHResult(hr, "Failed to get firewall rules enumerator");

        VARIANT varRule;
        VariantInit(&varRule);
        bool found = false;

        while (pEnum->Next(1, &varRule, nullptr) == S_OK && !found) {
            if (varRule.vt == VT_DISPATCH) {
                CComPtr<IDispatch> pDisp = varRule.pdispVal;
                CComPtr<INetFwRule> pRule;
                hr = pDisp->QueryInterface(IID_INetFwRule, reinterpret_cast<void**>(&pRule));

                if (SUCCEEDED(hr)) {
                    BSTR bstrName = nullptr;
                    hr = pRule->get_Name(&bstrName);

                    if (SUCCEEDED(hr) && bstrName) {
                        std::wstring wRuleName(bstrName, SysStringLen(bstrName));
                        std::string currentRuleName = wideToUtf8(wRuleName);

                        if (currentRuleName == ruleName) {
                            // 找到匹配的规则，使用获取到的原始名称删除
                            hr = pFwRules->Remove(bstrName);
                            SysFreeString(bstrName);
                            checkHResult(hr, "Failed to remove firewall rule");
                            found = true;
                            continue;  // 跳过SysFreeString
                        }
                        SysFreeString(bstrName);
                    }
                }
            }
            VariantClear(&varRule);
        }

        if (!found) {
            throw FirewallControllerException(ErrorCode::FireWallRuleNotFound,
                                              "Rule not found: " + ruleName);
        }
    } catch (const FirewallControllerException&) {
        throw;  // Rethrow the engine
    } catch (const _com_error& e) {
        throw FirewallControllerException(ErrorCode::FireWallComError,
                                          std::string("COM error in removeRule: ") +
                                              std::string(e.ErrorMessage()));
    } catch (const std::exception& ex) {
        throw FirewallControllerException(ErrorCode::FireWallError,
                                          std::string("Exception in removeRule: ") + ex.what());
    }
}

bool FirewallControllerWindows::ruleExists(const std::string& ruleName) const {
    bool found = false;

    try {
        // 初始化COM并获取防火墙策略
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // 获取规则集合
        CComPtr<INetFwRules> pFwRules;
        HRESULT hr = pNetFwPolicy2->get_Rules(&pFwRules);
        checkHResult(hr, "Failed to get firewall rules collection");

        // 首先尝试直接访问
        CComPtr<INetFwRule> pDirectRule;
        auto ruleNameW = utf82wstring(ruleName);
        hr = pFwRules->Item(_bstr_t(ruleNameW.c_str()), &pDirectRule);
        if (SUCCEEDED(hr) && pDirectRule) {
            return true;
        }

        CComPtr<IEnumVARIANT> pEnum;
        hr = pFwRules->get__NewEnum(reinterpret_cast<IUnknown**>(&pEnum));
        if (FAILED(hr)) {
            checkHResult(hr, "Failed to get firewall rules enumerator");
        }

        VARIANT varRule;
        VariantInit(&varRule);

        while (pEnum->Next(1, &varRule, nullptr) == S_OK && !found) {
            if (varRule.vt == VT_DISPATCH) {
                CComPtr<IDispatch> pDisp = varRule.pdispVal;
                CComPtr<INetFwRule> pRule;
                hr = pDisp->QueryInterface(IID_INetFwRule, reinterpret_cast<void**>(&pRule));
                if (SUCCEEDED(hr)) {
                    BSTR name = nullptr;
                    hr = pRule->get_Name(&name);
                    if (SUCCEEDED(hr) && name) {
                        std::wstring wRuleName(name, SysStringLen(name));
                        std::string aRuleName = wideToUtf8(wRuleName);
                        SysFreeString(name);

                        if (aRuleName == ruleName) {
                            found = true;
                        }
                    }
                }
            }
            VariantClear(&varRule);
        }
    } catch (...) {
        throw;
    }

    return found;
}

FirewallRule convertFwRule2Rule(const CComPtr<INetFwRule>& pRule) {
    FirewallRule rule;
    // Get rule properties
    BSTR name = nullptr;
    auto hr = pRule->get_Name(&name);
    if (SUCCEEDED(hr) && name) {
        std::wstring wName(name, SysStringLen(name));
        rule.name = wideToUtf8(wName);
        SysFreeString(name);
    }

    BSTR appName = nullptr;
    hr = pRule->get_ApplicationName(&appName);
    if (SUCCEEDED(hr) && appName) {
        std::wstring wAppName(appName, SysStringLen(appName));
        rule.applicationPath = wideToUtf8(wAppName);
        SysFreeString(appName);
    }

    long protocol = 0;
    hr = pRule->get_Protocol(&protocol);
    if (SUCCEEDED(hr)) {
        rule.protocol = protocol;
    }

    BSTR localPorts = nullptr;
    hr = pRule->get_LocalPorts(&localPorts);
    if (SUCCEEDED(hr) && localPorts) {
        std::wstring wLocalPorts(localPorts, SysStringLen(localPorts));
        rule.localPorts = wideToUtf8(wLocalPorts);
        SysFreeString(localPorts);
    }

    BSTR remotePorts = nullptr;
    hr = pRule->get_RemotePorts(&remotePorts);
    if (SUCCEEDED(hr) && remotePorts) {
        std::wstring wRemotePorts(remotePorts, SysStringLen(remotePorts));
        rule.remotePorts = wideToUtf8(remotePorts);
        SysFreeString(remotePorts);
    }

    BSTR description = nullptr;
    hr = pRule->get_Description(&description);
    if (SUCCEEDED(hr) && description) {
        std::wstring wDescription(description, SysStringLen(description));
        rule.description = wideToUtf8(wDescription);
        SysFreeString(description);
    }

    NET_FW_ACTION action;
    hr = pRule->get_Action(&action);
    if (SUCCEEDED(hr)) {
        rule.action =
            (action == NET_FW_ACTION_ALLOW) ? FirewallAction::Allow : FirewallAction::Block;
    }

    NET_FW_RULE_DIRECTION direction;
    hr = pRule->get_Direction(&direction);
    if (SUCCEEDED(hr)) {
        rule.direction = (direction == NET_FW_RULE_DIR_IN) ? FirewallDirection::Inbound :
                                                             FirewallDirection::Outbound;
    }

    // 添加本地地址
    BSTR localAddresses = nullptr;
    hr = pRule->get_LocalAddresses(&localAddresses);
    if (SUCCEEDED(hr) && localAddresses) {
        std::wstring wLocalAddresses(localAddresses, SysStringLen(localAddresses));
        rule.localAddresses = wideToUtf8(wLocalAddresses);
        SysFreeString(localAddresses);
    }

    // 添加远程地址
    BSTR remoteAddresses = nullptr;
    hr = pRule->get_RemoteAddresses(&remoteAddresses);
    if (SUCCEEDED(hr) && remoteAddresses) {
        std::wstring wRemoteAddresses(remoteAddresses, SysStringLen(remoteAddresses));
        rule.remoteAddresses = wideToUtf8(wRemoteAddresses);
        SysFreeString(remoteAddresses);
    }

    // 添加启用状态
    VARIANT_BOOL enabled;
    hr = pRule->get_Enabled(&enabled);
    if (SUCCEEDED(hr)) {
        rule.enabled = (enabled == VARIANT_TRUE);
    }

    return rule;
}

std::vector<FirewallRule> FirewallControllerWindows::getRules() const {
    try {
        // 初始化COM并获取防火墙策略
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // 获取规则集合
        CComPtr<INetFwRules> pFwRules;
        HRESULT hr = pNetFwPolicy2->get_Rules(&pFwRules);
        checkHResult(hr, "Failed to get firewall rule collection");

        // 枚举规则
        std::vector<FirewallRule> rules;

        CComPtr<IEnumVARIANT> pEnum;
        hr = pFwRules->get__NewEnum(reinterpret_cast<IUnknown**>(&pEnum));
        checkHResult(hr, "Failed to get firewall rule enumerator");

        VARIANT varRule;
        VariantInit(&varRule);

        while (pEnum->Next(1, &varRule, nullptr) == S_OK) {
            if (varRule.vt == VT_DISPATCH) {
                CComPtr<IDispatch> pDisp = varRule.pdispVal;
                CComPtr<INetFwRule> pRule;
                hr = pDisp->QueryInterface(IID_INetFwRule, reinterpret_cast<void**>(&pRule));

                if (SUCCEEDED(hr)) {
                    rules.push_back(convertFwRule2Rule(pRule));
                }
            }
            VariantClear(&varRule);
        }
        return rules;
    } catch (const _com_error& e) {
        throw FirewallControllerException(ErrorCode::FireWallComError,
                                          std::string("COM error in getRules: ") +
                                              std::string(e.ErrorMessage()));
    } catch (const std::exception& ex) {
        throw FirewallControllerException(ErrorCode::FireWallError,
                                          std::string("Exception in getRules: ") + ex.what());
    }
}

FirewallStatus FirewallControllerWindows::getStatus() const {
    try {
        // Initialize COM and get firewall policy
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // Check firewall status for each profile
        FirewallStatus status;
        VARIANT_BOOL enabled = VARIANT_FALSE;

        // Domain profile
        HRESULT hr = pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_DOMAIN, &enabled);
        if (SUCCEEDED(hr)) {
            status.domainProfileEnabled = (enabled == VARIANT_TRUE);
        }

        // Private profile
        hr = pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_PRIVATE, &enabled);
        if (SUCCEEDED(hr)) {
            status.privateProfileEnabled = (enabled == VARIANT_TRUE);
        }

        // Public profile
        hr = pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_PUBLIC, &enabled);
        if (SUCCEEDED(hr)) {
            status.publicProfileEnabled = (enabled == VARIANT_TRUE);
        }

        return status;
    } catch (const _com_error& e) {
        throw FirewallControllerException(ErrorCode::FireWallComError,
                                          std::string("COM error in getStatus: ") +
                                              std::string(e.ErrorMessage()));
    } catch (const std::exception& ex) {
        throw FirewallControllerException(ErrorCode::FireWallError,
                                          std::string("Exception in getStatus: ") + ex.what());
    }
}

void FirewallControllerWindows::setStatus(FirewallStatus status) {
    try {
        // Initialize COM and get firewall policy
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // Set domain profile
        HRESULT hr = pNetFwPolicy2->put_FirewallEnabled(
            NET_FW_PROFILE2_DOMAIN, status.domainProfileEnabled ? VARIANT_TRUE : VARIANT_FALSE);
        checkHResult(hr, "Failed to set domain profile status");

        // Set private profile
        hr = pNetFwPolicy2->put_FirewallEnabled(
            NET_FW_PROFILE2_PRIVATE, status.privateProfileEnabled ? VARIANT_TRUE : VARIANT_FALSE);

        checkHResult(hr, "Failed to set private profile status");

        // Set public profile
        hr = pNetFwPolicy2->put_FirewallEnabled(
            NET_FW_PROFILE2_PUBLIC, status.publicProfileEnabled ? VARIANT_TRUE : VARIANT_FALSE);
        checkHResult(hr, "Failed to set public profile status");
    } catch (const FirewallControllerException&) {
        throw;  // Rethrow the engine
    } catch (const std::exception& ex) {
        throw FirewallControllerException(ErrorCode::FireWallError,
                                          std::string("Exception in setStatus: ") + ex.what());
    }
}

std::vector<FirewallRule> FirewallControllerWindows::getRule(const std::string& ruleName) const {
    try {
        // Initialize COM and get firewall policy
        FirewallPolicyHelper policyHelper;
        INetFwPolicy2* pNetFwPolicy2 = policyHelper.getPolicy();

        // Get rules collection
        CComPtr<INetFwRules> pFwRules;
        HRESULT hr = pNetFwPolicy2->get_Rules(&pFwRules);
        checkHResult(hr, "Failed to get firewall rule collection");

        // Enumerate rules
        std::vector<FirewallRule> rules;

        CComPtr<IEnumVARIANT> pEnum;
        hr = pFwRules->get__NewEnum(reinterpret_cast<IUnknown**>(&pEnum));
        checkHResult(hr, "Failed to get firewall rule enumerator");

        VARIANT varRule;
        VariantInit(&varRule);

        while (pEnum->Next(1, &varRule, nullptr) == S_OK) {
            if (varRule.vt == VT_DISPATCH) {
                CComPtr<IDispatch> pDisp = varRule.pdispVal;
                CComPtr<INetFwRule> pRule;
                hr = pDisp->QueryInterface(IID_INetFwRule, reinterpret_cast<void**>(&pRule));
                if (SUCCEEDED(hr)) {
                    // 获取规则名称并比较
                    BSTR name = nullptr;
                    hr = pRule->get_Name(&name);

                    if (SUCCEEDED(hr) && name) {
                        std::wstring wRuleName(name, SysStringLen(name));
                        std::string currentRuleName = wideToUtf8(wRuleName);
                        SysFreeString(name);

                        if (currentRuleName == ruleName) {
                            // 名称匹配，添加规则（不使用异常控制流）
                            FirewallRule rule = convertFwRule2Rule(pRule);
                            rules.push_back(rule);
                        }
                    }
                }
            }
            VariantClear(&varRule);
        }
        return rules;
    } catch (const _com_error& e) {
        throw FirewallControllerException(ErrorCode::FireWallComError,
                                          std::string("COM error in getRuleByName: ") +
                                              std::string(e.ErrorMessage()));
    } catch (const std::exception& ex) {
        throw FirewallControllerException(ErrorCode::FireWallError,
                                          std::string("Exception in getRuleByName: ") + ex.what());
    }
}

// Factory function
std::shared_ptr<IFirewallController> createFirewallController() {
    return std::make_shared<FirewallControllerWindows>();
}

}  // namespace system_kit
}  // namespace leigod