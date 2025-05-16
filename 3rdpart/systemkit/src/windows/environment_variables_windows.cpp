/**
 * @file environment_variables_windows.cpp
 * @brief Environment system_kit implementation for Windows.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-13 00:25:10 (UTC)
 * Author: chenxu
 */

#include "environment_variables_windows.hpp"

#include "common/utils/strings.h"
#include "windows_utils.hpp"

#include <Windows.h>

namespace leigod {
namespace system_kit {

using namespace utils;
using namespace common::utils;

std::string getRegistryEnvironmentVariable(HKEY hKey, const std::string& name) {
    try {
        // 转换变量名为宽字符
        std::wstring wName = utf8ToWide(name);

        // 获取值类型和大小
        DWORD type = 0;
        DWORD dataSize = 0;
        LONG result = RegQueryValueExW(hKey, wName.c_str(), nullptr, &type, nullptr, &dataSize);

        if (result != ERROR_SUCCESS) {
            if (result == ERROR_FILE_NOT_FOUND) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableNotFound,
                                                   "Environment variable not found: " + name);
            } else {
                throw EnvironmentVariableException(ErrorCode::EnvironmentQueryError,
                                                   "Failed to query registry value: " +
                                                       getLastErrorMessage());
            }
        }

        // 确保是字符串类型
        if (type != REG_SZ && type != REG_EXPAND_SZ) {
            throw EnvironmentVariableException(ErrorCode::EnvironmentTypeError,
                                               "Registry value is not a string type");
        }

        // 分配缓冲区并获取值
        std::vector<WCHAR> buffer(dataSize / sizeof(WCHAR) + 1, 0);  // +1 确保以 null 结尾
        result = RegQueryValueExW(hKey, wName.c_str(), nullptr, &type,
                                  reinterpret_cast<LPBYTE>(buffer.data()), &dataSize);

        if (result != ERROR_SUCCESS) {
            throw EnvironmentVariableException(ErrorCode::EnvironmentQueryError,
                                               "Failed to get registry value: " +
                                                   getLastErrorMessage());
        }

        // 确保字符串正确终止
        buffer[dataSize / sizeof(WCHAR)] = L'\0';

        // 如果是可展开字符串，需要展开环境变量引用
        if (type == REG_EXPAND_SZ) {
            DWORD expandedSize = ExpandEnvironmentStringsW(buffer.data(), nullptr, 0);
            if (expandedSize > 0) {
                std::vector<WCHAR> expandedBuffer(expandedSize, 0);
                if (ExpandEnvironmentStringsW(buffer.data(), expandedBuffer.data(), expandedSize)) {
                    // 转换为 UTF-8 并返回
                    return wideToUtf8(expandedBuffer.data());
                }
            }
        }

        // 转换为 UTF-8 并返回
        return wideToUtf8(buffer.data());
    } catch (const EnvironmentVariableException&) {
        throw;  // 重新抛出自定义异常
    } catch (const std::exception& ex) {
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in getRegistryEnvironmentVariable: " +
                                               std::string(ex.what()));
    }
}

std::string EnvironmentVariablesWindows::get(const std::string& name, EnvVarScope scope) const {
    if (name.empty()) {
        throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                           "Environment variable name cannot be empty");
    }

    try {
        if (scope == EnvVarScope::Process) {
            std::wstring wName = utf8ToWide(name);

            // 获取所需的缓冲区大小
            DWORD bufferSize = GetEnvironmentVariableW(wName.c_str(), nullptr, 0);
            if (bufferSize == 0) {
                DWORD error = GetLastError();
                if (error == ERROR_ENVVAR_NOT_FOUND) {
                    throw EnvironmentVariableException(ErrorCode::EnvironmentVariableNotFound,
                                                       "Environment variable not found: " + name);
                } else {
                    throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                       "Failed to get environment variable size: " +
                                                           getLastErrorMessage());
                }
            }

            // 分配并填充缓冲区
            std::vector<WCHAR> buffer(bufferSize);
            DWORD result = GetEnvironmentVariableW(wName.c_str(), buffer.data(), bufferSize);
            if (result == 0 || result >= bufferSize) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentSizeError,
                                                   "Failed to get environment variable value: " +
                                                       getLastErrorMessage());
            }

            // 转换为 UTF-8 并返回
            return wideToUtf8(buffer.data());
        } else if (scope == EnvVarScope::User || scope == EnvVarScope::System) {
            HKEY rootKey = HKEY_CURRENT_USER;
            std::string keyPath = "Environment";

            if (scope == EnvVarScope::System) {
                rootKey = HKEY_LOCAL_MACHINE;
                keyPath = R"(SYSTEM\CurrentControlSet\Control\Session Manager\Environment)";
            }
            // 打开注册表键
            std::wstring wKeyPath = utf8ToWide(keyPath);
            HKEY hKey;
            LONG result = RegOpenKeyExW(rootKey, wKeyPath.c_str(), 0, KEY_READ, &hKey);

            if (result != ERROR_SUCCESS) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to open registry key: " +
                                                       getLastErrorMessage());
            }

            // 使用 RAII 确保注册表句柄被关闭
            struct RegKeyCloser {
                HKEY hKey;
                ~RegKeyCloser() {
                    if (hKey)
                        RegCloseKey(hKey);
                }
            } keyCloser{hKey};

            return getRegistryEnvironmentVariable(hKey, name);
        } else {
            throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                               "Invalid environment variable scope: " +
                                                   std::to_string(static_cast<int>(scope)));
        }
    } catch (const EnvironmentVariableException&) {
        throw;  // 重新抛出自定义异常
    } catch (const std::exception& ex) {
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in get: " + std::string(ex.what()));
    }
}

void EnvironmentVariablesWindows::set(const std::string& name, const std::string& value,
                                      EnvVarScope scope) {
    if (name.empty()) {
        throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                           "Environment variable name cannot be empty");
    }

    try {
        // 转换为宽字符
        std::wstring wName = utf8ToWide(name);
        std::wstring wValue = utf8ToWide(value);

        if (scope == EnvVarScope::Process) {
            // 设置进程环境变量
            BOOL success = SetEnvironmentVariableW(wName.c_str(), wValue.c_str());
            if (!success) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to set environment variable: " +
                                                       getLastErrorMessage());
            }
            return;
        } else if (scope == EnvVarScope::User || scope == EnvVarScope::System) {
            HKEY rootKey = HKEY_CURRENT_USER;
            std::string keyPath = "Environment";

            // 修正系统环境变量的注册表路径
            if (scope == EnvVarScope::System) {
                rootKey = HKEY_LOCAL_MACHINE;
                keyPath = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
            }

            // 检查系统环境变量的权限
            if (scope == EnvVarScope::System) {
                BOOL isAdmin = FALSE;
                SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
                PSID adminGroup = nullptr;

                // 创建一个 SID 来表示管理员组
                if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                              DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                              &adminGroup)) {
                    throw EnvironmentVariableException(ErrorCode::EnvironmentCheckAdminError,
                                                       "Failed to initialize SID: " +
                                                           getLastErrorMessage());
                }

                // 检查当前用户是否属于管理员组
                if (!CheckTokenMembership(nullptr, adminGroup, &isAdmin)) {
                    FreeSid(adminGroup);
                    throw EnvironmentVariableException(ErrorCode::EnvironmentCheckAdminError,
                                                       "Failed to check administrator rights: " +
                                                           getLastErrorMessage());
                }

                FreeSid(adminGroup);

                if (!isAdmin) {
                    throw EnvironmentVariableException(ErrorCode::AccessDenied,
                                                       "Administrator privileges are required to "
                                                       "modify system environment variables");
                }
            }

            // 打开注册表键
            std::wstring wKeyPath = utf8ToWide(keyPath);
            HKEY hKey;
            LONG result = RegOpenKeyExW(rootKey, wKeyPath.c_str(), 0, KEY_WRITE, &hKey);

            if (result != ERROR_SUCCESS) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to open registry key: " +
                                                       getLastErrorMessage());
            }

            // 使用 RAII 确保注册表句柄被关闭
            struct RegKeyCloser {
                HKEY hKey;
                ~RegKeyCloser() {
                    if (hKey)
                        RegCloseKey(hKey);
                }
            } keyCloser{hKey};

            // 设置注册表值
            result = RegSetValueExW(hKey, wName.c_str(), 0, REG_SZ,
                                    reinterpret_cast<const BYTE*>(wValue.c_str()),
                                    static_cast<DWORD>((wValue.length() + 1) * sizeof(WCHAR)));

            if (result != ERROR_SUCCESS) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to set registry value: " +
                                                       getLastErrorMessage());
            }

            // 广播环境变更消息
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                                reinterpret_cast<LPARAM>(L"Environment"), SMTO_ABORTIFHUNG, 5000,
                                nullptr);

            return;
        } else {
            throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                               "Invalid environment variable scope: " +
                                                   std::to_string(static_cast<int>(scope)));
        }
    } catch (const EnvironmentVariableException&) {
        throw;  // 重新抛出自定义异常
    } catch (const std::exception& ex) {
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in set: " + std::string(ex.what()));
    }
}

void EnvironmentVariablesWindows::remove(const std::string& name, EnvVarScope scope) {
    if (name.empty()) {
        throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                           "Environment variable name cannot be empty");
    }

    try {
        // 转换为宽字符
        std::wstring wName = utf8ToWide(name);

        if (scope == EnvVarScope::Process) {
            // 删除进程环境变量
            BOOL success = SetEnvironmentVariableW(wName.c_str(), nullptr);
            if (!success) {
                DWORD error = GetLastError();
                if (error == ERROR_ENVVAR_NOT_FOUND) {
                    return;  // 变量不存在被视为成功
                }
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to remove environment variable: " +
                                                       getLastErrorMessage());
            }
            return;
        } else if (scope == EnvVarScope::User || scope == EnvVarScope::System) {
            HKEY rootKey = HKEY_CURRENT_USER;
            std::string keyPath = "Environment";

            // 修正系统环境变量的注册表路径
            if (scope == EnvVarScope::System) {
                rootKey = HKEY_LOCAL_MACHINE;
                keyPath = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
            }

            // 检查系统环境变量的权限
            if (scope == EnvVarScope::System) {
                BOOL isAdmin = FALSE;
                SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
                PSID adminGroup = nullptr;

                // 创建一个 SID 来表示管理员组
                if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                              DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                              &adminGroup)) {
                    throw EnvironmentVariableException(ErrorCode::EnvironmentCheckAdminError,
                                                       "Failed to initialize SID: " +
                                                           getLastErrorMessage());
                }

                // 检查当前用户是否属于管理员组
                if (!CheckTokenMembership(nullptr, adminGroup, &isAdmin)) {
                    FreeSid(adminGroup);
                    throw EnvironmentVariableException(ErrorCode::EnvironmentCheckAdminError,
                                                       "Failed to check administrator rights: " +
                                                           getLastErrorMessage());
                }

                FreeSid(adminGroup);

                if (!isAdmin) {
                    throw EnvironmentVariableException(ErrorCode::AccessDenied,
                                                       "Administrator privileges are required to "
                                                       "modify system environment variables");
                }
            }

            // 打开注册表键
            std::wstring wKeyPath = utf8ToWide(keyPath);
            HKEY hKey;
            LONG result = RegOpenKeyExW(rootKey, wKeyPath.c_str(), 0, KEY_WRITE, &hKey);

            if (result != ERROR_SUCCESS) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to open registry key: " +
                                                       getLastErrorMessage());
            }

            // 使用 RAII 确保注册表句柄被关闭
            struct RegKeyCloser {
                HKEY hKey;
                ~RegKeyCloser() {
                    if (hKey)
                        RegCloseKey(hKey);
                }
            } keyCloser{hKey};

            // 删除注册表值
            result = RegDeleteValueW(hKey, wName.c_str());

            if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to delete registry value: " +
                                                       getLastErrorMessage());
            }

            // 广播环境变更消息
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                                reinterpret_cast<LPARAM>(L"Environment"), SMTO_ABORTIFHUNG, 5000,
                                nullptr);

            return;
        } else {
            throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                               "Invalid environment variable scope: " +
                                                   std::to_string(static_cast<int>(scope)));
        }
    } catch (const EnvironmentVariableException&) {
        throw;  // 重新抛出自定义异常
    } catch (const std::exception& ex) {
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in remove: " + std::string(ex.what()));
    }
}

bool EnvironmentVariablesWindows::exists(const std::string& name, EnvVarScope scope) const {
    if (name.empty()) {
        throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                           "Environment variable name cannot be empty");
    }

    try {
        // 使用 get 方法检查变量是否存在
        get(name, scope);

        // 如果没有抛出异常，则变量存在
        return true;
    } catch (const EnvironmentVariableException& e) {
        if (e.code() == static_cast<int>(ErrorCode::EnvironmentVariableNotFound)) {
            // 变量不存在
            return false;
        } else {
            // 其他错误，传递错误信息
            throw;
        }
    } catch (const std::exception& ex) {
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in exists: " + std::string(ex.what()));
    }
}

std::map<std::string, std::string> EnvironmentVariablesWindows::getAll(EnvVarScope scope) const {
    try {
        std::map<std::string, std::string> envVars;

        if (scope == EnvVarScope::Process) {
            // 获取进程环境变量

            // 获取环境块
            LPWCH envBlock = GetEnvironmentStringsW();
            if (envBlock == nullptr) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentGetError,
                                                   "Failed to get environment strings: " +
                                                       getLastErrorMessage());
            }

            // 使用 RAII 确保环境块被释放
            struct EnvBlockFreer {
                LPWCH block;
                ~EnvBlockFreer() {
                    if (block)
                        FreeEnvironmentStringsW(block);
                }
            } blockFreer{envBlock};

            // 遍历环境块，提取变量名和值
            LPWCH currentVar = envBlock;
            while (*currentVar) {
                std::wstring entry(currentVar);
                size_t equalsPos = entry.find(L'=');

                if (equalsPos != std::wstring::npos && equalsPos > 0) {
                    std::wstring name = entry.substr(0, equalsPos);
                    std::wstring value = entry.substr(equalsPos + 1);

                    envVars[wideToUtf8(name)] = wideToUtf8(value);
                }

                // 移动到下一个条目
                currentVar += entry.length() + 1;
            }
            return envVars;
        } else if (scope == EnvVarScope::User || scope == EnvVarScope::System) {
            // 获取用户或系统环境变量

            // 首先获取所有变量名
            auto names = getNames(scope);

            // 然后获取每个变量的值
            for (const auto& name : names) {
                auto value = get(name, scope);
                envVars[name] = value;
            }

            return envVars;
        } else {
            throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                               "Invalid environment variable scope: " +
                                                   std::to_string(static_cast<int>(scope)));
        }
    } catch (const EnvironmentVariableException&) {
        throw;
    } catch (const std::exception& ex) {
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in getAll: " + std::string(ex.what()));
    }
}

std::string EnvironmentVariablesWindows::expand(const std::string& input) const {
    if (input.empty()) {
        // 如果输入为空，返回空字符串
        return "";
    }

    try {
        // 转换输入字符串为宽字符
        std::wstring wInput = utf8ToWide(input);

        // 获取所需缓冲区大小
        DWORD bufferSize = ExpandEnvironmentStringsW(wInput.c_str(), nullptr, 0);
        if (bufferSize == 0) {
            throw EnvironmentVariableException(ErrorCode::EnvironmentSizeError,
                                               "failed get size: " + getLastErrorMessage());
        }

        // 分配缓冲区
        std::vector<WCHAR> buffer(bufferSize);
        // 展开环境变量
        DWORD result = ExpandEnvironmentStringsW(wInput.c_str(), buffer.data(), bufferSize);
        if (result == 0 || result > bufferSize) {
            throw EnvironmentVariableException(ErrorCode::EnvironmentExpandError,
                                               "Failed to expand environment variables: " +
                                                   getLastErrorMessage());
        }

        // 转换结果为 UTF-8
        return wideToUtf8(buffer.data());
    } catch (const EnvironmentVariableException&) {
        throw;
    } catch (const std::exception& ex) {
        // 捕获其他异常并返回错误信息
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in expand: " + std::string(ex.what()));
    }
}

std::vector<std::string> EnvironmentVariablesWindows::getNames(EnvVarScope scope) const {
    try {
        if (scope == EnvVarScope::Process) {
            // 获取进程环境变量名列表
            std::vector<std::string> names;

            // 获取环境块
            LPWCH envBlock = GetEnvironmentStringsW();
            if (envBlock == nullptr) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to get environment strings: " +
                                                       getLastErrorMessage());
            }

            // 使用 RAII 确保环境块被释放
            struct EnvBlockFreer {
                LPWCH block;
                ~EnvBlockFreer() {
                    if (block)
                        FreeEnvironmentStringsW(block);
                }
            } blockFreer{envBlock};

            // 遍历环境块，提取变量名
            LPWCH currentVar = envBlock;
            while (*currentVar) {
                std::wstring entry(currentVar);
                size_t equalsPos = entry.find(L'=');

                if (equalsPos != std::wstring::npos && equalsPos > 0) {
                    std::wstring name = entry.substr(0, equalsPos);
                    names.push_back(wideToUtf8(name));
                }

                // 移动到下一个条目
                currentVar += entry.length() + 1;
            }

            return names;
        } else if (scope == EnvVarScope::User || scope == EnvVarScope::System) {
            HKEY rootKey = HKEY_CURRENT_USER;
            std::string keyPath = "Environment";

            // 修正系统环境变量的注册表路径
            if (scope == EnvVarScope::System) {
                rootKey = HKEY_LOCAL_MACHINE;
                keyPath = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
            }

            // 打开注册表键
            std::wstring wKeyPath = utf8ToWide(keyPath);
            HKEY hKey;
            LONG result = RegOpenKeyExW(rootKey, wKeyPath.c_str(), 0, KEY_READ, &hKey);

            if (result != ERROR_SUCCESS) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to open registry key: " +
                                                       getLastErrorMessage());
            }

            // 使用 RAII 确保注册表句柄被关闭
            struct RegKeyCloser {
                HKEY hKey;
                ~RegKeyCloser() {
                    if (hKey)
                        RegCloseKey(hKey);
                }
            } keyCloser{hKey};

            // 获取键值数量
            DWORD valueCount = 0;
            DWORD maxValueNameLength = 0;
            result = RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                      &valueCount, &maxValueNameLength, nullptr, nullptr, nullptr);

            if (result != ERROR_SUCCESS) {
                throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                                   "Failed to query registry key info: " +
                                                       getLastErrorMessage());
            }

            // 分配缓冲区
            maxValueNameLength++;  // 包含结束符的空间
            std::vector<WCHAR> nameBuffer(maxValueNameLength);
            std::vector<std::string> names;

            // 枚举键值
            for (DWORD i = 0; i < valueCount; ++i) {
                DWORD nameSize = maxValueNameLength;
                result = RegEnumValueW(hKey, i, nameBuffer.data(), &nameSize, nullptr, nullptr,
                                       nullptr, nullptr);

                if (result == ERROR_SUCCESS) {
                    std::wstring valueName(nameBuffer.data(), nameSize);
                    names.push_back(wideToUtf8(valueName));
                }
            }

            return names;
        } else {
            throw EnvironmentVariableException(ErrorCode::InvalidArgument,
                                               "Invalid environment variable scope: " +
                                                   std::to_string(static_cast<int>(scope)));
        }
    } catch (const EnvironmentVariableException&) {
        throw;
    } catch (const std::exception& ex) {
        throw EnvironmentVariableException(ErrorCode::EnvironmentVariableError,
                                           "Exception in getNames: " + std::string(ex.what()));
    }
}

std::shared_ptr<IEnvironmentVariables> createEnvironmentVariables() {
    return std::make_shared<EnvironmentVariablesWindows>();
}

}  // namespace system_kit
}  // namespace leigod