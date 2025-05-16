#include "permission_handler_macos.hpp"

#include <iostream>
#include <unistd.h>

using namespace leigod::manager;

bool PermissionHandlerMacOS::checkPermission() {
    // MacOS-specific permission check
    return geteuid() == 0;
}

bool PermissionHandlerMacOS::requestPermission() {
    // Request elevated permissions on MacOS
    if (!checkPermission()) {
        std::cout << "Root permissions required. Please run the application as root." << std::endl;
        return false;
    }
    return true;
}

bool PermissionHandlerMacOS::performOperation(const std::string& operation) {
    if (!checkPermission() && !requestPermission()) {
        return false;
    }
    // Perform the operation
    std::cout << "Performing operation: " << operation << std::endl;
    return true;
}

// Factory function
std::shared_ptr<PermissionManager> createPermissionManager() {
    return std::make_shared<PermissionHandlerMacOS>();
}