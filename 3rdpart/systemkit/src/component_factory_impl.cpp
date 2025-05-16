/**
 * @file component_factory_impl.hpp
 * @brief Implementation of the system_kit factory class.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-14
 * Author: chenxu
 */

#include "component_factory_impl.hpp"

#include "systemkit/environment/environment_variables.hpp"
#include "systemkit/firewall/firewall_controller.hpp"
#include "systemkit/permissions/permission_handler.hpp"
#include "systemkit/process/process_launcher.hpp"
#include "systemkit/registry/registry_manager.hpp"

namespace leigod {
namespace system_kit {

extern std::shared_ptr<IEnvironmentVariables> createEnvironmentVariables();
extern std::shared_ptr<IFirewallController> createFirewallController();
extern std::shared_ptr<IPermissionHandler> createPermissionHandler();
extern std::shared_ptr<IProcessLauncher> createProcessLauncher();
extern std::shared_ptr<IRegistryManager> createRegistryManager();

std::shared_ptr<IRegistryManager> ComponentFactoryImpl::createRegistryManager() const {
    return leigod::system_kit::createRegistryManager();
}

std::shared_ptr<IProcessLauncher> ComponentFactoryImpl::createProcessLauncher() const {
    return leigod::system_kit::createProcessLauncher();
}

std::shared_ptr<IPermissionHandler> ComponentFactoryImpl::createPermissionHandler() const {
    return leigod::system_kit::createPermissionHandler();
}

std::shared_ptr<IFirewallController> ComponentFactoryImpl::createFirewallController() const {
    return leigod::system_kit::createFirewallController();
}

std::shared_ptr<IEnvironmentVariables> ComponentFactoryImpl::createEnvironmentVariables() const {
    return leigod::system_kit::createEnvironmentVariables();
}

std::shared_ptr<IComponentFactory> IComponentFactory::getInstance() {
    static auto instance = std::make_shared<ComponentFactoryImpl>();
    return instance;
}

}  // namespace system_kit
}  // namespace leigod