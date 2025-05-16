/**
 * @file macos_firewall_manager.cpp
 * @brief Implementation of FirewallManager for macOS platform.
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#include "firewall_controller_macos.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace leigod::manager;

bool FirewallControllerMacOS::allowApplication(const std::string& appPath,
                                               const std::string& ruleName) {
    // 允许应用程序通过防火墙的逻辑
    try {
        std::string command =
            "sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add " + appPath;
        if (system(command.c_str()) != 0) {
            throw std::runtime_error("Failed to add firewall rule");
        }
        std::cout << "Allowing application " << appPath << " with rule name " << ruleName
                  << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error allowing application: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool FirewallControllerMacOS::removeApplication(const std::string& ruleName) {
    // 移除防火墙规则的逻辑
    try {
        std::string command =
            "sudo /usr/libexec/ApplicationFirewall/socketfilterfw --remove " + ruleName;
        if (system(command.c_str()) != 0) {
            throw std::runtime_error("Failed to remove firewall rule");
        }
        std::cout << "Removing firewall rule with name " << ruleName << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error removing rule: " << e.what() << std::endl;
        return false;
    }

    return true;
}