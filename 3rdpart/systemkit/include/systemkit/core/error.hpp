/**
 * @file error.hpp
 * @brief Error codes and error class for system operations.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-26
 * Author: chenxu
 */

#ifndef LEIGOD_SYSTEM_KIT_ERROR_HPP
#define LEIGOD_SYSTEM_KIT_ERROR_HPP

#include <string>

namespace leigod {
namespace system_kit {

/**
 * @brief Error codes for system operations
 */
enum class ErrorCode {
    // General errors (0-99)
    Success = 0,             ///< Operation completed successfully
    Unknown = 1,             ///< Unknown or unspecified error
    InvalidArgument = 2,     ///< Invalid argument provided
    InvalidOperation = 3,    ///< Operation not valid in current state
    NotImplemented = 4,      ///< Operation not implemented
    NotSupported = 5,        ///< Operation not supported on this platform
    OperationCancelled = 6,  ///< Operation cancelled by user or system
    Timeout = 7,             ///< Operation timed out

    // Resource errors (100-199)
    ResourceNotFound = 100,   ///< Requested resource not found
    ResourceBusy = 101,       ///< Resource is in use by another process
    ResourceExhausted = 102,  ///< Resource has been exhausted (e.g., disk full)
    ResourceLocked = 103,     ///< Resource is locked
    ResourceExists = 104,     ///< Resource already exists

    // System errors (200-299)
    SystemError = 200,     ///< General system error
    OutOfMemory = 201,     ///< Insufficient memory to complete operation
    OutOfDiskSpace = 202,  ///< Insufficient disk space
    DeviceError = 203,     ///< Hardware device error

    // File system errors (300-399)
    FileError = 300,          ///< General file error
    FileNotFound = 301,       ///< File not found
    FileExists = 302,         ///< File already exists
    FileAccessDenied = 303,   ///< Permission denied for file operation
    FileTooLarge = 304,       ///< File size exceeds limit
    DirectoryNotEmpty = 305,  ///< Directory is not empty

    // Network errors (400-499)
    NetworkError = 400,        ///< General network error
    ConnectionFailed = 401,    ///< Connection failed
    ConnectionClosed = 402,    ///< Connection closed by peer
    HostNotFound = 403,        ///< Host not found
    NetworkUnreachable = 404,  ///< Network is unreachable

    // Security errors (500-599)
    SecurityError = 500,         ///< General security error
    AccessDenied = 501,          ///< Access denied
    AuthenticationFailed = 502,  ///< Authentication failed
    PermissionDenied = 503,      ///< Permission denied
    TokenExpired = 504,          ///< Security token expired
    RuleNotFound = 505,          ///< Security rule not found
    OperationFailed = 506,       ///< Security operation failed

    // Registry errors (600-699)
    RegistryError = 600,            ///< General registry error
    RegistryKeyNotFound = 601,      ///< Registry key not found
    RegistryValueNotFound = 602,    ///< Registry value not found
    RegistryAccessDenied = 603,     ///< Registry access denied
    RegistryOperationFailed = 604,  ///< Registry operation failed

    // Process and service errors (700-799)
    ProcessError = 700,            ///< General process error
    ProcessStartFailed = 701,      ///< Failed to start process
    ProcessNotFound = 702,         ///< Process not found
    ProcessAccessDenied = 703,     ///< Process access denied
    ProcessOperationFailed = 704,  ///< Process operation failed
    ServiceError = 750,            ///< General service error
    ServiceNotFound = 751,         ///< Service not found
    ServiceAccessDenied = 752,     ///< Service access denied
    ServiceOperationFailed = 753,  ///< Service operation failed

    // Environment variables errors (800-899)
    EnvironmentVariableError = 800,         ///< General environment variable error
    EnvironmentVariableNotFound = 801,      ///< Environment variable not found
    EnvironmentVariableAccessDenied = 802,  ///< Environment variable access denied
    EnvironmentQueryError,                  ///< Environment variable query error
    EnvironmentTypeError,                   ///< Environment variable type error
    EnvironmentSizeError,                   ///< Environment variable get size error
    EnvironmentGetError,                    ///< Environment variable get error
    EnvironmentCheckAdminError,             ///< Environment variable check user is admin error
    EnvironmentExpandError,                 ///< Environment variable expand error

    // Add more error codes here...(900-999)
    ConvertError = 900,            ///< Convert error
    ConvertErrorNotNumber = 901,   ///< Convert error not number
    ConvertErrorOutOfRange = 902,  ///< Convert error out of range
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
        case ErrorCode::NetworkError:
            return "Network error";
        case ErrorCode::ConnectionFailed:
            return "Connection failed";
        case ErrorCode::ConnectionClosed:
            return "Connection closed";
        case ErrorCode::HostNotFound:
            return "Host not found";
        case ErrorCode::NetworkUnreachable:
            return "Network unreachable";

        // Security errors
        case ErrorCode::SecurityError:
            return "Security error";
        case ErrorCode::AccessDenied:
            return "Access denied";
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

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SYSTEM_KIT_ERROR_HPP