/**
 * @file component_factory_impl.hpp
 * @brief Concrete implementation of the system_kit factory class.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @note This file is part of the Leigod System Kit.
 * Created: 2025-03-14
 * Author: chenxu
 */

#ifndef LEIGOD_SRC_MANAGER_INCLUDE_MANAGER_FACTORY_IMPL_HPP
#define LEIGOD_SRC_MANAGER_INCLUDE_MANAGER_FACTORY_IMPL_HPP

#include "systemkit/core/component_factory.hpp"

namespace leigod {
namespace system_kit {

class ComponentFactoryImpl : public IComponentFactory {
public:
    std::shared_ptr<IRegistryManager> createRegistryManager() const override;

    std::shared_ptr<IProcessLauncher> createProcessLauncher() const override;

    std::shared_ptr<IPermissionHandler> createPermissionHandler() const override;

    std::shared_ptr<IFirewallController> createFirewallController() const override;

    std::shared_ptr<IEnvironmentVariables> createEnvironmentVariables() const override;
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_MANAGER_INCLUDE_MANAGER_FACTORY_IMPL_HPP