/**
 * @file process_manager_mac.hpp
 * @brief macOS-specific implementation of the ProcessManager interface.
 * @details This file provides the macOS-specific implementation for starting processes using
 * posix_spawn and shell commands as a fallback using popen instead of system.
 * @date Created: 2025-03-13
 * author
 */

#ifndef LEIGOD_SRC_MANAGER_PROCESS_MANAGER_MAC_HPP
#define LEIGOD_SRC_MANAGER_PROCESS_MANAGER_MAC_HPP

#include "systemkit/process/process_launcher.hpp"

#include <string>
#include <vector>

namespace leigod {
namespace manager {

class ProcessLauncherMacOS : public IProcessLauncher {
public:
    bool startProcess(const std::string& executablePath, const std::string& args,
                      bool hideWindow) override;

private:
    bool startWithPosixSpawn(const std::string& executablePath, const std::string& args,
                             bool hideWindow);
    bool startWithShell(const std::string& executablePath, const std::string& args,
                        bool hideWindow);
};

}  // namespace manager
}  // namespace leigod

#endif  // LEIGOD_SRC_MANAGER_PROCESS_MANAGER_MAC_HPP