/**
 * @file permission_manager_macos.hpp
 * @brief MacOS-specific implementation of the PermissionManager.
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-13
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_MANAGER_PERMISSION_MANAGER_MACOS_HPP
#define LEIGOD_SRC_MANAGER_PERMISSION_MANAGER_MACOS_HPP

#include "systemkit/permissions/permission_handler.hpp"

namespace leigod {
namespace manager {
/**
 * @class PermissionHandlerMacOS
 * @brief MacOS-specific implementation of the PermissionManager.
 */
class PermissionHandlerMacOS : public IPermissionHandler {
public:
    bool checkPermission() override;

    bool requestPermission() override;

    bool performOperation(const std::string& operation) override;
};

}  // namespace manager
}  // namespace leigod

#endif  // LEIGOD_SRC_MANAGER_PERMISSION_MANAGER_MACOS_HPP