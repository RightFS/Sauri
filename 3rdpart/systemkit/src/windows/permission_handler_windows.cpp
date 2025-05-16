/**
 * @file permission_handler_windows.cpp
 * @brief Windows-specific implementation of the IPermissionHandler.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#include "permission_handler_windows.hpp"

#include "common/utils/strings.h"
#include "windows_utils.hpp"

namespace leigod {
namespace system_kit {

using namespace utils;
using namespace common;
using namespace common::utils;

/**
 * @brief RAII wrapper for Windows security descriptor
 */
class SecurityDescriptorGuard {
public:
    explicit SecurityDescriptorGuard(PSECURITY_DESCRIPTOR sd = nullptr) : m_sd(sd) {}

    ~SecurityDescriptorGuard() {
        if (m_sd) {
            LocalFree(m_sd);
        }
    }

    PSECURITY_DESCRIPTOR get() const {
        return m_sd;
    }

    void reset(PSECURITY_DESCRIPTOR sd = nullptr) {
        if (m_sd) {
            LocalFree(m_sd);
        }
        m_sd = sd;
    }

    PSECURITY_DESCRIPTOR release() {
        PSECURITY_DESCRIPTOR temp = m_sd;
        m_sd = nullptr;
        return temp;
    }

    // Prevent copying
    SecurityDescriptorGuard(const SecurityDescriptorGuard&) = delete;
    SecurityDescriptorGuard& operator=(const SecurityDescriptorGuard&) = delete;

private:
    PSECURITY_DESCRIPTOR m_sd;
};

/**
 * @brief RAII wrapper for Windows SID
 */
class SidGuard {
public:
    explicit SidGuard(PSID sid = nullptr) : m_sid(sid) {}

    ~SidGuard() {
        if (m_sid) {
            LocalFree(m_sid);
        }
    }

    PSID get() const {
        return m_sid;
    }

    void reset(PSID sid = nullptr) {
        if (m_sid) {
            LocalFree(m_sid);
        }
        m_sid = sid;
    }

    PSID release() {
        PSID temp = m_sid;
        m_sid = nullptr;
        return temp;
    }

    // Prevent copying
    SidGuard(const SidGuard&) = delete;
    SidGuard& operator=(const SidGuard&) = delete;

private:
    PSID m_sid;
};

// Implementation of PermissionHandlerWindows

ACCESS_MASK PermissionHandlerWindows::accessRightToAccessMask(AccessRight accessRights,
                                                              ResourceType resourceType) const {
    ACCESS_MASK mask = 0;

    // Start with generic mappings that apply to all resource types
    if (accessRights & AccessRight::Read) {
        mask |= GENERIC_READ;
    }
    if (accessRights & AccessRight::Write) {
        mask |= GENERIC_WRITE;
    }
    if (accessRights & AccessRight::Execute) {
        mask |= GENERIC_EXECUTE;
    }
    if (accessRights & AccessRight::Delete) {
        mask |= DELETE;
    }
    if (accessRights & AccessRight::ReadPermissions) {
        mask |= READ_CONTROL;
    }
    if (accessRights & AccessRight::WritePermissions) {
        mask |= WRITE_DAC;
    }
    if (accessRights & AccessRight::ChangeOwner) {
        mask |= WRITE_OWNER;
    }
    if (accessRights & AccessRight::FullControl) {
        mask |= GENERIC_ALL;
    }

    // Add resource-specific rights
    switch (resourceType) {
        case ResourceType::File:
        case ResourceType::Directory:
            // Special file/directory permissions included in generic rights
            break;

        case ResourceType::Registry:
            // Map registry-specific permissions
            if (accessRights & AccessRight::Read) {
                mask |= KEY_READ;
            }
            if (accessRights & AccessRight::Write) {
                mask |= KEY_WRITE;
            }
            if (accessRights & AccessRight::FullControl) {
                mask |= KEY_ALL_ACCESS;
            }
            break;

        case ResourceType::Service:
            // Map service-specific permissions
            if (accessRights & AccessRight::Read) {
                mask |= SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_INTERROGATE |
                        SERVICE_ENUMERATE_DEPENDENTS;
            }
            if (accessRights & AccessRight::Write) {
                mask |= SERVICE_CHANGE_CONFIG;
            }
            if (accessRights & AccessRight::Execute) {
                mask |= SERVICE_START | SERVICE_STOP | SERVICE_PAUSE_CONTINUE;
            }
            if (accessRights & AccessRight::FullControl) {
                mask |= SERVICE_ALL_ACCESS;
            }
            break;
    }

    return mask;
}

AccessRight PermissionHandlerWindows::accessMaskToAccessRight(ACCESS_MASK accessMask,
                                                              ResourceType resourceType) const {
    uint32_t rights = 0;

    // Generic mappings that apply to all resource types
    if ((accessMask & GENERIC_READ) || (accessMask & FILE_GENERIC_READ)) {
        rights |= static_cast<uint32_t>(AccessRight::Read);
    }
    if ((accessMask & GENERIC_WRITE) || (accessMask & FILE_GENERIC_WRITE)) {
        rights |= static_cast<uint32_t>(AccessRight::Write);
    }
    if ((accessMask & GENERIC_EXECUTE) || (accessMask & FILE_GENERIC_EXECUTE)) {
        rights |= static_cast<uint32_t>(AccessRight::Execute);
    }
    if (accessMask & DELETE) {
        rights |= static_cast<uint32_t>(AccessRight::Delete);
    }
    if (accessMask & READ_CONTROL) {
        rights |= static_cast<uint32_t>(AccessRight::ReadPermissions);
    }
    if (accessMask & WRITE_DAC) {
        rights |= static_cast<uint32_t>(AccessRight::WritePermissions);
    }
    if (accessMask & WRITE_OWNER) {
        rights |= static_cast<uint32_t>(AccessRight::ChangeOwner);
    }

    // Check for resource-specific masks
    switch (resourceType) {
        case ResourceType::Registry:
            if (accessMask & KEY_READ) {
                rights |= static_cast<uint32_t>(AccessRight::Read);
            }
            if (accessMask & KEY_WRITE) {
                rights |= static_cast<uint32_t>(AccessRight::Write);
            }
            break;

        case ResourceType::Service:
            if ((accessMask & SERVICE_QUERY_CONFIG) || (accessMask & SERVICE_QUERY_STATUS)) {
                rights |= static_cast<uint32_t>(AccessRight::Read);
            }
            if (accessMask & SERVICE_CHANGE_CONFIG) {
                rights |= static_cast<uint32_t>(AccessRight::Write);
            }
            if ((accessMask & SERVICE_START) || (accessMask & SERVICE_STOP)) {
                rights |= static_cast<uint32_t>(AccessRight::Execute);
            }
            break;

        default:
            break;
    }

    // Check for full control
    if ((accessMask & GENERIC_ALL) ||
        (resourceType == ResourceType::Registry && (accessMask & KEY_ALL_ACCESS)) ||
        (resourceType == ResourceType::Service && (accessMask & SERVICE_ALL_ACCESS))) {
        rights = static_cast<uint32_t>(AccessRight::FullControl);
    }

    return static_cast<AccessRight>(rights);
}

void PermissionHandlerWindows::inheritanceModeToFlags(InheritanceMode inheritanceMode,
                                                      ResourceType resourceType,
                                                      DWORD& inheritanceFlags,
                                                      DWORD& propagationFlags) const {
    // Set default values
    inheritanceFlags = 0;
    propagationFlags = 0;

    // Files and directories have special handling for inheritance
    bool isFileOrDirectory =
        (resourceType == ResourceType::File || resourceType == ResourceType::Directory);

    switch (inheritanceMode) {
        case InheritanceMode::None:
            // No inheritance flags
            break;

        case InheritanceMode::ThisOnly:
            // Apply to this object only
            if (isFileOrDirectory) {
                propagationFlags = PROTECTED_DACL_SECURITY_INFORMATION;
            }
            break;

        case InheritanceMode::ContainerOnly:
            // Apply to immediate children only
            inheritanceFlags = CONTAINER_INHERIT_ACE;
            if (isFileOrDirectory) {
                propagationFlags = INHERIT_ONLY_ACE;
            }
            break;

        case InheritanceMode::Descendents:
            // Apply to all descendents (not this object)
            if (isFileOrDirectory) {
                inheritanceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
                propagationFlags = INHERIT_ONLY_ACE;
            } else {
                inheritanceFlags = CONTAINER_INHERIT_ACE;
            }
            break;

        case InheritanceMode::Full:
            // Apply to this object and all descendents
            if (isFileOrDirectory) {
                inheritanceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
            } else {
                inheritanceFlags = CONTAINER_INHERIT_ACE;
            }
            break;
    }
}

InheritanceMode PermissionHandlerWindows::flagsToInheritanceMode(DWORD inheritanceFlags,
                                                                 DWORD propagationFlags,
                                                                 ResourceType resourceType) const {
    // If no inheritance flags are set, it's "ThisOnly" or "None"
    if (inheritanceFlags == 0) {
        if (propagationFlags & PROTECTED_DACL_SECURITY_INFORMATION) {
            return InheritanceMode::ThisOnly;
        }
        return InheritanceMode::None;
    }

    bool isFileOrDirectory =
        (resourceType == ResourceType::File || resourceType == ResourceType::Directory);
    bool containerInherit = (inheritanceFlags & CONTAINER_INHERIT_ACE) != 0;
    bool objectInherit = (inheritanceFlags & OBJECT_INHERIT_ACE) != 0;
    bool inheritOnly = (propagationFlags & INHERIT_ONLY_ACE) != 0;

    if (isFileOrDirectory) {
        // For files and directories
        if (containerInherit && objectInherit) {
            // Both container and object inheritance
            if (inheritOnly) {
                return InheritanceMode::Descendents;
            } else {
                return InheritanceMode::Full;
            }
        } else if (containerInherit) {
            // Only container inheritance
            if (inheritOnly) {
                return InheritanceMode::ContainerOnly;
            } else {
                // This is a rare case, but closest match is Full
                return InheritanceMode::Full;
            }
        }
    } else {
        // For registry and services
        if (containerInherit) {
            if (inheritOnly) {
                return InheritanceMode::Descendents;
            } else {
                return InheritanceMode::Full;
            }
        }
    }

    // Default fallback
    return InheritanceMode::ThisOnly;
}

void PermissionHandlerWindows::resourceToWindowsObject(const std::string& resourcePath,
                                                       ResourceType resourceType,
                                                       SE_OBJECT_TYPE& objectType,
                                                       std::wstring& objectName) const {
    try {
        // Convert path to wide string
        objectName = utf8ToWide(resourcePath);

        // Set object type based on resource type
        switch (resourceType) {
            case ResourceType::File:
                objectType = SE_FILE_OBJECT;
                break;

            case ResourceType::Directory:
                objectType = SE_FILE_OBJECT;  // Directories use the same object type as files
                break;

            case ResourceType::Registry:
                objectType = SE_REGISTRY_KEY;
                break;

            case ResourceType::Service:
                objectType = SE_SERVICE;
                break;

            default:
                throw PermissionHandlerException(ErrorCode::InvalidArgument,
                                                 "Unsupported resource type");
        }
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        // Handle other exceptions
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Failed to convert resource to Windows object: " +
                                             std::string(ex.what()));
    }
}

PSID PermissionHandlerWindows::trusteeNameToSid(const std::string& trusteeName) const {
    try {
        // Check if the input is already an SID string (S-1-...)
        if (trusteeName.substr(0, 2) == "S-") {
            PSID sid = nullptr;
            if (!ConvertStringSidToSidA(trusteeName.c_str(), &sid)) {
                throw PermissionHandlerException(ErrorCode::InvalidArgument,
                                                 "Failed to convert string SID to SID: " +
                                                     getLastErrorMessage());
            }
            return sid;
        }

        // Convert trustee name to wide string
        std::wstring wTrusteeName = utf8ToWide(trusteeName);

        // Look up the account
        SID_NAME_USE sidType;
        DWORD sidSize = 0;
        DWORD domainSize = 0;

        // First call to get required buffer sizes
        LookupAccountNameW(nullptr,  // Local computer
                           wTrusteeName.c_str(),
                           nullptr,  // SID buffer (will be allocated)
                           &sidSize,
                           nullptr,  // Domain name buffer (not needed)
                           &domainSize, &sidType);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            throw PermissionHandlerException(
                ErrorCode::SecurityError, "Failed to get buffer sizes for account: " + trusteeName +
                                              ", " + getLastErrorMessage());
        }

        // Allocate buffers
        std::vector<BYTE> sidBuffer(sidSize);
        std::vector<wchar_t> domainBuffer(domainSize);

        // Second call to get actual data
        if (!LookupAccountNameW(nullptr,  // Local computer
                                wTrusteeName.c_str(),
                                sidBuffer.data(),  // SID buffer
                                &sidSize,
                                domainBuffer.data(),  // Domain name buffer
                                &domainSize, &sidType)) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to lookup account: " + trusteeName + ", " +
                                                 getLastErrorMessage());
        }

        // Allocate and copy the SID
        PSID sid = nullptr;
        if (!ConvertSidToStringSidW(sidBuffer.data(), reinterpret_cast<LPWSTR*>(&sid))) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to convert SID to string: " +
                                                 getLastErrorMessage());
        }

        if (!IsValidSid(sid)) {
            LocalFree(sid);
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Invalid SID generated for account: " + trusteeName);
        }

        return sid;
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(
            ErrorCode::SecurityError, "Exception in trusteeNameToSid: " + std::string(ex.what()));
    }
}

std::string PermissionHandlerWindows::sidToTrusteeName(PSID sid) const {
    if (!IsValidSid(sid)) {
        throw PermissionHandlerException(ErrorCode::InvalidArgument, "Invalid SID provided");
    }

    try {
        // First try to get account name and domain
        wchar_t name[256] = {0};
        DWORD nameSize = sizeof(name) / sizeof(name[0]);
        wchar_t domain[256] = {0};
        DWORD domainSize = sizeof(domain) / sizeof(domain[0]);
        SID_NAME_USE sidType;

        if (LookupAccountSidW(nullptr,  // Local computer
                              sid, name, &nameSize, domain, &domainSize, &sidType)) {
            // Format as DOMAIN\Username or just Username for local accounts
            std::wstring wTrusteeName;
            if (domainSize > 1) {  // Domain name exists
                wTrusteeName = std::wstring(domain) + L"\\" + std::wstring(name);
            } else {
                wTrusteeName = std::wstring(name);
            }

            return wideToUtf8(wTrusteeName);
        }

        // If lookup fails, fall back to SID string
        LPWSTR stringSid = nullptr;
        if (ConvertSidToStringSidW(sid, &stringSid)) {
            std::wstring wSid(stringSid);
            LocalFree(stringSid);
            return wideToUtf8(wSid);
        }
        throw PermissionHandlerException(
            ErrorCode::SecurityError, "Failed to convert SID to string: " + getLastErrorMessage());
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        // Handle other exceptions
        throw PermissionHandlerException(
            ErrorCode::SecurityError, "Exception in sidToTrusteeName: " + std::string(ex.what()));
    }
}

void PermissionHandlerWindows::getSecurityInfo(const std::string& resourcePath,
                                               ResourceType resourceType,
                                               SECURITY_INFORMATION securityInfo,
                                               PSECURITY_DESCRIPTOR& pSD) const {
    try {
        SE_OBJECT_TYPE objectType;
        std::wstring objectName;
        resourceToWindowsObject(resourcePath, resourceType, objectType, objectName);

        DWORD error = ::GetNamedSecurityInfoW(objectName.c_str(), objectType, securityInfo,
                                              nullptr,  // Owner SID (retrieved to pSD)
                                              nullptr,  // Group SID (retrieved to pSD)
                                              nullptr,  // DACL (retrieved to pSD)
                                              nullptr,  // SACL (retrieved to pSD)
                                              &pSD);

        if (error != ERROR_SUCCESS) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to get security information: " +
                                                 getLastErrorMessage());
        }
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception in getSecurityInfo: " + std::string(ex.what()));
    }
}

void PermissionHandlerWindows::setSecurityInfo(const std::string& resourcePath,
                                               ResourceType resourceType,
                                               SECURITY_INFORMATION securityInfo,
                                               PSECURITY_DESCRIPTOR pSD) const {
    try {
        SE_OBJECT_TYPE objectType;
        std::wstring objectName;

        resourceToWindowsObject(resourcePath, resourceType, objectType, objectName);

        // Extract components we need to set
        PSID pOwner = nullptr;
        PSID pGroup = nullptr;
        PACL pDacl = nullptr;
        PACL pSacl = nullptr;
        BOOL defaulted = FALSE;

        if (securityInfo & OWNER_SECURITY_INFORMATION) {
            if (!GetSecurityDescriptorOwner(pSD, &pOwner, &defaulted)) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get owner from security descriptor: " +
                                                     getLastErrorMessage());
            }
        }

        if (securityInfo & GROUP_SECURITY_INFORMATION) {
            if (!GetSecurityDescriptorGroup(pSD, &pGroup, &defaulted)) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get group from security descriptor: " +
                                                     getLastErrorMessage());
            }
        }

        if (securityInfo & DACL_SECURITY_INFORMATION) {
            BOOL daclPresent = FALSE;
            if (!GetSecurityDescriptorDacl(pSD, &daclPresent, &pDacl, &defaulted) || !daclPresent) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get DACL from security descriptor: " +
                                                     getLastErrorMessage());
            }
        }

        if (securityInfo & SACL_SECURITY_INFORMATION) {
            BOOL saclPresent = FALSE;
            if (!GetSecurityDescriptorSacl(pSD, &saclPresent, &pSacl, &defaulted) || !saclPresent) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get SACL from security descriptor: " +
                                                     getLastErrorMessage());
            }
        }

        // Apply the security information
        DWORD error = ::SetNamedSecurityInfoW(const_cast<LPWSTR>(objectName.c_str()), objectType,
                                              securityInfo, pOwner, pGroup, pDacl, pSacl);

        if (error != ERROR_SUCCESS) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to set security information: " +
                                                 getLastErrorMessage());
        }
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception in setSecurityInfo: " + std::string(ex.what()));
    }
}

SecurityInfo PermissionHandlerWindows::getPermissions(const std::string& resourcePath,
                                                      ResourceType resourceType) const {
    try {
        // Get security descriptor
        PSECURITY_DESCRIPTOR pSD = nullptr;
        getSecurityInfo(resourcePath, resourceType,
                        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION,
                        pSD);

        SecurityDescriptorGuard sdGuard(pSD);

        SecurityInfo securityInfo;

        // Get owner
        PSID ownerSid = nullptr;
        BOOL defaulted = FALSE;
        if (!GetSecurityDescriptorOwner(pSD, &ownerSid, &defaulted) || !ownerSid) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to get owner from security descriptor: " +
                                                 getLastErrorMessage());
        }

        try {
            securityInfo.owner = sidToTrusteeName(ownerSid);
        } catch ([[maybe_unused]] const PermissionHandlerException& e) {
            // If we can't resolve the name, use SID string
            LPWSTR stringSid = nullptr;
            if (ConvertSidToStringSidW(ownerSid, &stringSid)) {
                securityInfo.owner = wideToUtf8(stringSid);
                LocalFree(stringSid);
            } else {
                securityInfo.owner = "<unknown>";
            }
        }

        // Get group
        PSID groupSid = nullptr;
        if (!GetSecurityDescriptorGroup(pSD, &groupSid, &defaulted) || !groupSid) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to get group from security descriptor: " +
                                                 getLastErrorMessage());
        }

        try {
            securityInfo.group = sidToTrusteeName(groupSid);
        } catch ([[maybe_unused]] const PermissionHandlerException& e) {
            // If we can't resolve the name, use SID string
            LPWSTR stringSid = nullptr;
            if (ConvertSidToStringSidW(groupSid, &stringSid)) {
                securityInfo.group = wideToUtf8(stringSid);
                LocalFree(stringSid);
            } else {
                securityInfo.group = "<unknown>";
            }
        }

        // Get DACL
        PACL pDacl = nullptr;
        BOOL daclPresent = FALSE;
        if (!GetSecurityDescriptorDacl(pSD, &daclPresent, &pDacl, &defaulted)) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to get DACL from security descriptor: " +
                                                 getLastErrorMessage());
        }

        if (daclPresent && pDacl != nullptr) {
            // Enumerate ACEs in the DACL
            ACL_SIZE_INFORMATION aclInfo = {0};
            if (!GetAclInformation(pDacl, &aclInfo, sizeof(aclInfo), AclSizeInformation)) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get ACL information: " +
                                                     getLastErrorMessage());
            }

            for (DWORD i = 0; i < aclInfo.AceCount; i++) {
                LPVOID ace = nullptr;
                if (!GetAce(pDacl, i, &ace) || ace == nullptr) {
                    continue;  // Skip this ACE if we can't get it
                }

                ACE_HEADER* aceHeader = static_cast<ACE_HEADER*>(ace);

                // Create permission entry
                Permission permission;

                // Set inheritance mode
                permission.inheritance = flagsToInheritanceMode(
                    aceHeader->AceFlags & (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE),
                    aceHeader->AceFlags & (INHERIT_ONLY_ACE | PROTECTED_DACL_SECURITY_INFORMATION),
                    resourceType);

                // Get trustee (SID) and access mask based on ACE type
                PSID trustee = nullptr;
                ACCESS_MASK accessMask = 0;

                if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                    ACCESS_ALLOWED_ACE* allowedAce = static_cast<ACCESS_ALLOWED_ACE*>(ace);
                    trustee = reinterpret_cast<PSID>(&allowedAce->SidStart);
                    accessMask = allowedAce->Mask;
                    permission.action = PermissionAction::Grant;
                } else if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE) {
                    ACCESS_DENIED_ACE* deniedAce = static_cast<ACCESS_DENIED_ACE*>(ace);
                    trustee = reinterpret_cast<PSID>(&deniedAce->SidStart);
                    accessMask = deniedAce->Mask;
                    permission.action = PermissionAction::Deny;
                } else {
                    continue;  // Skip other ACE types
                }

                try {
                    permission.trustee = sidToTrusteeName(trustee);
                } catch ([[maybe_unused]] const PermissionHandlerException& e) {
                    // Handle engine
                    // If we can't resolve the name, use SID string
                    LPWSTR stringSid = nullptr;
                    if (ConvertSidToStringSidW(trustee, &stringSid)) {
                        permission.trustee = wideToUtf8(stringSid);
                        LocalFree(stringSid);
                    } else {
                        permission.trustee = "<unknown>";
                    }
                }

                // Convert access mask to access rights
                permission.accessRights = accessMaskToAccessRight(accessMask, resourceType);

                // Add permission to result
                securityInfo.permissions.push_back(permission);
            }
        }
        return securityInfo;
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception in getPermissions: " + std::string(ex.what()));
    }
}

void PermissionHandlerWindows::setPermissions(const std::string& resourcePath,
                                              ResourceType resourceType,
                                              const std::vector<Permission>& permissions,
                                              bool replaceAll) {
    try {
        // First get the existing security descriptor if we're not replacing everything
        PSECURITY_DESCRIPTOR pSD = nullptr;
        SecurityDescriptorGuard sdGuard;

        if (!replaceAll) {
            getSecurityInfo(resourcePath, resourceType,
                            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                            pSD);

            sdGuard.reset(pSD);
        } else {
            // Create a new security descriptor
            pSD = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
            if (!pSD) {
                throw PermissionHandlerException(ErrorCode::OutOfMemory,
                                                 "Failed to allocate security descriptor: " +
                                                     getLastErrorMessage());
            }

            sdGuard.reset(pSD);

            if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to initialize security descriptor: " +
                                                     getLastErrorMessage());
            }
        }

        // Create a new DACL or get the existing one
        PACL pOldDacl = nullptr;
        BOOL daclPresent = FALSE;
        BOOL defaulted = FALSE;

        if (!replaceAll) {
            if (!GetSecurityDescriptorDacl(pSD, &daclPresent, &pOldDacl, &defaulted)) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get DACL from security descriptor: " +
                                                     getLastErrorMessage());
            }
        }

        // Calculate size of new DACL
        DWORD aclSize = sizeof(ACL);

        // Add space for each ACE
        for (const auto& permission : permissions) {
            // Get the trustee's SID
            auto sid = trusteeNameToSid(permission.trustee);
            SidGuard sidGuard(sid);
            DWORD sidLength = GetLengthSid(sidGuard.get());

            // Add space for ACE header + SID
            aclSize += sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sidLength;
        }

        // If preserving existing DACL, add its size too
        if (!replaceAll && daclPresent && pOldDacl != nullptr) {
            ACL_SIZE_INFORMATION aclInfo = {0};
            if (!GetAclInformation(pOldDacl, &aclInfo, sizeof(aclInfo), AclSizeInformation)) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get ACL information: " +
                                                     getLastErrorMessage());
            }

            // Add the size of all existing ACEs we want to keep
            aclSize += aclInfo.AclBytesInUse - sizeof(ACL);
        }

        // Allocate memory for new ACL
        PACL pNewDacl = static_cast<PACL>(LocalAlloc(LPTR, aclSize));
        if (!pNewDacl) {
            throw PermissionHandlerException(ErrorCode::OutOfMemory,
                                             "Failed to allocate memory for new DACL: " +
                                                 getLastErrorMessage());
        }

        // Initialize the new ACL
        if (!InitializeAcl(pNewDacl, aclSize, ACL_REVISION)) {
            LocalFree(pNewDacl);
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to initialize new DACL: " +
                                                 getLastErrorMessage());
        }

        // Copy existing ACEs if not replacing all
        if (!replaceAll && daclPresent && pOldDacl != nullptr) {
            ACL_SIZE_INFORMATION aclInfo = {0};
            if (!GetAclInformation(pOldDacl, &aclInfo, sizeof(aclInfo), AclSizeInformation)) {
                LocalFree(pNewDacl);
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get ACL information: " +
                                                     getLastErrorMessage());
            }

            // Copy ACEs from old DACL to new one
            for (DWORD i = 0; i < aclInfo.AceCount; i++) {
                LPVOID ace = nullptr;
                if (!GetAce(pOldDacl, i, &ace) || ace == nullptr) {
                    continue;  // Skip this ACE if we can't get it
                }

                if (!AddAce(pNewDacl, ACL_REVISION,
                            MAXDWORD,  // add to end
                            ace, ((PACE_HEADER)ace)->AceSize)) {
                    LocalFree(pNewDacl);
                    throw PermissionHandlerException(ErrorCode::SecurityError,
                                                     "Failed to add ACE to new DACL: " +
                                                         getLastErrorMessage());
                }
            }
        }

        // Add new ACEs
        for (const auto& permission : permissions) {
            // Get the trustee's SID
            auto sid = trusteeNameToSid(permission.trustee);
            SidGuard sidGuard(sid);

            // Convert permission to Windows access mask
            ACCESS_MASK accessMask = accessRightToAccessMask(permission.accessRights, resourceType);

            // Convert inheritance mode to flags
            DWORD inheritanceFlags = 0;
            DWORD propagationFlags = 0;

            inheritanceModeToFlags(permission.inheritance, resourceType, inheritanceFlags,
                                   propagationFlags);

            DWORD aceFlags = inheritanceFlags | propagationFlags;

            // Add the ACE based on the action type
            BOOL success = FALSE;

            if (permission.action == PermissionAction::Grant) {
                success = AddAccessAllowedAceEx(pNewDacl, ACL_REVISION, aceFlags, accessMask,
                                                sidGuard.get());
            } else if (permission.action == PermissionAction::Deny) {
                success = AddAccessDeniedAceEx(pNewDacl, ACL_REVISION, aceFlags, accessMask,
                                               sidGuard.get());
            } else if (permission.action == PermissionAction::Revoke) {
                // Revoke is special, we need to remove matching ACEs
                // Skip adding a new ACE, we'll handle this in the code to apply the DACL
                continue;
            }

            if (!success) {
                LocalFree(pNewDacl);
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to add ACE to new DACL: " +
                                                     getLastErrorMessage());
            }
        }

        // Set the new DACL in the security descriptor
        if (!SetSecurityDescriptorDacl(pSD,
                                       TRUE,  // DACL present
                                       pNewDacl,
                                       FALSE  // Not defaulted
                                       )) {
            LocalFree(pNewDacl);
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to set DACL in security descriptor: " +
                                                 getLastErrorMessage());
        }

        // Apply the security descriptor to the resource
        SE_OBJECT_TYPE objectType;
        std::wstring objectName;

        try {
            resourceToWindowsObject(resourcePath, resourceType, objectType, objectName);
        } catch (const PermissionHandlerException& e) {
            LocalFree(pNewDacl);
            throw e;  // Rethrow custom engine
        }

        // Apply the security descriptor
        DWORD error =
            ::SetNamedSecurityInfoW(const_cast<LPWSTR>(objectName.c_str()), objectType,
                                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                    nullptr,   // No owner
                                    nullptr,   // No group
                                    pNewDacl,  // New DACL
                                    nullptr    // No SACL
            );

        LocalFree(pNewDacl);  // Done with the DACL

        if (error != ERROR_SUCCESS) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to set security information: " +
                                                 getLastErrorMessage());
        }
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception in setPermissions: " + std::string(ex.what()));
    }
}

void PermissionHandlerWindows::setOwner(const std::string& resourcePath, ResourceType resourceType,
                                        const std::string& owner) {
    try {
        // Convert owner name to SID
        auto sid = trusteeNameToSid(owner);
        SidGuard sidGuard(sid);

        // Create a security descriptor
        SECURITY_DESCRIPTOR sd;
        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to initialize security descriptor: " +
                                                 getLastErrorMessage());
        }

        // Set owner in the security descriptor
        if (!SetSecurityDescriptorOwner(&sd, sidGuard.get(), FALSE)) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to set owner in security descriptor: " +
                                                 getLastErrorMessage());
        }

        // Apply the security descriptor to the resource
        SE_OBJECT_TYPE objectType;
        std::wstring objectName;
        resourceToWindowsObject(resourcePath, resourceType, objectType, objectName);

        // Apply the security descriptor
        DWORD error = ::SetNamedSecurityInfoW(const_cast<LPWSTR>(objectName.c_str()), objectType,
                                              OWNER_SECURITY_INFORMATION,
                                              sidGuard.get(),  // New owner
                                              nullptr,         // No group
                                              nullptr,         // No DACL
                                              nullptr          // No SACL
        );

        if (error != ERROR_SUCCESS) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to set owner: " + getLastErrorMessage());
        }
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception in setOwner: " + std::string(ex.what()));
    }
}

bool PermissionHandlerWindows::checkAccess(const std::string& resourcePath,
                                           ResourceType resourceType,
                                           AccessRight accessRights) const {
    try {
        // Convert resource path to object name and type
        SE_OBJECT_TYPE objectType;
        std::wstring objectName;
        resourceToWindowsObject(resourcePath, resourceType, objectType, objectName);

        // Convert access rights to access mask
        ACCESS_MASK desiredAccess = accessRightToAccessMask(accessRights, resourceType);

        // Create generic mapping
        GENERIC_MAPPING genericMapping = {0};

        switch (resourceType) {
            case ResourceType::File:
            case ResourceType::Directory:
                genericMapping.GenericRead = FILE_GENERIC_READ;
                genericMapping.GenericWrite = FILE_GENERIC_WRITE;
                genericMapping.GenericExecute = FILE_GENERIC_EXECUTE;
                genericMapping.GenericAll = FILE_ALL_ACCESS;
                break;

            case ResourceType::Registry:
                genericMapping.GenericRead = KEY_READ;
                genericMapping.GenericWrite = KEY_WRITE;
                genericMapping.GenericExecute = KEY_EXECUTE;
                genericMapping.GenericAll = KEY_ALL_ACCESS;
                break;

            case ResourceType::Service:
                genericMapping.GenericRead =
                    SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS;
                genericMapping.GenericWrite = SERVICE_CHANGE_CONFIG;
                genericMapping.GenericExecute =
                    SERVICE_START | SERVICE_STOP | SERVICE_PAUSE_CONTINUE;
                genericMapping.GenericAll = SERVICE_ALL_ACCESS;
                break;

            default:
                throw PermissionHandlerException(ErrorCode::InvalidArgument,
                                                 "Unsupported resource type");
        }

        // Map generic rights to specific rights
        MapGenericMask(&desiredAccess, &genericMapping);

        // For file/directory objects, we need to open a handle to check access
        if (resourceType == ResourceType::File || resourceType == ResourceType::Directory) {
            DWORD attributes = GetFileAttributesW(objectName.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to get file attributes: " +
                                                     getLastErrorMessage());
            }

            // Try to open the file with the requested access
            DWORD accessMask = 0;

            // Map desired access to CreateFile flags
            if (desiredAccess & FILE_READ_DATA) {
                accessMask |= GENERIC_READ;
            }
            if (desiredAccess & FILE_WRITE_DATA) {
                accessMask |= GENERIC_WRITE;
            }
            if (desiredAccess & FILE_EXECUTE) {
                accessMask |= GENERIC_EXECUTE;
            }

            HANDLE hFile = CreateFileW(
                objectName.c_str(), accessMask,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                (attributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0, nullptr);

            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
                return true;
            }

            DWORD error = GetLastError();
            if (error == ERROR_ACCESS_DENIED) {
                return false;  // Access denied means we don't have permission
            }

            throw PermissionHandlerException(
                ErrorCode::SecurityError, "Failed to check file access: " + getLastErrorMessage());
        }
        // For registry keys
        else if (resourceType == ResourceType::Registry) {
            // Try to open the key with the requested access
            HKEY hKey;
            REGSAM accessMask = 0;

            // Map desired access to registry access flags
            if (desiredAccess & KEY_READ) {
                accessMask |= KEY_READ;
            }
            if (desiredAccess & KEY_WRITE) {
                accessMask |= KEY_WRITE;
            }
            if (desiredAccess & KEY_EXECUTE) {
                accessMask |= KEY_EXECUTE;
            }

            LONG ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,  // Assumed root key, this is not complete
                                     objectName.c_str(), 0, accessMask, &hKey);

            if (ret == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }

            if (ret == ERROR_ACCESS_DENIED) {
                return false;  // Access denied means we don't have permission
            }

            if (ret == ERROR_FILE_NOT_FOUND) {
                throw PermissionHandlerException(ErrorCode::ResourceNotFound,
                                                 "Registry key not found: " + resourcePath);
            }
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to check registry access: " +
                                                 getLastErrorMessage());
        }
        // For services
        else if (resourceType == ResourceType::Service) {
            SC_HANDLE hSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);

            if (!hSCManager) {
                throw PermissionHandlerException(ErrorCode::SecurityError,
                                                 "Failed to open service control manager: " +
                                                     getLastErrorMessage());
            }

            DWORD accessMask = 0;

            // Map desired access to service access flags
            if (desiredAccess & SERVICE_QUERY_CONFIG) {
                accessMask |= SERVICE_QUERY_CONFIG;
            }
            if (desiredAccess & SERVICE_CHANGE_CONFIG) {
                accessMask |= SERVICE_CHANGE_CONFIG;
            }
            if (desiredAccess & (SERVICE_START | SERVICE_STOP)) {
                accessMask |= SERVICE_START | SERVICE_STOP;
            }

            SC_HANDLE hService = OpenServiceW(hSCManager, objectName.c_str(), accessMask);

            CloseServiceHandle(hSCManager);

            if (hService) {
                CloseServiceHandle(hService);
                return true;
            }

            DWORD error = GetLastError();
            if (error == ERROR_ACCESS_DENIED) {
                return false;  // Access denied means we don't have permission
            }

            if (error == ERROR_SERVICE_DOES_NOT_EXIST) {
                throw PermissionHandlerException(ErrorCode::ResourceNotFound,
                                                 "Service not found: " + resourcePath);
            }

            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to check service access: " +
                                                 getLastErrorMessage());
        }

        // Fallback for other resource types or if direct access check fails
        throw PermissionHandlerException(ErrorCode::InvalidArgument,
                                         "Access check not implemented for this resource type: " +
                                             std::to_string(static_cast<int>(resourceType)));
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception in checkAccess: " + std::string(ex.what()));
    }
}

void PermissionHandlerWindows::takeOwnership(const std::string& resourcePath,
                                             ResourceType resourceType) {
    try {
        // Get the current user's SID
        HANDLE hToken;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            throw PermissionHandlerException(
                ErrorCode::SecurityError, "Failed to open process token: " + getLastErrorMessage());
        }

        DWORD tokenInfoLen = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &tokenInfoLen);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            CloseHandle(hToken);
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to get token information size: " +
                                                 getLastErrorMessage());
        }

        std::vector<BYTE> tokenInfoBuffer(tokenInfoLen);
        TOKEN_USER* tokenUser = reinterpret_cast<TOKEN_USER*>(tokenInfoBuffer.data());

        if (!GetTokenInformation(hToken, TokenUser, tokenUser, tokenInfoLen, &tokenInfoLen)) {
            CloseHandle(hToken);
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to get token information: " +
                                                 getLastErrorMessage());
        }

        CloseHandle(hToken);

        // Get object information
        SE_OBJECT_TYPE objectType;
        std::wstring objectName;
        resourceToWindowsObject(resourcePath, resourceType, objectType, objectName);

        // Enable take ownership privilege - 使用公共接口
        auto priv = adjustPrivilege("SeTakeOwnershipPrivilege", true);

        if (!priv) {
            // If we can't enable the privilege, we can't take ownership
            throw PermissionHandlerException(
                ErrorCode::SecurityError,
                "Failed to enable take ownership privilege. Run as administrator.");
        }

        // Apply the ownership change
        DWORD setInfoError = SetNamedSecurityInfoW(const_cast<LPWSTR>(objectName.c_str()),
                                                   objectType, OWNER_SECURITY_INFORMATION,
                                                   tokenUser->User.Sid, nullptr, nullptr, nullptr);

        // Always try to disable the privilege when done
        adjustPrivilege("SeTakeOwnershipPrivilege", false);

        if (setInfoError != ERROR_SUCCESS) {
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to take ownership: " + getLastErrorMessage());
        }
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception in takeOwnership: " + std::string(ex.what()));
    }
}

bool PermissionHandlerWindows::isRunningAsAdministrator() const {
    try {
        BOOL isAdmin = FALSE;
        HANDLE hToken = nullptr;

        // Open the process token
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            throw PermissionHandlerException(
                ErrorCode::SecurityError, "Failed to open process token: " + getLastErrorMessage());
        }

        // Get token elevation information
        TOKEN_ELEVATION_TYPE elevationType;
        DWORD returnLength = 0;

        if (GetTokenInformation(hToken, TokenElevationType, &elevationType, sizeof(elevationType),
                                &returnLength)) {
            isAdmin = (elevationType == TokenElevationTypeFull);
        }

        // If TOKEN_ELEVATION_TYPE is not supported (pre-Vista), try traditional SID check
        if (!isAdmin) {
            // Allocate and initialize a SID for the Administrators group
            SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
            PSID adminGroup = nullptr;

            if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
                // Check if the token contains admin SID
                if (!CheckTokenMembership(nullptr, adminGroup, &isAdmin)) {
                    FreeSid(adminGroup);
                    CloseHandle(hToken);
                    throw PermissionHandlerException(ErrorCode::SecurityError,
                                                     "Failed to check token membership: " +
                                                         getLastErrorMessage());
                }
                FreeSid(adminGroup);
            }
        }

        CloseHandle(hToken);
        return isAdmin != FALSE;
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception checking administrator privileges: " +
                                             std::string(ex.what()));
    }
}

bool PermissionHandlerWindows::adjustPrivilege(const std::string& privilegeName, bool enable) {
    try {
        // Convert to wide string and call internal implementation
        return adjustPrivilegeInternal(utf8ToWide(privilegeName), enable);
    } catch (const PermissionHandlerException& e) {
        throw e;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Failed to convert privilege name: " +
                                             std::string(ex.what()));
    }
}

// Internal implementation method - using wide strings
bool PermissionHandlerWindows::adjustPrivilegeInternal(const std::wstring& privilegeName,
                                                       bool enable) {
    try {
        // Open process token for adjusting privileges
        HANDLE hToken;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                              &hToken)) {
            throw PermissionHandlerException(
                ErrorCode::SecurityError, "Failed to open process token: " + getLastErrorMessage());
        }

        // Get LUID for the privilege
        TOKEN_PRIVILEGES tp;
        LUID luid;

        if (!LookupPrivilegeValueW(nullptr, privilegeName.c_str(), &luid)) {
            CloseHandle(hToken);
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to lookup privilege value: " +
                                                 getLastErrorMessage());
        }

        // Set privilege information
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

        // Adjust the token privileges
        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr,
                                   nullptr)) {
            CloseHandle(hToken);
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to adjust token privileges: " +
                                                 getLastErrorMessage());
        }

        // Check if the adjustment was successful
        DWORD error = GetLastError();
        CloseHandle(hToken);

        return error != ERROR_NOT_ALL_ASSIGNED;
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(
            ErrorCode::SecurityError, "Exception adjusting privilege: " + std::string(ex.what()));
    }
}

bool PermissionHandlerWindows::elevateToAdministrator(const ElevationParameters& params) {
    try {
        // Check if we are already running as administrator
        auto isAdmin = isRunningAsAdministrator();
        if (isAdmin) {
            return isAdmin;
        }

        // Get the path of the current executable
        wchar_t exePath[MAX_PATH];
        if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH)) {
            throw PermissionHandlerException(ErrorCode::SystemError,
                                             "Failed to get current executable path: " +
                                                 getLastErrorMessage());
        }

        // Prepare ShellExecuteEx parameters
        SHELLEXECUTEINFOW shellExecuteInfo = {sizeof(shellExecuteInfo)};

        shellExecuteInfo.lpVerb = L"runas";  // Request elevation
        shellExecuteInfo.lpFile = exePath;

        // Set working directory
        std::wstring workingDir;
        if (!params.workingDirectory.empty()) {
            workingDir = utf8ToWide(params.workingDirectory);
            shellExecuteInfo.lpDirectory = workingDir.c_str();
        }

        // Set command line arguments
        std::wstring arguments;
        if (!params.arguments.empty()) {
            arguments = utf8ToWide(params.arguments);
            shellExecuteInfo.lpParameters = arguments.c_str();
        }

        // Set show command
        shellExecuteInfo.nShow = params.showCmd;

        // Request process handle if we need to wait
        if (params.waitForElevation) {
            shellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        }

        // Execute the elevated process
        BOOL result = ShellExecuteExW(&shellExecuteInfo);

        if (!result) {
            DWORD error = GetLastError();

            // ERROR_CANCELLED indicates the user declined the elevation prompt
            if (error == ERROR_CANCELLED) {
                return false;  // User declined elevation
            }
            throw PermissionHandlerException(ErrorCode::SecurityError,
                                             "Failed to elevate process: " + getLastErrorMessage());
        }

        // Wait for the elevated process if requested
        if (params.waitForElevation && shellExecuteInfo.hProcess) {
            WaitForSingleObject(shellExecuteInfo.hProcess, INFINITE);
            CloseHandle(shellExecuteInfo.hProcess);
        }

        // Indicate success (process is now running elevated)
        return true;
    } catch (const PermissionHandlerException&) {
        throw;  // Rethrow custom engine
    } catch (const std::exception& ex) {
        throw PermissionHandlerException(ErrorCode::SecurityError,
                                         "Exception elevating process: " + std::string(ex.what()));
    }
}

// Factory function
std::shared_ptr<IPermissionHandler> createPermissionHandler() {
    return std::make_shared<PermissionHandlerWindows>();
}

}  // namespace system_kit
}  // namespace leigod