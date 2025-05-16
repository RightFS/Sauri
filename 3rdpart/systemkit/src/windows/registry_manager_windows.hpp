/**
 * @file registry_manager_windows.hpp
 * @brief Windows-specific implementation of the IRegistryManager.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_WINDOWS_REGISTRY_MANAGER_WINDOWS_HPP
#define LEIGOD_SRC_WINDOWS_REGISTRY_MANAGER_WINDOWS_HPP

#include "systemkit/registry/registry_manager.hpp"

#include <Windows.h>
#include <map>
#include <string>

namespace leigod {
namespace system_kit {

/**
 * @class RegistryManagerWindows
 * @brief Windows platform implementation of registry management
 *
 * This class provides access to the Windows Registry API to manage
 * registry keys and values on Windows platforms. Internally uses
 * wide character API functions to fully support Unicode (including
 * Chinese characters) and prevents encoding issues.
 *
 * Supports accessing both 32-bit and 64-bit registry views on 64-bit Windows.
 */
class RegistryManagerWindows : public IRegistryManager {
public:
    /**
     * @brief Constructor
     */
    RegistryManagerWindows() = default;

    /**
     * @brief Destructor
     */
    ~RegistryManagerWindows() override = default;

    /**
     * @brief Check if a registry key exists
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param view Registry view to access (32-bit or 64-bit)
     * @return bool True if the key exists, false otherwise
     * @throw RegistryManagerException
     */
    bool keyExists(RegistryHive rootKey, const std::string& subKey,
                   RegistryView view = RegistryView::Default) const override;

    /**
     * @brief Create a registry key
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path to create
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the key was created or already exists
     * @throw RegistryManagerException
     */
    void createKey(RegistryHive rootKey, const std::string& subKey,
                   RegistryView view = RegistryView::Default) override;

    /**
     * @brief Delete a registry key and all its subkeys
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path to delete
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the key was deleted
     * @throw RegistryManagerException
     */
    void deleteKey(RegistryHive rootKey, const std::string& subKey,
                   RegistryView view = RegistryView::Default) override;

    /**
     * @brief Get a list of subkeys in a registry key
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param view Registry view to access (32-bit or 64-bit)
     * @return std::vector<std::string>> List of subkey names
     * @throw RegistryManagerException
     */
    std::vector<std::string> getSubKeys(RegistryHive rootKey, const std::string& subKey,
                                        RegistryView view = RegistryView::Default) const override;

    /**
     * @brief Check if a registry value exists
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to check
     * @param view Registry view to access (32-bit or 64-bit)
     * @return bool True if the value exists, false otherwise
     * @throw RegistryManagerException
     */
    bool valueExists(RegistryHive rootKey, const std::string& subKey, const std::string& valueName,
                     RegistryView view = RegistryView::Default) const override;

    /**
     * @brief Get a registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to get (empty for default value)
     * @param view Registry view to access (32-bit or 64-bit)
     * @return RegistryValue The registry value if successful
     * @throw RegistryManagerException
     */
    RegistryValue getValue(RegistryHive rootKey, const std::string& subKey,
                           const std::string& valueName = "", bool expandEnvironmentVars = false,
                           RegistryView view = RegistryView::Default) const override;

    /**
     * @brief Get all values under a registry key
     *
     * @param hive The registry hive
     * @param keyPath Path to the key
     * @return std::vector<RegistryItem>> List of values with their types if successful
     * @throw RegistryManagerException
     */
    std::vector<RegistryItem> getItems(RegistryHive rootKey, const std::string& subKey,
                                       RegistryView view = RegistryView::Default) const override;

    /**
     * @brief Get a list of value names in a registry key
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param view Registry view to access (32-bit or 64-bit)
     * @return std::vector<std::string> List of value names
     * @throw RegistryManagerException
     */
    std::vector<std::string>
    getValueNames(RegistryHive rootKey, const std::string& subKey,
                  RegistryView view = RegistryView::Default) const override;

    /**
     * @brief Get the type of a registry value
     *
     * @param hive The registry hive
     * @param keyPath Path to the key containing the value
     * @param valueName Name of the value to check
     * @return RegistryValueType The type of the value if successful
     * @throw RegistryManagerException
     */
    RegistryValueType getValueType(RegistryHive hive, const std::string& keyPath,
                                   const std::string& valueName,
                                   RegistryView view = RegistryView::Default) const override;

    /**
     * @brief Set a string registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set (empty for default value)
     * @param value The string value to set
     * @param expandable True to create an expandable string (with environment variables)
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the value was set
     * @throw RegistryManagerException
     */
    void setString(RegistryHive rootKey, const std::string& subKey, const std::string& valueName,
                   const std::string& value, bool expandable = false,
                   RegistryView view = RegistryView::Default) override;

    /**
     * @brief Set a DWORD (32-bit integer) registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param value The 32-bit integer value to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the value was set
     * @throw RegistryManagerException
     */
    void setDWord(RegistryHive rootKey, const std::string& subKey, const std::string& valueName,
                  uint32_t value, RegistryView view = RegistryView::Default) override;

    /**
     * @brief Set a QWORD (64-bit integer) registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param value The 64-bit integer value to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the value was set
     * @throw RegistryManagerException
     */
    void setQWord(RegistryHive rootKey, const std::string& subKey, const std::string& valueName,
                  uint64_t value, RegistryView view = RegistryView::Default) override;

    /**
     * @brief Set a binary registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param data The binary data to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the value was set
     * @throw RegistryManagerException
     */
    void setBinary(RegistryHive rootKey, const std::string& subKey, const std::string& valueName,
                   const std::vector<uint8_t>& data,
                   RegistryView view = RegistryView::Default) override;

    /**
     * @brief Set a multi-string registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param values The vector of strings to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the value was set
     * @throw RegistryManagerException
     */
    void setMultiString(RegistryHive rootKey, const std::string& subKey,
                        const std::string& valueName, const std::vector<std::string>& values,
                        RegistryView view = RegistryView::Default) override;

    /**
     * @brief Delete a registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to delete
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the value was deleted
     * @throw RegistryManagerException
     */
    void deleteValue(RegistryHive rootKey, const std::string& subKey, const std::string& valueName,
                     RegistryView view = RegistryView::Default) override;

    /**
     * @brief Splits the registry path into root key and subkey.
     *
     * @param path The full registry path to split.
     * @return std::pair<RegistryHive, std::string> The root key and subkey pair if
     * successful
     * @throw RegistryManagerException
     */
    std::pair<RegistryHive, std::string> parsePath(const std::string& path) override;

private:
    /**
     * @brief Convert RegistryHive to Windows HKEY
     * @param rootKey The registry root key enum value
     * @return HKEY The Windows HKEY value
     */
    HKEY rootKeyToHkey(RegistryHive rootKey) const;

    /**
     * @brief Convert Windows registry type to RegistryValueType
     * @param winType The Windows registry type
     * @return RegistryValueType The registry value type enum
     */
    RegistryValueType winTypeToValueType(DWORD winType) const;

    /**
     * @brief Convert RegistryValueType to Windows registry type
     * @param type The registry value type enum
     * @return DWORD The Windows registry type
     */
    DWORD valueTypeToWinType(RegistryValueType type) const;

    /**
     * @brief Convert RegistryView to Windows access flags
     * @param view The registry view enum value
     * @return DWORD The Windows registry access flags
     */
    DWORD registryViewToFlags(RegistryView view) const;

    /**
     * @brief Open a registry key with specified access rights
     * @param rootKey The root key
     * @param subKey The subkey path
     * @param access The desired access rights
     * @param view Registry view to access (32-bit or 64-bit)
     * @return HKEY Handle to the opened key if successful
     * @throw RegistryManagerException
     */
    HKEY openKey(RegistryHive rootKey, const std::string& subKey, RegistryAccess access,
                 RegistryView view = RegistryView::Default) const;

    /**
     * @brief Get the value of a registry key
     * @param key The registry key handle
     * @param valueName The name of the value to get
     * @return RegistryValue> The registry value if successful
     */
    RegistryValueType windowsTypeToValueType(DWORD winType) const;

    /**
     * @brief Auto-create a registry key if it does not exist
     * @param rootKey The root key
     * @param subKey The subkey path
     * @param view The registry view
     *
     * @throw RegistryManagerException
     */
    void autoCreateKey(RegistryHive rootKey, const std::string& subKey, RegistryView view);
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_WINDOWS_REGISTRY_MANAGER_WINDOWS_HPP