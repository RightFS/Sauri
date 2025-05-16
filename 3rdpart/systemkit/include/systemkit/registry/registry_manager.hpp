/**
 * @file registry_manager.hpp
 * @brief Abstract base class for platform-specific register managers.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_MANAGER_INCLUDE_REGISTRY_MANAGER_HPP
#define LEIGOD_SRC_MANAGER_INCLUDE_REGISTRY_MANAGER_HPP

#include "systemkit/exceptions/exceptions.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace leigod {
namespace system_kit {

/**
 * @brief Registry value data types
 */
enum class RegistryValueType {
    None,          ///< REG_NONE
    String,        ///< String value (REG_SZ in Windows)
    ExpandString,  ///< Expandable string with environment variables (REG_EXPAND_SZ in Windows)
    Binary,        ///< Binary data (REG_BINARY in Windows)
    DWord,         ///< 32-bit number (REG_DWORD in Windows)
    QWord,         ///< 64-bit number (REG_QWORD in Windows)
    MultiString,   ///< Multi-string value (REG_MULTI_SZ in Windows)
    Unknown        ///< Other types
};

/**
 * @brief Predefined registry root keys
 */
enum class RegistryHive {
    ClassesRoot,   ///< HKEY_CLASSES_ROOT
    CurrentUser,   ///< HKEY_CURRENT_USER
    LocalMachine,  ///< HKEY_LOCAL_MACHINE
    Users,         ///< HKEY_USERS
    CurrentConfig  ///< HKEY_CURRENT_CONFIG
};

/**
 * @brief Registry view (32-bit or 64-bit)
 *
 * On 64-bit Windows, registry has two views: a 32-bit view (WOW6432Node)
 * and a 64-bit view (native path). This enum controls which view to access.
 */
enum class RegistryView {
    Default,     ///< Use default view based on application architecture
    Force32Bit,  ///< Force access to 32-bit view (WOW6432Node)
    Force64Bit   ///< Force access to 64-bit view (native path)
};

/**
 * @brief Information about a registry value
 */
struct RegistryValue {
    std::string name;                ///< Value name
    RegistryValueType type;          ///< Value type
    RegistryValueType originalType;  ///< Original value type (before conversion)
    std::vector<uint8_t> data;       ///< Raw value data

    /**
     * @brief Get the value as a string (if type is String or ExpandString)
     * @return String value
     * @throws std::runtime_error if type is not String or ExpandString
     */
    std::string asString() const;

    /**
     * @brief Get the value as a DWORD (if type is DWord)
     * @return 32-bit integer value
     * @throws std::runtime_error if type is not DWord
     */
    uint32_t asDWord() const;

    /**
     * @brief Get the value as a QWORD (if type is QWord)
     * @return 64-bit integer value
     * @throws std::runtime_error if type is not QWord
     */
    uint64_t asQWord() const;

    /**
     * @brief Get the value as a vector of strings (if type is MultiString)
     * @return Vector of strings
     * @throws std::runtime_error if type is not MultiString
     */
    std::vector<std::string> asMultiString() const;
};

/**
 * @brief Registry value item information
 */
struct RegistryItem {
    std::string name;        ///< Value name
    RegistryValueType type;  ///< Value type
    uint32_t dataSize;       ///< Size of value data in bytes
};

/**
 * @brief Registry access install_options
 */
enum class RegistryAccess {
    Read,      ///< Read access
    Write,     ///< Write access
    ReadWrite  ///< Read and write access
};

/**
 * @class IRegistryManager
 * @brief Interface for registry operations
 *
 * This interface provides methods to read, write, and manipulate
 * registry keys and values in a platform-independent manner.
 */
class IRegistryManager {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IRegistryManager() = default;

    /**
     * @brief Check if a registry key exists
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param view Registry view to access (32-bit or 64-bit)
     * @return bool True if the key exists, false otherwise
     * @throw RegistryManagerException if an error occurs
     */
    virtual bool keyExists(RegistryHive rootKey, const std::string& subKey,
                           RegistryView view = RegistryView::Default) const = 0;

    /**
     * @brief Create a registry key
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path to create
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the key was created or already exists
     * @throw RegistryManagerException if an error occurs
     */
    virtual void createKey(RegistryHive rootKey, const std::string& subKey,
                           RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Delete a registry key and all its subkeys
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path to delete
     * @param view Registry view to access (32-bit or 64-bit)
     * @return void Success if the key was deleted
     * @throw RegistryManagerException if an error occurs
     */
    virtual void deleteKey(RegistryHive rootKey, const std::string& subKey,
                           RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Get a list of subkeys in a registry key
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param view Registry view to access (32-bit or 64-bit)
     * @return std::vector<std::string> List of subkey names
     * @throw RegistryManagerException if an error occurs
     */
    virtual std::vector<std::string>
    getSubKeys(RegistryHive rootKey, const std::string& subKey,
               RegistryView view = RegistryView::Default) const = 0;

    /**
     * @brief Check if a registry value exists
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to check
     * @param view Registry view to access (32-bit or 64-bit)
     * @return bool True if the value exists, false otherwise
     * @throw RegistryManagerException if an error occurs
     */
    virtual bool valueExists(RegistryHive rootKey, const std::string& subKey,
                             const std::string& valueName,
                             RegistryView view = RegistryView::Default) const = 0;

    /**
     * @brief Get a registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to get (empty for default value)
     * @param view Registry view to access (32-bit or 64-bit)
     * @return RegistryValue The registry value if successful
     * @throw RegistryManagerException if an error occurs
     */
    virtual RegistryValue getValue(RegistryHive rootKey, const std::string& subKey,
                                   const std::string& valueName = "",
                                   bool expandEnvironmentVars = false,
                                   RegistryView view = RegistryView::Default) const = 0;

    /**
     * @brief Get all values under a registry key
     *
     * @param hive The registry hive
     * @param keyPath Path to the key
     * @return std::vector<RegistryItem> List of values with their types if successful
     * @throw RegistryManagerException if an error occurs
     */
    virtual std::vector<RegistryItem> getItems(RegistryHive rootKey, const std::string& subKey,
                                               RegistryView view = RegistryView::Default) const = 0;

    /**
     * @brief Get a list of value names in a registry key
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param view Registry view to access (32-bit or 64-bit)
     * @return std::vector<std::string> List of value names
     * @throw RegistryManagerException if an error occurs
     */
    virtual std::vector<std::string>
    getValueNames(RegistryHive rootKey, const std::string& subKey,
                  RegistryView view = RegistryView::Default) const = 0;

    /**
     * @brief Get the type of a registry value
     *
     * @param hive The registry hive
     * @param keyPath Path to the key containing the value
     * @param valueName Name of the value to check
     * @return RegistryValueType The type of the value if successful
     * @throw RegistryManagerException if an error occurs
     */
    virtual RegistryValueType getValueType(RegistryHive hive, const std::string& keyPath,
                                           const std::string& valueName,
                                           RegistryView view = RegistryView::Default) const = 0;

    /**
     * @brief Set a string registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set (empty for default value)
     * @param value The string value to set
     * @param expandable True to create an expandable string (with environment variables)
     * @param view Registry view to access (32-bit or 64-bit)
     *
     * @throw RegistryManagerException if an error occurs
     */
    virtual void setString(RegistryHive rootKey, const std::string& subKey,
                           const std::string& valueName, const std::string& value,
                           bool expandable = false, RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Set a DWORD (32-bit integer) registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param value The 32-bit integer value to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @throw RegistryManagerException if an error occurs
     */
    virtual void setDWord(RegistryHive rootKey, const std::string& subKey,
                          const std::string& valueName, uint32_t value,
                          RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Set a QWORD (64-bit integer) registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param value The 64-bit integer value to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @throw RegistryManagerException if an error occurs
     */
    virtual void setQWord(RegistryHive rootKey, const std::string& subKey,
                          const std::string& valueName, uint64_t value,
                          RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Set a binary registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param data The binary data to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @throw RegistryManagerException if an error occurs
     */
    virtual void setBinary(RegistryHive rootKey, const std::string& subKey,
                           const std::string& valueName, const std::vector<uint8_t>& data,
                           RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Set a multi-string registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to set
     * @param values The vector of strings to set
     * @param view Registry view to access (32-bit or 64-bit)
     * @throw RegistryManagerException if an error occurs
     */
    virtual void setMultiString(RegistryHive rootKey, const std::string& subKey,
                                const std::string& valueName,
                                const std::vector<std::string>& values,
                                RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Delete a registry value
     *
     * @param rootKey The root key (HKEY_*)
     * @param subKey The subkey path
     * @param valueName The name of the value to delete
     * @param view Registry view to access (32-bit or 64-bit)
     * @throw RegistryManagerException if an error occurs
     */
    virtual void deleteValue(RegistryHive rootKey, const std::string& subKey,
                             const std::string& valueName,
                             RegistryView view = RegistryView::Default) = 0;

    /**
     * @brief Splits the registry path into root key and subkey.
     *
     * @param path The full registry path to split.
     * @return std::pair<RegistryHive, std::string> The root key and subkey pair if
     * successful
     * @throw RegistryManagerException if the path is invalid or an error occurs
     */
    virtual std::pair<RegistryHive, std::string> parsePath(const std::string& path) = 0;
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_MANAGER_INCLUDE_REGISTRY_MANAGER_HPP