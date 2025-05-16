/**
 * @file permission_handler_windows.hpp
 * @brief Windows-specific implementation of the IPermissionHandler.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_WINDOWS_PERMISSION_MANAGER_WINDOWS_HPP
#define LEIGOD_SRC_WINDOWS_PERMISSION_MANAGER_WINDOWS_HPP

#include "systemkit/permissions/permission_handler.hpp"

#include <AccCtrl.h>
#include <Aclapi.h>
#include <Sddl.h>
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

namespace leigod {
namespace system_kit {

/**
 * @class PermissionHandlerWindows
 * @brief Windows platform implementation of permission management
 *
 * This class uses the Windows Security API to manage permissions (ACLs and DACLs)
 * on files, directories, registry keys, and other securable objects.
 */
class PermissionHandlerWindows : public IPermissionHandler {
public:
    /**
     * @brief Constructor
     */
    PermissionHandlerWindows() = default;

    /**
     * @brief Destructor
     */
    ~PermissionHandlerWindows() override = default;

    /**
     * @brief Get permissions for a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @return SecurityInfo Security information if successful
     * @throw PermissionHandlerException
     */
    SecurityInfo getPermissions(const std::string& resourcePath,
                                ResourceType resourceType) const override;

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
    void setPermissions(const std::string& resourcePath, ResourceType resourceType,
                        const std::vector<Permission>& permissions,
                        bool replaceAll = false) override;

    /**
     * @brief Set owner for a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @param owner Name of the user or group to set as owner
     * @return void Success if owner was set
     * @throw PermissionHandlerException
     */
    void setOwner(const std::string& resourcePath, ResourceType resourceType,
                  const std::string& owner) override;

    /**
     * @brief Check if the current process has the specified permissions for a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @param accessRights Access rights to check
     * @return bool True if the current process has the permissions
     * @throw PermissionHandlerException
     */
    bool checkAccess(const std::string& resourcePath, ResourceType resourceType,
                     AccessRight accessRights) const override;

    /**
     * @brief Take ownership of a resource
     *
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource (file, directory, registry, etc.)
     * @return void Success if ownership was taken
     * @throw PermissionHandlerException
     */
    void takeOwnership(const std::string& resourcePath, ResourceType resourceType) override;

    /**
     * @brief Check if the current process is running with administrator privileges
     *
     * @return bool True if the process has administrator privileges
     * @throw PermissionHandlerException
     */
    bool isRunningAsAdministrator() const override;

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
    bool elevateToAdministrator(const ElevationParameters& params) override;

    /**
     * @brief Enable or disable a Windows privilege for the current process
     *
     * @param privilegeName The name of the privilege (e.g., "SeDebugPrivilege")
     * @param enable True to enable the privilege, false to disable
     * @return bool True if the privilege was successfully adjusted
     * @throw PermissionHandlerException
     */
    bool adjustPrivilege(const std::string& privilegeName, bool enable) override;

private:
    /**
     * @brief Adjust a privilege for the current process
     *
     * @param privilegeName The name of the privilege (e.g., "SeDebugPrivilege")
     * @param enable True to enable the privilege, false to disable
     * @return bool True if the privilege was successfully adjusted
     * @throw PermissionHandlerException
     */
    bool adjustPrivilegeInternal(const std::wstring& privilegeName, bool enable);

    /**
     * @brief Convert AccessRight to Windows access mask
     * @param accessRights AccessRight value
     * @param resourceType Type of resource
     * @return ACCESS_MASK Windows access mask
     */
    ACCESS_MASK accessRightToAccessMask(AccessRight accessRights, ResourceType resourceType) const;

    /**
     * @brief Convert Windows access mask to AccessRight
     * @param accessMask Windows access mask
     * @param resourceType Type of resource
     * @return AccessRight value
     */
    AccessRight accessMaskToAccessRight(ACCESS_MASK accessMask, ResourceType resourceType) const;

    /**
     * @brief Convert InheritanceMode to Windows inheritance flags
     * @param inheritanceMode InheritanceMode value
     * @param resourceType Type of resource
     * @param inheritanceFlags Output inheritance flags
     * @param propagationFlags Output propagation flags
     */
    void inheritanceModeToFlags(InheritanceMode inheritanceMode, ResourceType resourceType,
                                DWORD& inheritanceFlags, DWORD& propagationFlags) const;

    /**
     * @brief Convert Windows inheritance flags to InheritanceMode
     * @param inheritanceFlags Windows inheritance flags
     * @param propagationFlags Windows propagation flags
     * @param resourceType Type of resource
     * @return InheritanceMode value
     */
    InheritanceMode flagsToInheritanceMode(DWORD inheritanceFlags, DWORD propagationFlags,
                                           ResourceType resourceType) const;

    /**
     * @brief Get security information from a resource
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource
     * @param securityInfo Security information to retrieve
     * @param pSD Output security descriptor
     * @return void Success if security information was retrieved
     * @throw PermissionHandlerException
     */
    void getSecurityInfo(const std::string& resourcePath, ResourceType resourceType,
                         SECURITY_INFORMATION securityInfo, PSECURITY_DESCRIPTOR& pSD) const;

    /**
     * @brief Set security information for a resource
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource
     * @param securityInfo Security information to set
     * @param pSD Security descriptor to apply
     * @return void Success if security information was set
     * @throw PermissionHandlerException
     */
    void setSecurityInfo(const std::string& resourcePath, ResourceType resourceType,
                         SECURITY_INFORMATION securityInfo, PSECURITY_DESCRIPTOR pSD) const;

    /**
     * @brief Convert resource type and path to Windows object
     * @param resourcePath Path to the resource
     * @param resourceType Type of resource
     * @param objectType Output SE_OBJECT_TYPE
     * @param objectName Output object name (wide string)
     * @return void Success if conversion succeeded
     * @throw PermissionHandlerException
     */
    void resourceToWindowsObject(const std::string& resourcePath, ResourceType resourceType,
                                 SE_OBJECT_TYPE& objectType, std::wstring& objectName) const;

    /**
     * @brief Convert trustee name to Windows SID
     * @param trusteeName Name of the user or group (account name)
     * @return PSID SID if successful, ownership transferred to caller
     * @throw PermissionHandlerException
     */
    PSID trusteeNameToSid(const std::string& trusteeName) const;

    /**
     * @brief Convert Windows SID to trustee name
     * @param sid SID to convert
     * @return std::string Trustee name if successful
     * @throw PermissionHandlerException
     */
    std::string sidToTrusteeName(PSID sid) const;
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_WINDOWS_PERMISSION_MANAGER_WINDOWS_HPP