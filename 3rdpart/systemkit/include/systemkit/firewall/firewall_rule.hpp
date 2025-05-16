/**
 * @file firewall_rule.hpp
 * @brief Represents a firewall rule.
 * @details This class defines the interface for managing firewall rules across different platforms.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_SYSTEM_KIT_FIREWALL_FIREWALL_RULE_HPP
#define LEIGOD_SYSTEM_KIT_FIREWALL_FIREWALL_RULE_HPP

#include <string>

namespace leigod {
namespace system_kit {

/**
 * @brief Firewall rule action (allow or block)
 */
enum class FirewallAction { Allow, Block };

/**
 * @brief Firewall rule direction (inbound or outbound)
 */
enum class FirewallDirection { Inbound, Outbound };

/**
 * @brief Common protocol values for firewall rules
 */
struct FirewallProtocol {
    static constexpr int Any = 0;
    static constexpr int ICMP = 1;
    static constexpr int TCP = 6;
    static constexpr int UDP = 17;
};

/**
 * @brief Represents a firewall rule
 */
class FirewallRule {
public:
    /**
     * @brief Default constructor creates an empty rule
     */
    FirewallRule()
        : protocol(FirewallProtocol::Any),
          action(FirewallAction::Allow),
          direction(FirewallDirection::Inbound),
          enabled(true) {}

    /**
     * @brief Create a firewall rule with basic properties
     *
     * @param ruleName Name of the rule
     * @param app Path to the application
     * @param ruleAction Allow or block
     * @param ruleDirection Inbound or outbound
     */
    FirewallRule(const std::string& ruleName, const std::string& app, FirewallAction ruleAction,
                 FirewallDirection ruleDirection)
        : name(ruleName),
          applicationPath(app),
          protocol(FirewallProtocol::Any),
          action(ruleAction),
          direction(ruleDirection),
          enabled(true) {}

    /**
     * @brief Set local ports for this rule
     *
     * @param ports Port string (e.g., "80,443" or "1000-2000")
     * @return Reference to this rule for method chaining
     */
    FirewallRule& setLocalPorts(const std::string& ports) {
        localPorts = ports;
        return *this;
    }

    /**
     * @brief Set remote ports for this rule
     *
     * @param ports Port string (e.g., "80,443" or "1000-2000")
     * @return Reference to this rule for method chaining
     */
    FirewallRule& setRemotePorts(const std::string& ports) {
        remotePorts = ports;
        return *this;
    }

    /**
     * @brief Set protocol for this rule
     *
     * @param proto Protocol number (use FirewallProtocol constants)
     * @return Reference to this rule for method chaining
     */
    FirewallRule& setProtocol(int proto) {
        protocol = proto;
        return *this;
    }

    /**
     * @brief Set local addresses for this rule
     *
     * @param addresses Address string (e.g., "192.168.1.1" or "192.168.1.0/24")
     * @return Reference to this rule for method chaining
     */
    FirewallRule& setLocalAddresses(const std::string& addresses) {
        localAddresses = addresses;
        return *this;
    }

    /**
     * @brief Set remote addresses for this rule
     *
     * @param addresses Address string (e.g., "192.168.1.1" or "192.168.1.0/24")
     * @return Reference to this rule for method chaining
     */
    FirewallRule& setRemoteAddresses(const std::string& addresses) {
        remoteAddresses = addresses;
        return *this;
    }

    /**
     * @brief Enable or disable this rule
     *
     * @param isEnabled Whether the rule should be enabled
     * @return Reference to this rule for method chaining
     */
    FirewallRule& setEnabled(bool isEnabled) {
        enabled = isEnabled;
        return *this;
    }

    bool operator==(const FirewallRule& other) const {
        return name == other.name && direction == other.direction && action == other.action &&
               protocol == other.protocol && enabled == other.enabled;
    }

    // Rule properties
    std::string name;             ///< Rule name
    std::string applicationPath;  ///< Path to the application
    std::string description;      ///< Rule description
    int protocol;                 ///< Protocol (TCP=6, UDP=17, etc.)
    std::string localPorts;       ///< Local ports affected
    std::string remotePorts;      ///< Remote ports affected
    std::string localAddresses;   ///< Local addresses affected
    std::string remoteAddresses;  ///< Remote addresses affected
    FirewallAction action;        ///< Allow or block
    FirewallDirection direction;  ///< Inbound or outbound
    bool enabled;                 ///< Whether the rule is enabled
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SYSTEM_KIT_FIREWALL_FIREWALL_RULE_HPP