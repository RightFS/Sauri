/**
 * @file systemkit.hpp
 * @brief Main include file for the SystemKit library
 *
 * This header includes all components of the SystemKit library,
 * providing a single point of inclusion for users of the library.
 *
 * @author UnixCodor
 * @date 2025-03-26
 */

#ifndef SYSTEMKIT_HPP
#define SYSTEMKIT_HPP

// Library version information is defined by CMake
#include "systemkit/version.hpp"

// Core components
#include "core/component_factory.hpp"

// Feature modules
#include "environment/environment_variables.hpp"
#include "firewall/firewall_controller.hpp"
#include "permissions/permission_handler.hpp"
#include "process/process_launcher.hpp"
#include "registry/registry_manager.hpp"

// Exceptions
#include "exceptions/exceptions.hpp"

/**
 * @namespace systemkit
 * @brief Main namespace for the SystemKit library
 *
 * Contains all classes, functions, and sub-namespaces that make up the
 * SystemKit library for operating system interface abstraction.
 */
namespace system_kit {

/**
 * @brief Returns the version of the SystemKit library as a string
 * @return String representation of the library version (e.g. "1.0.0")
 */
inline std::string version() {
    return std::to_string(SYSTEMKIT_VERSION_MAJOR) + "." + std::to_string(SYSTEMKIT_VERSION_MINOR) +
           "." + std::to_string(SYSTEMKIT_VERSION_PATCH);
}

}  // namespace system_kit

#endif  // SYSTEMKIT_HPP