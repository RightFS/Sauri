/**
 * @file windows_firewall_manager.hpp
 * @brief Implementation of IFirewallController for Windows platform.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_WINDOWS_FIREWALL_MANAGER_WINDOWS_HPP
#define LEIGOD_SRC_WINDOWS_FIREWALL_MANAGER_WINDOWS_HPP

#include "systemkit/firewall/firewall_controller.hpp"

#include <vector>

namespace leigod {
namespace system_kit {

/**
 * @class FirewallControllerWindows
 * @brief Windows platform implementation of firewall controller
 *
 * This class provides access to the Windows Firewall API to manage firewall rules
 * and control firewall settings on Windows platforms.
 */
class FirewallControllerWindows : public IFirewallController {
public:
    /**
     * @brief Constructor
     */
    FirewallControllerWindows() = default;

    /**
     * @brief Destructor
     */
    ~FirewallControllerWindows() override = default;

    /**
     * @brief Add a new firewall rule
     *
     * @param rule The firewall rule to add
     * @return void Success if rule was added, Failure otherwise
     * @throw FirewallControllerException
     */
    void addRule(const FirewallRule& rule) override;

    /**
     * @brief Update an existing firewall rule
     *
     * @param ruleName The name of the firewall rule to update
     * @param updatedRule The new rule configuration
     * @return void Success if the rule was updated
     * @throw FirewallControllerException
     */
    void updateRule(const std::string& ruleName, const FirewallRule& updatedRule) override;

    /**
     * @brief Remove an existing firewall rule
     *
     * @param ruleName Name of the rule to remove
     * @return void Success if rule was removed, Failure otherwise
     * @throw FirewallControllerException
     */
    void removeRule(const std::string& ruleName) override;

    /**
     * @brief Check if a rule with the given name exists
     *
     * @param ruleName Name of the rule to check
     * @return bool Success with true if rule exists, false otherwise
     * @throw FirewallControllerException
     */
    bool ruleExists(const std::string& ruleName) const override;

    /**
     * @brief Get all firewall rules
     *
     * @return std::vector<FirewallRule> List of all firewall rules if successful
     * @throw FirewallControllerException
     */
    std::vector<FirewallRule> getRules() const override;

    /**
     * @brief Get the current firewall status
     *
     * @return FirewallStatus> The current firewall status if successful
     * @throw FirewallControllerException
     */
    FirewallStatus getStatus() const override;

    /**
     * @brief Set the firewall status
     *
     * @param status The status to set
     * @return void Success if status was set, Failure otherwise
     * @throw FirewallControllerException
     */
    void setStatus(FirewallStatus status) override;

    /**
     * @brief Get a firewall rule by name
     *
     * @param ruleName The name of the firewall rule to retrieve
     * @return std::vector<FirewallRule> The firewall rule if found, error otherwise
     * @throw FirewallControllerException
     */
    std::vector<FirewallRule> getRule(const std::string& ruleName) const override;

private:
    // Private implementation details are kept in the .cpp file
};
}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_WINDOWS_FIREWALL_MANAGER_WINDOWS_HPP