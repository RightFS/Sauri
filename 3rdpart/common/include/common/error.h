/**
 * @file error.hpp
 * @brief Error codes and error class for system operations.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-26
 * Author: chenxu
 */

#ifndef LEIGOD_COMMON_ERROR_HPP
#define LEIGOD_COMMON_ERROR_HPP

#include <string>

namespace leigod {
namespace common {

/**
 * @brief Error codes for system operations
 */
enum class ErrorCode : int {
    // General errors (0-99)
    Success = 0,         ///< Operation completed successfully
    Unknown,             ///< Unknown or unspecified error
    InvalidArgument,     ///< Invalid argument provided
    InvalidOperation,    ///< Operation not valid in current state
    NotImplemented,      ///< Operation not implemented
    NotSupported,        ///< Operation not supported on this platform
    OperationCancelled,  ///< Operation cancelled by user or system
    Timeout,             ///< Operation timed out
    ComInitError,        ///< COM initialization error

    // Resource errors (100-199)
    ResourceNotFound = 100,  ///< Requested resource not found
    ResourceBusy,            ///< Resource is in use by another process
    ResourceExhausted,       ///< Resource has been exhausted (e.g., disk full)
    ResourceLocked,          ///< Resource is locked
    ResourceExists,          ///< Resource already exists

    // System errors (200-299)
    SystemError = 200,  ///< General system error
    OutOfMemory,        ///< Insufficient memory to complete operation
    OutOfDiskSpace,     ///< Insufficient disk space
    DeviceError,        ///< Hardware device error

    // File system errors (300-399)
    FileError = 300,    ///< General file error
    FileNotFound,       ///< File not found
    FileExists,         ///< File already exists
    FileAccessDenied,   ///< Permission denied for file operation
    FileTooLarge,       ///< File size exceeds limit
    DirectoryNotEmpty,  ///< Directory is not empty

    // Firewall errors (400-499)
    FireWallError = 400,      ///< General firewall error
    FireWallInitError,        ///< Firewall initialization error
    FireWallInitFwRuleError,  ///< Firewall initialization rule error
    FireWallRuleNotFound,     ///< Firewall rule not found
    FireWallAccessDenied,     ///< Firewall access denied
    FireWallComError,         ///< Firewall COM error

    // Security errors (500-599)
    SecurityError = 500,   ///< General security error
    AuthenticationFailed,  ///< Authentication failed
    PermissionDenied,      ///< Permission denied
    TokenExpired,          ///< Security token expired
    OperationFailed,       ///< Security operation failed

    // Registry errors (600-699)
    RegistryError = 600,         ///< General registry error
    RegistryTypeError,           ///< Registry type error
    RegistryKeyNotFound,         ///< Registry key not found
    RegistryValueNotFound,       ///< Registry value not found
    RegistryAccessDenied,        ///< Registry access denied
    RegistryOperationFailed,     ///< Registry operation failed
    RegistryGetValueSizeFailed,  ///< Registry get value size failed
    RegistryQueryFailed,         ///< Registry query failed
    RegistryEnumFailed,          ///< Registry enumeration failed
    RegistryCreateFailed,        ///< Registry creation failed
    RegistrySetValueFailed,      ///< Registry set value failed

    // Process and service errors (700-799)
    ProcessError = 700,      ///< General process error
    ProcessStartFailed,      ///< Failed to start process
    ProcessNotFound,         ///< Process not found
    ProcessAccessDenied,     ///< Process access denied
    ProcessOperationFailed,  ///< Process operation failed
    ServiceError,            ///< General service error
    ServiceNotFound,         ///< Service not found
    ServiceAccessDenied,     ///< Service access denied
    ServiceOperationFailed,  ///< Service operation failed

    // Environment variables errors (800-899)
    EnvironmentVariableError,         ///< General environment variable error
    EnvironmentVariableNotFound,      ///< Environment variable not found
    EnvironmentVariableAccessDenied,  ///< Environment variable access denied

    // Add more error codes here...(900-999)
    ConvertError = 900,      ///< Convert error3
    ConvertErrorNotString,   ///< Convert error not string
    ConvertErrorNotNumber,   ///< Convert error not number
    ConvertErrorNotInteger,  ///< Convert error not integer
    ConvertErrorNotDouble,   ///< Convert error not double
    ConvertErrorNotBoolean,  ///< Convert error not boolean
    ConvertErrorNotObject,   ///< Convert error not object
    ConvertErrorNotArray,    ///< Convert error not array
    ConvertErrorOutOfRange,  ///< Convert error out of range
};

/**
 * @brief Convert error code to string representation
 * @param code The error code
 * @return String representation of the error code
 */
inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        // General errors
        case ErrorCode::Success:
            return "Success";
        case ErrorCode::Unknown:
            return "Unknown error";
        case ErrorCode::InvalidArgument:
            return "Invalid argument";
        case ErrorCode::InvalidOperation:
            return "Invalid operation";
        case ErrorCode::NotImplemented:
            return "Not implemented";
        case ErrorCode::NotSupported:
            return "Not supported";
        case ErrorCode::OperationCancelled:
            return "Operation cancelled";
        case ErrorCode::Timeout:
            return "Timeout";

        // Resource errors
        case ErrorCode::ResourceNotFound:
            return "Resource not found";
        case ErrorCode::ResourceBusy:
            return "Resource busy";
        case ErrorCode::ResourceExhausted:
            return "Resource exhausted";
        case ErrorCode::ResourceLocked:
            return "Resource locked";
        case ErrorCode::ResourceExists:
            return "Resource already exists";

        // System errors
        case ErrorCode::SystemError:
            return "System error";
        case ErrorCode::OutOfMemory:
            return "Out of memory";
        case ErrorCode::OutOfDiskSpace:
            return "Out of disk space";
        case ErrorCode::DeviceError:
            return "Device error";

        // File system errors
        case ErrorCode::FileError:
            return "File error";
        case ErrorCode::FileNotFound:
            return "File not found";
        case ErrorCode::FileExists:
            return "File already exists";
        case ErrorCode::FileAccessDenied:
            return "File access denied";
        case ErrorCode::FileTooLarge:
            return "File too large";
        case ErrorCode::DirectoryNotEmpty:
            return "Directory not empty";

        // Network errors
        case ErrorCode::FireWallError:
            return "Firewall error";
        case ErrorCode::FireWallInitError:
            return "Firewall initialization error";
        case ErrorCode::FireWallInitFwRuleError:
            return "Firewall initialization rule error";
        case ErrorCode::FireWallRuleNotFound:
            return "Firewall rule not found";
            //        case ErrorCode::NetworkUnreachable:
            //            return "Network unreachable";

        // Security errors
        case ErrorCode::SecurityError:
            return "Security error";
        case ErrorCode::AuthenticationFailed:
            return "Authentication failed";
        case ErrorCode::PermissionDenied:
            return "Permission denied";
        case ErrorCode::TokenExpired:
            return "Token expired";

        // Registry errors
        case ErrorCode::RegistryError:
            return "Registry error";
        case ErrorCode::RegistryKeyNotFound:
            return "Registry key not found";
        case ErrorCode::RegistryValueNotFound:
            return "Registry value not found";
        case ErrorCode::RegistryAccessDenied:
            return "Registry access denied";
        case ErrorCode::RegistryOperationFailed:
            return "Registry operation failed";

        // Process and service errors
        case ErrorCode::ProcessError:
            return "Process error";
        case ErrorCode::ProcessStartFailed:
            return "Process start failed";
        case ErrorCode::ProcessNotFound:
            return "Process not found";
        case ErrorCode::ProcessAccessDenied:
            return "Process access denied";
        case ErrorCode::ProcessOperationFailed:
            return "Process operation failed";
        case ErrorCode::ServiceError:
            return "Service error";
        case ErrorCode::ServiceNotFound:
            return "Service not found";
        case ErrorCode::ServiceAccessDenied:
            return "Service access denied";
        case ErrorCode::ServiceOperationFailed:
            return "Service operation failed";

        // Environment variables errors
        case ErrorCode::EnvironmentVariableError:
            return "Environment variable error";
        case ErrorCode::EnvironmentVariableNotFound:
            return "Environment variable not found";
        case ErrorCode::EnvironmentVariableAccessDenied:
            return "Environment variable access denied";

        default:
            return "Undefined error";
    }
}

}  // namespace common
}  // namespace leigod

#endif  // LEIGOD_COMMON_ERROR_HPP