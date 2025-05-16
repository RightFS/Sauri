#include "permission_handler_linux.hpp"

#include <iostream>
#include <unistd.h>

using namespace leigod::manager;

bool PermissionHandlerLinux::checkPermission() {
    // Linux-specific permission check
    return geteuid() == 0;
}

bool PermissionHandlerLinux::requestPermission() {
    // Request elevated permissions on Linux
    if (!checkPermission()) {
        std::cout << "Root permissions required. Please run the application as root." << std::endl;
        return false;
    }
    return true;
}

bool PermissionHandlerLinux::performOperation(const std::string& operation) {
    if (!checkPermission() && !requestPermission()) {
        return false;
    }
    // Perform the operation
    std::cout << "Performing operation: " << operation << std::endl;
    return true;
}

// Factory function
std::shared_ptr<PermissionManager> createPermissionManager() {
    return std::make_shared<PermissionHandlerLinux>();
}