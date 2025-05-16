/**
 * @file systemkit_exceptions.hpp
 * @brief Abstract factory for instruction of steam
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-17 00:25:10 (UTC)
 * Author: chenxu
 */

#ifndef LEIGOD_SYSTEMKIT_INCLUDE_SYSTEMKIT_EXCEPTIONS_HPP
#define LEIGOD_SYSTEMKIT_INCLUDE_SYSTEMKIT_EXCEPTIONS_HPP

#include "common/exception.h"

#include <stdexcept>
#include <string>

namespace leigod {
namespace system_kit {
/**
 * @brief Base class for system kit exceptions.
 *
 * This class serves as the base class for all exceptions
 * thrown by the system kit. It extends the standard
 * std::engine class and provides a constructor to
 * set the error message.
 */
class SystemKitException : public common::Exception {
public:
    /**
     * @brief Constructor for SystemKitException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit SystemKitException(T code, const std::string& message)
        : common::Exception(code, message) {}
};

/**
 * @brief Exception class for environment variable errors.
 *
 * This class represents errors that occur in the environment
 * system_kit component of the system kit.
 */
class EnvironmentVariableException : public SystemKitException {
public:
    /**
     * @brief Constructor for EnvironmentVariableException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit EnvironmentVariableException(T code, const std::string& message)
        : SystemKitException(code, message) {}
};

/**
 * @brief Exception class for firewall controller errors.
 *
 * This class represents errors that occur in the firewall
 * system_kit component of the system kit.
 */
class FirewallControllerException : public SystemKitException {
public:
    /**
     * @brief Constructor for FirewallControllerException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit FirewallControllerException(T code, const std::string& message)
        : SystemKitException(code, message) {}
};

/**
 * @brief Exception class for permission handler errors.
 *
 * This class represents errors that occur in the permission
 * system_kit component of the system kit.
 */
class PermissionHandlerException : public SystemKitException {
public:
    /**
     * @brief Constructor for PermissionHandlerException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit PermissionHandlerException(T code, const std::string& message)
        : SystemKitException(code, message) {}
};

/**
 * @brief Exception class for process launcher errors.
 *
 * This class represents errors that occur in the process
 * system_kit component of the system kit.
 */
class ProcessLauncherException : public SystemKitException {
public:
    /**
     * @brief Constructor for ProcessLauncherException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit ProcessLauncherException(T code, const std::string& message)
        : SystemKitException(code, message) {}
};

/**
 * @brief Exception class for process launcher CreateProcess errors.
 *
 * This class represents errors that occur in the process
 * system_kit component of the system kit.
 */
class ProcessCreateException : public ProcessLauncherException {
public:
    /**
     * @brief Constructor for ProcessLauncherException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit ProcessCreateException(T code, const std::string& message)
        : ProcessLauncherException(code, message) {}
};

/**
 * @brief Exception class for process launcher ShellExecuteEx errors.
 *
 * This class represents errors that occur in the process
 * system_kit component of the system kit.
 */
class ProcessShellExecuteExException : public ProcessLauncherException {
public:
    /**
     * @brief Constructor for ProcessLauncherException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit ProcessShellExecuteExException(T code, const std::string& message)
        : ProcessLauncherException(code, message) {}
};

/**
 * @brief Exception class for registry system_kit errors.
 *
 * This class represents errors that occur in the registry
 * system_kit component of the system kit.
 */
class RegistryManagerException : public SystemKitException {
public:
    /**
     * @brief Constructor for RegistryManagerException.
     * @param message The error message to be associated with the engine.
     */
    template <class T>
    explicit RegistryManagerException(T code, const std::string& message)
        : SystemKitException(code, message) {}
};
}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SYSTEMKIT_INCLUDE_SYSTEMKIT_EXCEPTIONS_HPP