/**
 * @file environment_variables_windows.hpp
 * @brief Environment system_kit implementation for Windows.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-13 00:25:10 (UTC)
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_WINDOWS_ENVIRONMENT_MANAGER_WINDOWS_HPP
#define LEIGOD_SRC_WINDOWS_ENVIRONMENT_MANAGER_WINDOWS_HPP

#include "systemkit/environment/environment_variables.hpp"

#include <vector>

namespace leigod {
namespace system_kit {

/**
 * @class EnvironmentVariablesWindows
 * @brief Windows platform implementation of environment variables management
 *
 * This class handles process, user, and system environment variables on Windows
 * using Win32 API and registry operations.
 */
class EnvironmentVariablesWindows : public IEnvironmentVariables {
public:
    /**
     * @brief Constructor
     */
    EnvironmentVariablesWindows() = default;

    /**
     * @brief Destructor
     */
    ~EnvironmentVariablesWindows() override = default;

    /**
     * @brief Get an environment variable value
     *
     * @param name The name of the environment variable to retrieve
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return std::string The value of the environment variable if successful
     * @throw EnvironmentVariableException
     */
    std::string get(const std::string& name,
                    EnvVarScope scope = EnvVarScope::Process) const override;

    /**
     * @brief Set an environment variable
     *
     * @param name The name of the environment variable to set
     * @param value The value to set
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return void Success if the operation succeeded
     * @throw EnvironmentVariableException
     */
    void set(const std::string& name, const std::string& value,
             EnvVarScope scope = EnvVarScope::Process) override;

    /**
     * @brief Remove an environment variable
     *
     * @param name The name of the environment variable to remove
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return void Success if the operation succeeded
     * @throw EnvironmentVariableException
     */
    void remove(const std::string& name, EnvVarScope scope = EnvVarScope::Process) override;

    /**
     * @brief Check if an environment variable exists
     *
     * @param name The name of the environment variable to check
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return bool True if the environment variable exists, false otherwise
     * @throw EnvironmentVariableException
     */
    bool exists(const std::string& name, EnvVarScope scope = EnvVarScope::Process) const override;

    /**
     * @brief Get all environment variables for the specified scope
     *
     * @param scope The scope of environment variables to retrieve (Process, User, or System)
     * @return std::map<std::string, std::string> Map of environment variables if successful
     * @throw EnvironmentVariableException
     */
    std::map<std::string, std::string>
    getAll(EnvVarScope scope = EnvVarScope::Process) const override;

    /**
     * @brief Expands environment variables references in a string
     *
     * For example, converts "%PATH%" to the actual PATH value.
     *
     * @param input String containing environment variables references
     * @return std::string String with environment variables expanded if successful
     * @throw EnvironmentVariableException
     */
    std::string expand(const std::string& input) const override;

private:
    /**
     * @brief Get the environment variable value from the specified scope
     *
     * @param scope The scope of the environment variable (Process, User, or System)
     * @return std::vector<std::string> The value of the environment variable if successful
     * @throw EnvironmentVariableException
     */
    std::vector<std::string> getNames(EnvVarScope scope) const;
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_WINDOWS_ENVIRONMENT_MANAGER_WINDOWS_HPP