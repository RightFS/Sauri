/**
 * @file registry_manager_windows.cpp
 * @brief Windows-specific implementation of the IRegistryManager.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#include "registry_manager_windows.hpp"

#include "common/utils/strings.h"
#include "systemkit/exceptions/exceptions.hpp"
#include "windows_utils.hpp"

#include <map>
#include <string>

namespace leigod {
namespace system_kit {
using namespace utils;
using namespace common;
using namespace common::utils;

namespace {
// std::map<std::string, HKEY> registryMap = {
//     {"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT},
//     {"HKEY_CURRENT_USER", HKEY_CURRENT_USER},
//     {"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE},
//     {"HKEY_USERS", HKEY_USERS},
//     {"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA},
//     {"HKEY_PERFORMANCE_TEXT", HKEY_PERFORMANCE_TEXT},
//     {"HKEY_PERFORMANCE_NLSTEXT", HKEY_PERFORMANCE_NLSTEXT},
//     {"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG},
//     {"HKEY_DYN_DATA", HKEY_DYN_DATA},
//     {"HKEY_CURRENT_USER_LOCAL_SETTINGS", HKEY_CURRENT_USER_LOCAL_SETTINGS},
// };

std::map<std::string, RegistryHive> registryMap = {
    {"HKEY_CLASSES_ROOT", RegistryHive::ClassesRoot},
    {"HKEY_CURRENT_USER", RegistryHive::CurrentUser},
    {"HKEY_LOCAL_MACHINE", RegistryHive::LocalMachine},
    {"HKEY_USERS", RegistryHive::Users},
    //    {"HKEY_PERFORMANCE_DATA", RegistryHive::PerformanceData},
    //    {"HKEY_PERFORMANCE_TEXT", RegistryHive::PerformanceText},
    //    {"HKEY_PERFORMANCE_NLSTEXT", RegistryHive::PerformanceNlsText},
    {"HKEY_CURRENT_CONFIG", RegistryHive::CurrentConfig},
    //    {"HKEY_DYN_DATA", RegistryHive::DynData},
    //    {"HKEY_CURRENT_USER_LOCAL_SETTINGS", RegistryHive::CurrentUserLocalSettings},
};
}  // namespace

using namespace utils;

/**
 * @brief Check if we're running on a 64-bit Windows system
 * @return true if running on 64-bit Windows, false otherwise
 */
bool isWindows64Bit() {
#ifdef _WIN64
    // If we're a 64-bit process, we must be on 64-bit Windows
    return true;
#else
    // Check if we're a 32-bit process running on 64-bit Windows
    BOOL isWow64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &isWow64)) {
        return isWow64 != FALSE;
    }
    return false;  // If IsWow64Process fails, assume 32-bit Windows
#endif
}

/**
 * @brief RAII wrapper for Windows registry keys
 */
class RegistryKeyGuard {
public:
    explicit RegistryKeyGuard(HKEY key) : m_key(key) {}

    ~RegistryKeyGuard() {
        if (m_key != nullptr) {
            RegCloseKey(m_key);
        }
    }

    HKEY get() const {
        return m_key;
    }

    // Prevent copying
    RegistryKeyGuard(const RegistryKeyGuard&) = delete;
    RegistryKeyGuard& operator=(const RegistryKeyGuard&) = delete;

private:
    HKEY m_key;
};

/**
 * @brief Get error message for Windows registry errors
 * @param errorCode Windows error code
 * @return Error message string
 */
std::string getRegistryErrorMessage(LONG errorCode) {
    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_IGNORE_INSERTS,
                                 nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

    std::wstring wmessage(messageBuffer, size);
    LocalFree(messageBuffer);

    // Remove newline characters
    wmessage.erase(std::remove(wmessage.begin(), wmessage.end(), L'\r'), wmessage.end());
    wmessage.erase(std::remove(wmessage.begin(), wmessage.end(), L'\n'), wmessage.end());

    // Convert to UTF-8
    return wideToUtf8(wmessage);
}

// Implementation of RegistryValue methods

std::string RegistryValue::asString() const {
    if (type != RegistryValueType::String && type != RegistryValueType::ExpandString) {
        throw SystemKitException(ErrorCode::RegistryTypeError,
                                 "Registry value is not a string type");
    }

    // Ensure there's at least a null terminator
    if (data.empty()) {
        return {};
    }

    // For wide strings, convert from UTF-16 to UTF-8
    if (data.size() >= 2 && data.size() % 2 == 0) {  // Check if it's likely a wide string
        size_t dataLen = data.size() / sizeof(wchar_t);
        bool hasNullTerminator =
            dataLen > 0 && (data[data.size() - 1] == 0 && data[data.size() - 2] == 0);
        std::wstring wstr(reinterpret_cast<const wchar_t*>(data.data()),
                          dataLen - (hasNullTerminator ? 1 : 0));
        return wideToUtf8(wstr);
    }

    // Convert data to string (excluding null terminator)
    return std::string(reinterpret_cast<const char*>(data.data()),
                       data.size() > 0 && data.back() == 0 ? data.size() - 1 : data.size());
}

uint32_t RegistryValue::asDWord() const {
    if (type != RegistryValueType::DWord) {
        throw SystemKitException(ErrorCode::RegistryTypeError,
                                 "Registry value is not a DWORD type");
    }

    if (data.size() != sizeof(uint32_t)) {
        throw SystemKitException(ErrorCode::RegistryTypeError, "Invalid DWORD data size");
    }

    return *reinterpret_cast<const uint32_t*>(data.data());
}

uint64_t RegistryValue::asQWord() const {
    if (type != RegistryValueType::QWord) {
        throw SystemKitException(ErrorCode::RegistryTypeError,
                                 "Registry value is not a QWORD type");
    }

    if (data.size() != sizeof(uint64_t)) {
        throw SystemKitException(ErrorCode::RegistryTypeError, "Invalid QWORD data size");
    }

    return *reinterpret_cast<const uint64_t*>(data.data());
}

std::vector<std::string> RegistryValue::asMultiString() const {
    if (type != RegistryValueType::MultiString) {
        throw SystemKitException(ErrorCode::RegistryTypeError,
                                 "Registry value is not a multi-string type");
    }

    std::vector<std::string> result;

    // Handle empty data
    if (data.empty()) {
        return result;
    }

    // Check if it's a wide char multi-string
    if (data.size() >= 2 && data.size() % 2 == 0) {
        const auto* currentWStr = reinterpret_cast<const wchar_t*>(data.data());
        const auto* dataEnd = reinterpret_cast<const wchar_t*>(data.data() + data.size());

        while (currentWStr < dataEnd && *currentWStr != L'\0') {
            std::wstring wstr(currentWStr);
            result.push_back(wideToUtf8(wstr));
            currentWStr += wstr.length() + 1;  // Skip to next string
        }
    } else {
        // Parse the multi-string data
        const char* currentStr = reinterpret_cast<const char*>(data.data());
        const char* dataEnd = reinterpret_cast<const char*>(data.data() + data.size());

        while (currentStr < dataEnd && *currentStr != '\0') {
            std::string str(currentStr);
            result.push_back(str);
            currentStr += str.size() + 1;  // Skip to next string
        }
    }

    return result;
}

// Implementation of RegistryManagerWindows

HKEY RegistryManagerWindows::rootKeyToHkey(RegistryHive rootKey) const {
    switch (rootKey) {
        case RegistryHive::ClassesRoot:
            return HKEY_CLASSES_ROOT;
        case RegistryHive::CurrentUser:
            return HKEY_CURRENT_USER;
        case RegistryHive::LocalMachine:
            return HKEY_LOCAL_MACHINE;
        case RegistryHive::Users:
            return HKEY_USERS;
        case RegistryHive::CurrentConfig:
            return HKEY_CURRENT_CONFIG;
        default:
            throw std::invalid_argument("Invalid registry root key");
    }
}

RegistryValueType RegistryManagerWindows::winTypeToValueType(DWORD winType) const {
    switch (winType) {
        case REG_SZ:
            return RegistryValueType::String;
        case REG_EXPAND_SZ:
            return RegistryValueType::ExpandString;
        case REG_BINARY:
            return RegistryValueType::Binary;
        case REG_DWORD:
            return RegistryValueType::DWord;
        case REG_QWORD:
            return RegistryValueType::QWord;
        case REG_MULTI_SZ:
            return RegistryValueType::MultiString;
        default:
            return RegistryValueType::Binary;  // Default to binary for unknown types
    }
}

DWORD RegistryManagerWindows::valueTypeToWinType(RegistryValueType type) const {
    switch (type) {
        case RegistryValueType::String:
            return REG_SZ;
        case RegistryValueType::ExpandString:
            return REG_EXPAND_SZ;
        case RegistryValueType::Binary:
            return REG_BINARY;
        case RegistryValueType::DWord:
            return REG_DWORD;
        case RegistryValueType::QWord:
            return REG_QWORD;
        case RegistryValueType::MultiString:
            return REG_MULTI_SZ;
        default:
            throw RegistryManagerException(ErrorCode::InvalidArgument,
                                           "Invalid registry root key: " +
                                               std::to_string(static_cast<int>(type)));
    }
}

DWORD RegistryManagerWindows::registryViewToFlags(RegistryView view) const {
    // If not on 64-bit Windows, registry view flags have no effect and should be 0
    if (!isWindows64Bit()) {
        return 0;
    }

    switch (view) {
        case RegistryView::Default:
            return 0;  // Use default based on process architecture
        case RegistryView::Force32Bit:
            return KEY_WOW64_32KEY;  // Force access to 32-bit view (WOW6432Node)
        case RegistryView::Force64Bit:
            return KEY_WOW64_64KEY;  // Force access to 64-bit view (native path)
        default:
            return 0;
    }
}

HKEY RegistryManagerWindows::openKey(RegistryHive rootKey, const std::string& subKey,
                                     RegistryAccess access, RegistryView view) const {
    try {
        HKEY hRootKey = rootKeyToHkey(rootKey);
        DWORD desiredAccess = 0;

        switch (access) {
            case RegistryAccess::Read:
                desiredAccess = KEY_READ;
                break;
            case RegistryAccess::Write:
                desiredAccess = KEY_WRITE;
                break;
            case RegistryAccess::ReadWrite:
                desiredAccess = KEY_READ | KEY_WRITE;
                break;
            default:
                throw RegistryManagerException(ErrorCode::InvalidArgument,
                                               "Invalid registry access mode :" +
                                                   std::to_string(static_cast<int>(access)));
        }

        // Add registry view flags
        desiredAccess |= registryViewToFlags(view);

        // 转换子键为宽字符
        std::wstring wSubKey;
        wSubKey = utf8ToWide(subKey);

        HKEY hKey;
        LONG result = RegOpenKeyExW(hRootKey, wSubKey.c_str(), 0, desiredAccess, &hKey);

        if (result != ERROR_SUCCESS) {
            if (result == ERROR_FILE_NOT_FOUND) {
                throw RegistryManagerException(ErrorCode::RegistryKeyNotFound,
                                               "Registry key not found: " + subKey);
            } else if (result == ERROR_ACCESS_DENIED) {
                // If access is denied, the key exists but we can't access it
                throw RegistryManagerException(ErrorCode::RegistryAccessDenied,
                                               "Registry access denied: " + subKey);
            } else {
                throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                               "Failed to open registry key: " +
                                                   getRegistryErrorMessage(result));
            }
        }

        return hKey;
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in openKey: " + std::string(ex.what()));
    }
}

bool RegistryManagerWindows::keyExists(RegistryHive rootKey, const std::string& subKey,
                                       RegistryView view) const {
    try {
        HKEY hRootKey = rootKeyToHkey(rootKey);

        // 转换子键为宽字符
        std::wstring wSubKey;
        wSubKey = utf8ToWide(subKey);

        HKEY hKey;
        LONG result = RegOpenKeyExW(hRootKey, wSubKey.c_str(), 0,
                                    KEY_READ | registryViewToFlags(view), &hKey);

        if (result == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        } else if (result == ERROR_FILE_NOT_FOUND) {
            return false;
        } else if (result == ERROR_ACCESS_DENIED) {
            // If access is denied, the key exists but we can't access it
            return true;
        } else {
            throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                           "Failed to check if registry key exists: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in keyExists: " + std::string(ex.what()));
    }
}

void RegistryManagerWindows::createKey(RegistryHive rootKey, const std::string& subKey,
                                       RegistryView view) {
    try {
        HKEY hRootKey = rootKeyToHkey(rootKey);

        // 转换子键为宽字符
        std::wstring wSubKey;
        wSubKey = utf8ToWide(subKey);

        HKEY hKey;
        DWORD disposition;
        LONG result =
            RegCreateKeyExW(hRootKey, wSubKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                            KEY_WRITE | registryViewToFlags(view), nullptr, &hKey, &disposition);

        if (result != ERROR_SUCCESS) {
            if (result == ERROR_ACCESS_DENIED) {
                throw RegistryManagerException(ErrorCode::RegistryAccessDenied,
                                               "Access denied when creating registry key: " +
                                                   subKey);
            } else {
                throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                               "Failed to create registry key: " +
                                                   getRegistryErrorMessage(result));
            }
        }

        RegCloseKey(hKey);
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in createKey: " + std::string(ex.what()));
    }
}

// 递归删除注册表键及其所有子键
LONG deleteKeyRecursively(HKEY hRootKey, const std::wstring& subKey, REGSAM samDesired) {
    // 先尝试直接打开键
    HKEY hKey;
    LONG result = RegOpenKeyExW(hRootKey, subKey.c_str(), 0,
                                KEY_READ | KEY_ENUMERATE_SUB_KEYS | samDesired, &hKey);

    if (result != ERROR_SUCCESS) {
        return result;  // 如果无法打开键，返回错误
    }

    // 枚举并删除所有子键
    wchar_t childKeyName[MAX_PATH];
    DWORD childKeyNameSize;

    // 注意：这里采用反复删除第一个子键的方法，因为删除后索引会变化
    while (true) {
        childKeyNameSize = MAX_PATH;
        result = RegEnumKeyExW(hKey, 0, childKeyName, &childKeyNameSize, nullptr, nullptr, nullptr,
                               nullptr);

        if (result == ERROR_NO_MORE_ITEMS) {
            break;  // 没有更多子键，退出循环
        } else if (result != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return result;  // 枚举失败，返回错误
        }

        // 递归删除子键
        result = deleteKeyRecursively(hKey, childKeyName, samDesired);
        if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
            RegCloseKey(hKey);
            return result;
        }
    }

    RegCloseKey(hKey);

    // 所有子键删除后，删除当前键
    result = RegDeleteKeyExW(hRootKey, subKey.c_str(), samDesired, 0);
    return result;
}

void RegistryManagerWindows::deleteKey(RegistryHive rootKey, const std::string& subKey,
                                       RegistryView view) {
    try {
        HKEY hRootKey = rootKeyToHkey(rootKey);

        // 转换子键为宽字符
        std::wstring wSubKey;
        wSubKey = utf8ToWide(subKey);
        LONG result = deleteKeyRecursively(hRootKey, wSubKey, registryViewToFlags(view));

        if (result != ERROR_SUCCESS) {
            if (result == ERROR_FILE_NOT_FOUND) {
                throw RegistryManagerException(ErrorCode::RegistryKeyNotFound,
                                               "Registry key not found: " + subKey);
            } else if (result == ERROR_ACCESS_DENIED) {
                throw RegistryManagerException(ErrorCode::RegistryAccessDenied,
                                               "Access denied when deleting registry key: " +
                                                   subKey);
            } else {
                throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                               "Failed to delete registry key: " +
                                                   getRegistryErrorMessage(result));
            }
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in deleteKey: " + std::string(ex.what()));
    }
}

std::vector<std::string> RegistryManagerWindows::getSubKeys(RegistryHive rootKey,
                                                            const std::string& subKey,
                                                            RegistryView view) const {
    try {
        auto key = openKey(rootKey, subKey, RegistryAccess::Read, view);
        RegistryKeyGuard keyGuard(key);
        std::vector<std::string> subKeys;
        wchar_t nameBuffer[256];
        DWORD nameSize;
        DWORD index = 0;
        while (true) {
            nameSize = sizeof(nameBuffer) / sizeof(nameBuffer[0]);
            LONG result = RegEnumKeyExW(keyGuard.get(), index, nameBuffer, &nameSize, nullptr,
                                        nullptr, nullptr, nullptr);

            if (result == ERROR_NO_MORE_ITEMS) {
                break;
            } else if (result != ERROR_SUCCESS) {
                throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                               "Failed to enumerate registry subkeys: " +
                                                   getRegistryErrorMessage(result));
            }

            // 转换宽字符为UTF-8
            subKeys.push_back(wideToUtf8(std::wstring(nameBuffer, nameSize)));
            index++;
        }

        return subKeys;
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in getSubKeys: " + std::string(ex.what()));
    }
}

bool RegistryManagerWindows::valueExists(RegistryHive rootKey, const std::string& subKey,
                                         const std::string& valueName, RegistryView view) const {
    try {
        HKEY__* key = nullptr;
        try {
            key = openKey(rootKey, subKey, RegistryAccess::Read, view);
        } catch (const RegistryManagerException& e) {
            if (e.code() == static_cast<int>(ErrorCode::RegistryKeyNotFound)) {
                return false;
            }
            throw;
        }

        RegistryKeyGuard keyGuard(key);
        // 转换值名称为宽字符
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        DWORD type;
        LONG result =
            RegQueryValueExW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(),
                             nullptr, &type, nullptr, nullptr);

        if (result == ERROR_SUCCESS) {
            return true;
        } else if (result == ERROR_FILE_NOT_FOUND) {
            return false;
        } else {
            throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                           "Failed to check if registry value exists: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in valueExists: " + std::string(ex.what()));
    }
}

RegistryValue RegistryManagerWindows::getValue(RegistryHive rootKey, const std::string& subKey,
                                               const std::string& valueName,
                                               bool expandEnvironmentVars,
                                               RegistryView view) const {
    try {
        auto key = openKey(rootKey, subKey, RegistryAccess::Read, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name to wide string
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        // First query to get the type and size
        DWORD type;
        DWORD dataSize = 0;
        LONG result =
            RegQueryValueExW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(),
                             nullptr, &type, nullptr, &dataSize);

        if (result == ERROR_FILE_NOT_FOUND) {
            throw RegistryManagerException(ErrorCode::RegistryKeyNotFound,
                                           "Registry value not found: " + valueName);
        } else if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
            // If the result is not ERROR_SUCCESS or ERROR_MORE_DATA, it indicates an error
            throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                           "Failed to query registry value size: " +
                                               getRegistryErrorMessage(result));
        }

        // Allocate buffer and retrieve the actual data
        std::vector<uint8_t> data(dataSize);
        result = RegQueryValueExW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(),
                                  nullptr, &type, data.data(), &dataSize);

        if (result != ERROR_SUCCESS) {
            if (result == ERROR_FILE_NOT_FOUND) {
                throw RegistryManagerException(ErrorCode::RegistryKeyNotFound,
                                               "Registry value not found: " + valueName);
            } else {
                throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                               "Failed to query registry value data: " +
                                                   getRegistryErrorMessage(result));
            }
        }

        RegistryValue value;
        value.name = valueName;
        value.type = winTypeToValueType(type);

        // Expand environment variables in string values
        if (expandEnvironmentVars && type == REG_EXPAND_SZ && dataSize >= 2) {
            // 确保数据是一个有效的宽字符串
            const auto* wideStr = reinterpret_cast<const wchar_t*>(data.data());
            size_t wideStrLen = (dataSize / sizeof(wchar_t)) - 1;  // 减去结尾的 null

            if (wideStrLen > 0) {
                std::wstring expandedStr;

                // First calculate the size needed for the expanded string
                DWORD requiredSize = ExpandEnvironmentStringsW(wideStr, nullptr, 0);
                if (requiredSize > 0) {
                    expandedStr.resize(requiredSize);

                    // Perform the actual environment variable expansion
                    DWORD expandResult =
                        ExpandEnvironmentStringsW(wideStr, &expandedStr[0], requiredSize);

                    if (expandResult > 0 && expandResult <= requiredSize) {
                        // The size returned by ExpandEnvironmentStringsW includes the null
                        // terminator
                        expandedStr.resize(expandResult - 1);

                        try {
                            value.data.assign(reinterpret_cast<const uint8_t*>(expandedStr.data()),
                                              reinterpret_cast<const uint8_t*>(
                                                  expandedStr.data() + expandedStr.size() + 1));

                            // Update type to String (after expansion it's a normal string)
                            value.type = RegistryValueType::String;

                            // Record original type
                            value.originalType = RegistryValueType::ExpandString;
                        } catch ([[maybe_unused]] const std::exception& ex) {
                            // If conversion fails, keep the original data
                            value.data = std::move(data);
                        }

                        return value;
                    }
                }
            }
        }

        value.data = std::move(data);
        return value;
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in getValue: " + std::string(ex.what()));
    }
}

std::vector<RegistryItem> RegistryManagerWindows::getItems(RegistryHive rootKey,
                                                           const std::string& subKey,
                                                           RegistryView view) const {
    try {
        auto key = openKey(rootKey, subKey, RegistryAccess::Read, view);
        RegistryKeyGuard keyGuard(key);
        std::vector<RegistryItem> items;
        // Query the number of values and maximum value name length in the key
        DWORD valueCount = 0;
        DWORD maxValueNameLen = 0;
        DWORD maxValueLen = 0;

        LONG result = RegQueryInfoKeyW(keyGuard.get(),
                                       nullptr,           // 类名
                                       nullptr,           // 类名大小
                                       nullptr,           // 保留
                                       nullptr,           // 子键数量
                                       nullptr,           // 最大子键名长度
                                       nullptr,           // 最大子键类名长度
                                       &valueCount,       // 值数量
                                       &maxValueNameLen,  // 最大值名长度
                                       &maxValueLen,      // 最大值长度
                                       nullptr,           // 安全描述符
                                       nullptr            // 最后写入时间
        );

        if (result != ERROR_SUCCESS) {
            throw RegistryManagerException(ErrorCode::RegistryQueryFailed,
                                           "Failed to query registry key info: " +
                                               getRegistryErrorMessage(result));
        }

        // Allocate buffer (add 1 for null terminator)
        std::vector<wchar_t> valueNameBuffer(maxValueNameLen + 1);

        // Enumerate all values
        for (DWORD i = 0; i < valueCount; i++) {
            DWORD valueNameSize = maxValueNameLen + 1;
            DWORD type = 0;
            DWORD dataSize = 0;

            // Get value name, type, and data size
            result = RegEnumValueW(keyGuard.get(), i, valueNameBuffer.data(), &valueNameSize,
                                   nullptr,   // 保留
                                   &type,     // 值类型
                                   nullptr,   // 不需要数据
                                   &dataSize  // 数据大小
            );

            if (result != ERROR_SUCCESS) {
                continue;  // Skip this value and continue enumeration
            }

            // 创建值项
            RegistryItem item;
            item.name = wideToUtf8(std::wstring(valueNameBuffer.data(), valueNameSize));
            item.type = windowsTypeToValueType(type);
            item.dataSize = dataSize;

            items.push_back(item);
        }

        return items;
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in getItems: " + std::string(ex.what()));
    }
}

std::vector<std::string> RegistryManagerWindows::getValueNames(RegistryHive rootKey,
                                                               const std::string& subKey,
                                                               RegistryView view) const {
    try {
        auto key = openKey(rootKey, subKey, RegistryAccess::Read, view);
        RegistryKeyGuard keyGuard(key);
        std::vector<std::string> valueNames;

        DWORD maxValueNameLen = 0;
        LONG ret = RegQueryInfoKeyW(keyGuard.get(), nullptr, nullptr, nullptr, nullptr, nullptr,
                                    nullptr, nullptr, &maxValueNameLen, nullptr, nullptr, nullptr);
        if (ret != ERROR_SUCCESS) {
            throw RegistryManagerException(ErrorCode::RegistryQueryFailed,
                                           "Failed to query maximum value name length");
        }
        std::vector<wchar_t> nameBuffer(maxValueNameLen + 1);
        DWORD nameSize;
        DWORD index = 0;

        while (true) {
            nameSize = maxValueNameLen + 1;
            LONG result = RegEnumValueW(keyGuard.get(), index, nameBuffer.data(), &nameSize,
                                        nullptr, nullptr, nullptr, nullptr);

            if (result == ERROR_NO_MORE_ITEMS) {
                break;
            } else if (result != ERROR_SUCCESS) {
                throw RegistryManagerException(ErrorCode::RegistryEnumFailed,
                                               "Failed to enumerate registry values: " +
                                                   getRegistryErrorMessage(result));
            }

            // Convert wide characters to UTF-8
            if (nameSize > 0) {
                valueNames.push_back(wideToUtf8(std::wstring(nameBuffer.data(), nameSize)));
            } else {
                // Default value
                valueNames.emplace_back("");
            }

            index++;
        }

        return valueNames;
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in getValueNames: " + std::string(ex.what()));
    }
}

RegistryValueType RegistryManagerWindows::getValueType(RegistryHive rootKey,
                                                       const std::string& subKey,
                                                       const std::string& valueName,
                                                       RegistryView view) const {
    try {
        auto key = openKey(rootKey, subKey, RegistryAccess::Read, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name to wide string
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);

        // Query value type
        DWORD type;
        DWORD dataSize = 0;

        // Call RegQueryValueEx to get the type
        LONG result = RegQueryValueExW(keyGuard.get(), wValueName.c_str(),
                                       nullptr,   // reserved
                                       &type,     // 类型输出
                                       nullptr,   // 不需要数据
                                       &dataSize  // 数据大小输出
        );

        if (result == ERROR_SUCCESS) {
            // Convert Windows type to our enum type
            return windowsTypeToValueType(type);
        } else if (result == ERROR_FILE_NOT_FOUND) {
            throw RegistryManagerException(ErrorCode::RegistryValueNotFound,
                                           "Registry value not found: " + valueName);
        } else {
            // If the result is not ERROR_SUCCESS, it indicates an error
            // or the value doesn't exist
            throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                           "Failed to query registry value type: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in getValueType: " + std::string(ex.what()));
    }
}

void RegistryManagerWindows::setString(RegistryHive rootKey, const std::string& subKey,
                                       const std::string& valueName, const std::string& value,
                                       bool expandable, RegistryView view) {
    try {
        autoCreateKey(rootKey, subKey, view);
        auto key = openKey(rootKey, subKey, RegistryAccess::Write, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name and value to wide string
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        std::wstring wValue = utf8ToWide(value);
        DWORD type = expandable ? REG_EXPAND_SZ : REG_SZ;
        LONG result = RegSetValueExW(
            keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(), 0, type,
            reinterpret_cast<const BYTE*>(wValue.c_str()),
            static_cast<DWORD>((wValue.length() + 1) * sizeof(wchar_t))  // Include null terminator
        );

        if (result != ERROR_SUCCESS) {
            throw RegistryManagerException(ErrorCode::RegistrySetValueFailed,
                                           "Failed to set registry string value: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in setString: " + std::string(ex.what()));
    }
}

void RegistryManagerWindows::setDWord(RegistryHive rootKey, const std::string& subKey,
                                      const std::string& valueName, uint32_t value,
                                      RegistryView view) {
    try {
        autoCreateKey(rootKey, subKey, view);
        auto key = openKey(rootKey, subKey, RegistryAccess::Write, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name to wide string
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        LONG result =
            RegSetValueExW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(), 0,
                           REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));

        if (result != ERROR_SUCCESS) {
            throw RegistryManagerException(ErrorCode::RegistrySetValueFailed,
                                           "Failed to set registry DWORD value: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in setDWord: " + std::string(ex.what()));
    }
}

void RegistryManagerWindows::setQWord(RegistryHive rootKey, const std::string& subKey,
                                      const std::string& valueName, uint64_t value,
                                      RegistryView view) {
    try {
        autoCreateKey(rootKey, subKey, view);
        auto key = openKey(rootKey, subKey, RegistryAccess::Write, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name to wide string
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        LONG result =
            RegSetValueExW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(), 0,
                           REG_QWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));

        if (result != ERROR_SUCCESS) {
            throw RegistryManagerException(ErrorCode::RegistrySetValueFailed,
                                           "Failed to set registry QWORD value: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in setQWord: " + std::string(ex.what()));
    }
}

void RegistryManagerWindows::setBinary(RegistryHive rootKey, const std::string& subKey,
                                       const std::string& valueName,
                                       const std::vector<uint8_t>& data, RegistryView view) {
    try {
        autoCreateKey(rootKey, subKey, view);
        auto key = openKey(rootKey, subKey, RegistryAccess::Write, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name to wide string
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        LONG result =
            RegSetValueExW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(), 0,
                           REG_BINARY, data.data(), static_cast<DWORD>(data.size()));

        if (result != ERROR_SUCCESS) {
            throw RegistryManagerException(ErrorCode::RegistrySetValueFailed,
                                           "Failed to set registry binary value: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in setBinary: " + std::string(ex.what()));
    }
}

void RegistryManagerWindows::setMultiString(RegistryHive rootKey, const std::string& subKey,
                                            const std::string& valueName,
                                            const std::vector<std::string>& values,
                                            RegistryView view) {
    try {
        autoCreateKey(rootKey, subKey, view);
        auto key = openKey(rootKey, subKey, RegistryAccess::Write, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name and value list to wide characters
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        std::vector<std::wstring> wValues;
        for (const auto& val : values) {
            wValues.push_back(utf8ToWide(val));
        }

        // Calculate total size needed
        size_t totalSize = 1;  // Final null terminator (in wchar_t units)
        for (const auto& wstr : wValues) {
            totalSize += wstr.length() + 1;  // Each string plus its null terminator
        }

        // Create the multi-string buffer (in wchar_t units)
        std::vector<wchar_t> buffer(totalSize, 0);
        wchar_t* currentPos = buffer.data();

        // Fill the buffer with strings and null terminators
        for (const auto& wstr : wValues) {
            wcscpy_s(currentPos, wstr.length() + 1, wstr.c_str());
            currentPos += wstr.length() + 1;
        }

        // Last string is followed by an additional null terminator (already zeroed)

        LONG result =
            RegSetValueExW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str(), 0,
                           REG_MULTI_SZ, reinterpret_cast<const BYTE*>(buffer.data()),
                           static_cast<DWORD>(buffer.size() * sizeof(wchar_t)));

        if (result != ERROR_SUCCESS) {
            throw RegistryManagerException(ErrorCode::RegistrySetValueFailed,
                                           "Failed to set registry multi-string value: " +
                                               getRegistryErrorMessage(result));
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in setMultiString: " + std::string(ex.what()));
    }
}

void RegistryManagerWindows::deleteValue(RegistryHive rootKey, const std::string& subKey,
                                         const std::string& valueName, RegistryView view) {
    try {
        auto key = openKey(rootKey, subKey, RegistryAccess::Write, view);
        RegistryKeyGuard keyGuard(key);

        // Convert value name to wide string
        std::wstring wValueName = valueName.empty() ? std::wstring() : utf8ToWide(valueName);
        LONG result =
            RegDeleteValueW(keyGuard.get(), wValueName.empty() ? nullptr : wValueName.c_str());

        if (result != ERROR_SUCCESS) {
            if (result == ERROR_FILE_NOT_FOUND) {
                return;  // Value not found, nothing to delete
            } else {
                throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                               "Failed to delete registry value: " +
                                                   getRegistryErrorMessage(result));
            }
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryError,
                                       "Exception in deleteValue: " + std::string(ex.what()));
    }
}

// Helper method for type conversion
RegistryValueType RegistryManagerWindows::windowsTypeToValueType(DWORD winType) const {
    switch (winType) {
        case REG_NONE:
            return RegistryValueType::None;
        case REG_SZ:
            return RegistryValueType::String;
        case REG_EXPAND_SZ:
            return RegistryValueType::ExpandString;
        case REG_BINARY:
            return RegistryValueType::Binary;
        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
            return RegistryValueType::DWord;
        case REG_QWORD:
            return RegistryValueType::QWord;
        case REG_MULTI_SZ:
            return RegistryValueType::MultiString;
        default:
            return RegistryValueType::Unknown;
    }
}

std::pair<RegistryHive, std::string> RegistryManagerWindows::parsePath(const std::string& path) {
    // Find the position of the first backslash
    size_t pos = path.find_first_of("\\/");

    // Check if the backslash is found
    if (pos == std::string::npos) {
        throw RegistryManagerException(ErrorCode::InvalidArgument,
                                       "Invalid registry path: " + path);
    }

    // Split the registry path into root key and subkey
    std::string rootKey = path.substr(0, pos);
    std::string subKey = path.substr(pos + 1);

    auto it = registryMap.find(rootKey);
    if (it == registryMap.end()) {
        throw RegistryManagerException(ErrorCode::InvalidArgument,
                                       "Invalid registry root key: " + rootKey);
    }

    return std::make_pair(it->second, subKey);
}

void RegistryManagerWindows::autoCreateKey(RegistryHive rootKey, const std::string& subKey,
                                           RegistryView view) {
    try {
        // 如果路径为空，无需创建
        if (subKey.empty()) {
            return;
        }

        HKEY hRootKey = rootKeyToHkey(rootKey);
        std::wstring wSubKey = utf8ToWide(subKey);

        // 查找所有的子键分隔符位置
        std::vector<size_t> separatorPositions;
        for (size_t i = 0; i < wSubKey.length(); ++i) {
            if (wSubKey[i] == L'\\') {
                separatorPositions.push_back(i);
            }
        }

        // 逐级创建子键
        HKEY hCurrentKey = nullptr;
        DWORD disposition;

        // 为了优化性能，尝试先直接创建完整路径
        LONG result = RegCreateKeyExW(
            hRootKey, wSubKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
            KEY_WRITE | registryViewToFlags(view), nullptr, &hCurrentKey, &disposition);

        // 如果成功，则直接返回
        if (result == ERROR_SUCCESS) {
            RegCloseKey(hCurrentKey);
            return;
        }

        // 如果直接创建失败，则逐级创建
        std::wstring currentPath;
        for (size_t i = 0; i <= separatorPositions.size(); ++i) {
            // 构建当前级别的路径
            if (i == 0 && !separatorPositions.empty()) {
                currentPath = wSubKey.substr(0, separatorPositions[i]);
            } else if (i < separatorPositions.size()) {
                currentPath = wSubKey.substr(0, separatorPositions[i]);
            } else {
                currentPath = wSubKey;
            }

            // 创建当前级别的键
            result = RegCreateKeyExW(hRootKey, currentPath.c_str(), 0, nullptr,
                                     REG_OPTION_NON_VOLATILE, KEY_WRITE | registryViewToFlags(view),
                                     nullptr, &hCurrentKey, &disposition);

            // 检查结果
            if (result != ERROR_SUCCESS) {
                if (result == ERROR_ACCESS_DENIED) {
                    throw RegistryManagerException(ErrorCode::RegistryAccessDenied,
                                                   "Access denied when creating registry key: " +
                                                       subKey);
                } else {
                    throw RegistryManagerException(ErrorCode::RegistryCreateFailed,
                                                   "Failed to create registry key: " +
                                                       getRegistryErrorMessage(result));
                }
            }

            // 每一级都关闭句柄
            RegCloseKey(hCurrentKey);
            hCurrentKey = nullptr;
        }
    } catch (const RegistryManagerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw RegistryManagerException(ErrorCode::RegistryOperationFailed,
                                       "Exception in autoCreateKey: " + std::string(ex.what()));
    }
}

std::shared_ptr<IRegistryManager> createRegistryManager() {
    return std::make_shared<RegistryManagerWindows>();
}

}  // namespace system_kit
}  // namespace leigod