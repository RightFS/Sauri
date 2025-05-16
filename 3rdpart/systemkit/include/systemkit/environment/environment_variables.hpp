/**
 * @file environment_variables.hpp
 * @brief Interface for environment variable managers.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-13 00:25:10 (UTC)
 * Author: chenxu
 */

#ifndef LEIGOD_INCLUDE_SYSTEMKIT_ENVIRONMENT_MANAGER_HPP
#define LEIGOD_INCLUDE_SYSTEMKIT_ENVIRONMENT_MANAGER_HPP

#include "systemkit/core/result.hpp"
#include "systemkit/exceptions/exceptions.hpp"

#include <map>
#include <memory>
#include <string>

namespace leigod {
namespace system_kit {

/**
 * @brief Scope of environment variables
 *
 * Defines the different scopes where environment variables can exist
 */
enum class EnvVarScope {
    Process,  ///< Current process environment
    User,     ///< User-specific environment (persists between sessions)
    System    ///< System-wide environment (affects all users)
};

/**
 * @class IEnvironmentVariables
 * @brief Interface for environment variables operations
 *
 * This interface provides methods to manage environment variables
 * at different scopes (process, user, system).
 */
class IEnvironmentVariables {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IEnvironmentVariables() = default;

    /**
     * @brief Get an environment variable value
     *
     * @param name The name of the environment variable to retrieve
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return std::string The value of the environment variable if successful
     * @throws EnvironmentVariableException
     */
    virtual std::string get(const std::string& name,
                            EnvVarScope scope = EnvVarScope::Process) const = 0;

    /**
     * @brief Set an environment variable
     *
     * @param name The name of the environment variable to set
     * @param value The value to set
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return void Success if the operation succeeded
     * @throws EnvironmentVariableException
     */
    virtual void set(const std::string& name, const std::string& value,
                     EnvVarScope scope = EnvVarScope::Process) = 0;

    /**
     * @brief Remove an environment variable
     *
     * @param name The name of the environment variable to remove
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return void Success if the operation succeeded
     * @throws EnvironmentVariableException
     */
    virtual void remove(const std::string& name, EnvVarScope scope = EnvVarScope::Process) = 0;

    /**
     * @brief Check if an environment variable exists
     *
     * @param name The name of the environment variable to check
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return bool True if the environment variable exists, false otherwise
     * @throws EnvironmentVariableException
     */
    virtual bool exists(const std::string& name,
                        EnvVarScope scope = EnvVarScope::Process) const = 0;

    /**
     * @brief Get all environment variables for the specified scope
     *
     * @param scope The scope of environment variables to retrieve (Process, User, or System)
     * @return std::map<std::string, std::string> Map of environment variables if successful
     * @throws EnvironmentVariableException
     */
    virtual std::map<std::string, std::string>
    getAll(EnvVarScope scope = EnvVarScope::Process) const = 0;

    /**
     * @brief Expands environment variables references in a string
     *
     * For example, converts "%PATH%" to the actual PATH value.
     *
     * @param input String containing environment variables references
     * @return std::string String with environment variables expanded if successful
     * @throws EnvironmentVariableException
     */
    virtual std::string expand(const std::string& input) const = 0;
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_INCLUDE_SYSTEMKIT_ENVIRONMENT_MANAGER_HPP