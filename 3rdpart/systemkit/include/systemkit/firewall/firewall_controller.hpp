/**
 * @file firewall_controller.hpp
 * @brief Abstract base class for platform-specific firewall managers.
 * @details This class defines the interface for managing firewall rules across different platforms.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_INCLUDE_SYSTEMKIT_FIREWALL_FIREWALL_CONTROLLER_HPP
#define LEIGOD_INCLUDE_SYSTEMKIT_FIREWALL_FIREWALL_CONTROLLER_HPP

#include "firewall_rule.hpp"
#include "systemkit/exceptions/exceptions.hpp"

#include <memory>
#include <string>
#include <vector>

namespace leigod {
namespace system_kit {

/**
 * @brief Represents the status of the firewall
 */
struct FirewallStatus {
    bool domainProfileEnabled = false;   ///< Status for domain network profile
    bool privateProfileEnabled = false;  ///< Status for private network profile
    bool publicProfileEnabled = false;   ///< Status for public network profile
};

/**
 * @brief Interface for firewall operations
 */
class IFirewallController {
public:
    virtual ~IFirewallController() = default;

    /**
     * @brief Add a new firewall rule
     *
     * @param rule The rule to add
     * @return void Success if rule was added, Failure otherwise
     * @throw FirewallControllerException
     */
    virtual void addRule(const FirewallRule& rule) = 0;

    /**
     * @brief Update an existing firewall rule
     *
     * @param ruleName The name of the firewall rule to update
     * @param updatedRule The new rule configuration
     * @return void Success if the rule was updated
     * @throw FirewallControllerException
     */
    virtual void updateRule(const std::string& ruleName, const FirewallRule& updatedRule) = 0;

    /**
     * @brief Remove an existing firewall rule
     *
     * @param ruleName Name of the rule to remove
     * @return void Success if rule was removed, Failure otherwise
     * @throw FirewallControllerException
     */
    virtual void removeRule(const std::string& ruleName) = 0;

    /**
     * @brief Check if a rule with the given name exists
     *
     * @param ruleName Name of the rule to check
     * @return bool Success with true if rule exists, false otherwise
     * @throw FirewallControllerException
     */
    virtual bool ruleExists(const std::string& ruleName) const = 0;

    /**
     * @brief Get all firewall rules
     *
     * @return std::vector<FirewallRule> Success with list of rules if successful
     * @throw FirewallControllerException
     */
    virtual std::vector<FirewallRule> getRules() const = 0;

    /**
     * @brief Get the current firewall status
     *
     * @return FirewallStatus Success with status if successful
     * @throw FirewallControllerException
     */
    virtual FirewallStatus getStatus() const = 0;

    /**
     * @brief Set the firewall status
     *
     * @param status The status to set
     * @return void Success if status was set, Failure otherwise
     * @throw FirewallControllerException
     */
    virtual void setStatus(FirewallStatus status) = 0;

    /**
     * @brief Get a firewall rule by name
     *
     * @param ruleName The name of the firewall rule to retrieve
     * @return std::vector<FirewallRule> The firewall rule if found, error otherwise
     * @throw FirewallControllerException
     */
    virtual std::vector<FirewallRule> getRule(const std::string& ruleName) const = 0;
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_INCLUDE_SYSTEMKIT_FIREWALL_FIREWALL_CONTROLLER_HPP