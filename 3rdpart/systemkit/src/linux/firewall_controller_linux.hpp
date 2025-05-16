/**
 * @file linux_firewall_manager.hpp
 * @brief Implementation of FirewallManager for Linux platform.
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_MANAGER_FIREWALL_MANAGER_LINUX_HPP
#define LEIGOD_SRC_MANAGER_FIREWALL_MANAGER_LINUX_HPP
#include "systemkit/firewall/firewall_controller.hpp"

namespace leigod {
namespace manager {
/**
 * @class LinuxFirewallManager
 * @brief Linux-specific implementation of FirewallManager.
 */
class FirewallControllerLinux : public IFirewallController {
public:
    bool allowApplication(const std::string& appPath, const std::string& ruleName) override;

    bool removeApplication(const std::string& ruleName) override;
};

}  // namespace manager
}  // namespace leigod

#endif  // LEIGOD_SRC_MANAGER_FIREWALL_MANAGER_LINUX_HPP