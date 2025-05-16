/**
 * @file component_factory.hpp
 * @brief Abstract factory class for creating various managers.
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-14
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_MANAGER_INCLUDE_MANAGER_FACTORY_HPP
#define LEIGOD_SRC_MANAGER_INCLUDE_MANAGER_FACTORY_HPP

#include "systemkit/environment/environment_variables.hpp"
#include "systemkit/firewall/firewall_controller.hpp"
#include "systemkit/permissions/permission_handler.hpp"
#include "systemkit/process/process_launcher.hpp"
#include "systemkit/registry/registry_manager.hpp"

#include <memory>

namespace leigod {
namespace system_kit {

class IComponentFactory {
public:
    virtual ~IComponentFactory() = default;

    virtual std::shared_ptr<IRegistryManager> createRegistryManager() const = 0;
    virtual std::shared_ptr<IProcessLauncher> createProcessLauncher() const = 0;
    virtual std::shared_ptr<IPermissionHandler> createPermissionHandler() const = 0;
    virtual std::shared_ptr<IFirewallController> createFirewallController() const = 0;
    virtual std::shared_ptr<IEnvironmentVariables> createEnvironmentVariables() const = 0;

    static std::shared_ptr<IComponentFactory> getInstance();
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_MANAGER_INCLUDE_MANAGER_FACTORY_HPP