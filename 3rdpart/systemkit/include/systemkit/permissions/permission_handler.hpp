/**
 * @file permission_handler.hpp
 * @brief Abstract base class for managing permissions.
 * @details This class defines the interface for managing permissions across different platforms.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */
#ifndef LEIGOD_INCLUDE_SYSTEMKIT_PERMISSION_PERMISSION_HANDLER_HPP
#define LEIGOD_INCLUDE_SYSTEMKIT_PERMISSION_PERMISSION_HANDLER_HPP

#include "systemkit/exceptions/exceptions.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace leigod {
namespace system_kit {

/**
 * @brief Types of resources that can have permissions applied
 */
enum class ResourceType {
    File,       ///< File resource
    Directory,  ///< Directory resource
    Registry,   ///< Registry key resource
    Service     ///< Windows service resource
};

/**
 * @brief Permission access rights
 */
enum class AccessRight : uint32_t {
    // Common rights
    Read = 0x00000001,         ///< Read access
    Write = 0x00000002,        ///< Write access
    Execute = 0x00000004,      ///< Execute access
    Delete = 0x00000008,       ///< Delete access
    ChangeOwner = 0x00000010,  ///< Change owner

    // Specialized rights
    ReadPermissions = 0x00000020,   ///< Read permissions
    WritePermissions = 0x00000040,  ///< Modify permissions

    // Combined rights
    ReadWrite = Read | Write,  ///< Read and write access
    FullControl = 0xFFFFFFFF,  ///< Full control (all rights)
    None = 0x00000000          ///< No rights
};

/**
 * @brief Operator overload for combining access rights
 * @param a First access right
 * @param b Second access right
 * @return Combined access rights
 */
inline AccessRight operator|(AccessRight a, AccessRight b) {
    return static_cast<AccessRight>(static_cast<std::underlying_type_t<AccessRight>>(a) |
                                    static_cast<std::underlying_type_t<AccessRight>>(b));
}

/**
 * @brief Operator overload for checking if access right contains another
 * @param a First access right
 * @param b Second access right
 * @return True if a contains all bits from b
 */
inline bool operator&(AccessRight a, AccessRight b) {
    return (static_cast<std::underlying_type_t<AccessRight>>(a) &
            static_cast<std::underlying_type_t<AccessRight>>(b)) ==
           static_cast<std::underlying_type_t<AccessRight>>(b);
}

/**
 * @brief Permission inheritance mode
 */
enum class InheritanceMode {
    None,           ///< No inheritance
    ThisOnly,       ///< Apply to this object only
    ContainerOnly,  ///< Apply to immediate children only
    Descendents,    ///< Apply to all descendents (not this object)
    Full            ///< Apply to this object and all descendents
};

/**
 * @brief Permission action when setting permissions
 */
enum class PermissionAction {
    Grant,  ///< Grant the permissions to the trustee
    Deny,   ///< Explicitly deny the permissions to the trustee
    Revoke  ///< Remove the permissions (both grant and deny) for the trustee
};

/**
 * @brief Represents a permission entry for a resource
 */
struct Permission {
    std::string trustee;          ///< Name of the user or group (account name or SID)
    AccessRight accessRights;     ///< Access rights for the trustee
    PermissionAction action;      ///< Grant, Deny or Revoke
    InheritanceMode inheritance;  ///< Inheritance mode
};

/**
 * @brief Security descriptor information
 */
struct SecurityInfo {
    std::string owner;                    ///< Owner name
    std::string group;                    ///< Primary group name
    std::vector<Permission> permissions;  ///< List of permission entries
};

/**
 * @brief Parameters for process elevation
 */
struct ElevationParameters {
    std::string arguments;         ///< Command line arguments to pass to elevated process
    std::string workingDirectory;  ///< Working directory for the elevated process (empty = inherit)
    int showCmd = 1;               ///< Show window command for the elevated process (1 = normal)
    bool waitForElevation = false;  ///< Whether to wait for the elevated process to exit
};

/**
 * @class IPermissionHandler
 * @brief Interface for permission and security management
 *
 * This interface provides methods to manage permissions on
 * various system resources like files, directories, registry keys, etc.
 */
class IPermissionHandler {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IPermissionHandler() = default;

    /**
     * @brief Get permissions for a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @return SecurityInfo Security information if successful
     * @throw PermissionHandlerException
     */
    virtual SecurityInfo getPermissions(const std::string& resourcePath,
                                        ResourceType resourceType) const = 0;

    /**
     * @brief Set permissions for a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @param permissions List of permission entries to apply
     * @param replaceAll If true, replaces all existing permissions; if false, merges with existing
     * @return void Success if permissions were set
     * @throw PermissionHandlerException
     */
    virtual void setPermissions(const std::string& resourcePath, ResourceType resourceType,
                                const std::vector<Permission>& permissions,
                                bool replaceAll = false) = 0;

    /**
     * @brief Set owner for a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @param owner Name of the user or group to set as owner
     * @return void Success if owner was set
     * @throw PermissionHandlerException
     */
    virtual void setOwner(const std::string& resourcePath, ResourceType resourceType,
                          const std::string& owner) = 0;

    /**
     * @brief Check if the current process has the specified permissions for a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @param accessRights Access rights to check
     * @return bool True if the current process has the permissions
     * @throw PermissionHandlerException
     */
    virtual bool checkAccess(const std::string& resourcePath, ResourceType resourceType,
                             AccessRight accessRights) const = 0;

    /**
     * @brief Take ownership of a resource
     *
     * Attempts to take ownership of the specified resource. Typically requires
     * elevated privileges.
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @return void Success if ownership was taken
     * @throw PermissionHandlerException
     */
    virtual void takeOwnership(const std::string& resourcePath, ResourceType resourceType) = 0;

    /**
     * @brief Check if the current process is running with administrator privileges
     *
     * @return bool True if the process has administrator privileges
     * @throw PermissionHandlerException
     */
    virtual bool isRunningAsAdministrator() const = 0;

    /**
     * @brief Elevate the current process to administrator privileges
     *
     * This function restarts the current process with elevated privileges
     * by requesting UAC elevation. If the process is already running with
     * administrator privileges, this function returns success immediately.
     *
     * @param params Parameters for the elevation process
     * @return bool True if elevation was requested or already elevated,
     *         false if elevation was canceled by the user
     * @throw PermissionHandlerException
     */
    virtual bool
    elevateToAdministrator(const ElevationParameters& params = ElevationParameters()) = 0;

    /**
     * @brief Enable or disable a privilege for the current process
     *
     * This method allows adjusting specific privileges assigned to the current process.
     * Not all platforms support this feature, and the specific privileges available vary by
     * platform.
     *
     * On Windows, privilege names include: "SeDebugPrivilege", "SeTakeOwnershipPrivilege", etc.
     *
     * @param privilegeName The name of the privilege to adjust (platform-specific)
     * @param enable True to enable the privilege, false to disable
     * @return bool True if the privilege was successfully adjusted
     * @throw PermissionHandlerException
     */
    virtual bool adjustPrivilege(const std::string& privilegeName, bool enable) = 0;
};

}  // namespace system_kit
}  // namespace leigod
#endif  // LEIGOD_INCLUDE_SYSTEMKIT_PERMISSION_PERMISSION_HANDLER_HPP