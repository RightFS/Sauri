/**
 * @file environment_manager_macos.hpp
 * @brief Environment manager implementation for macOS.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-13 00:25:10 (UTC)
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_MANAGER_ENVIRONMENT_MANAGER_MACOS_HPP
#define LEIGOD_SRC_MANAGER_ENVIRONMENT_MANAGER_MACOS_HPP

#include "systemkit/environment/environment_variables.hpp"

#include <string>
#include <unordered_map>

namespace leigod {
namespace manager {

class EnvironmentVariablesMacOS : public IEnvironmentVariables {
public:
    std::string getEnv(const std::string& name) const override;
    void setEnv(const std::string& name, const std::string& value) override;
    std::unordered_map<std::string, std::string> getAllEnv() const override;
    std::string expandEnvVars(const std::string& path) const override;
};

}  // namespace manager
}  // namespace leigod

#endif  // LEIGOD_SRC_MANAGER_ENVIRONMENT_MANAGER_MACOS_HPP