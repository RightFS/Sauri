/**
 * @file process_launcher_windows.hpp
 * @brief Windows-specific implementation of the IProcessLauncher interface.
 * @details This file provides the Windows-specific implementation for starting processes using
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * CreateProcess and ShellExecuteEx as a fallback.
 * @date Created: 2025-03-13
 * author
 */

#ifndef LEIGOD_SRC_WINDOWS_PROCESS_MANAGER_WINDOWS_HPP
#define LEIGOD_SRC_WINDOWS_PROCESS_MANAGER_WINDOWS_HPP

#include "systemkit/core/result.hpp"
#include "systemkit/process/process_launcher.hpp"

#include <windows.h>

namespace leigod {
namespace system_kit {

/**
 * @class ProcessLauncherWindows
 * @brief Windows platform implementation of process management
 *
 * This class handles process creation, termination, monitoring and control
 * using Win32 API on Windows platforms. Supports both CreateProcess and
 * ShellExecuteEx methods for process creation.
 */
class ProcessLauncherWindows : public IProcessLauncher {
public:
    /**
     * @brief Constructor
     */
    ProcessLauncherWindows() = default;

    /**
     * @brief Destructor
     */
    ~ProcessLauncherWindows() override = default;

    /**
     * @brief Start a new process using either CreateProcess or ShellExecuteEx
     *
     * @param startInfo Information about the process to start
     * @return ProcessHandle Handle to the started process if successful
     * @throw ProcessLauncherException
     */
    ProcessHandle start(const ProcessStartInfo& startInfo) override;

    /**
     * @brief Terminate a running process
     *
     * @param handle Handle to the process to terminate
     * @return void Success if the process was terminated
     * @throw ProcessLauncherException
     */
    void terminate(ProcessHandle handle) override;

    /**
     * @brief Wait for a process to exit
     *
     * @param handle Handle to the process to wait for
     * @param timeoutMs Maximum time to wait in milliseconds, -1 for infinite
     * @return bool True if the process exited, false if timed out
     * @throw ProcessLauncherException
     */
    bool wait(ProcessHandle handle, int timeoutMs = -1) override;

    /**
     * @brief Check if a process is running
     *
     * @param handle Handle to the process to check
     * @return bool True if the process is running, false otherwise
     * @throw ProcessLauncherException
     */
    bool isRunning(ProcessHandle handle) const override;

    /**
     * @brief Get a list of running processes
     *
     * @return std::vector<ProcessInfo> List of running processes if successful
     * @throw ProcessLauncherException
     */
    std::vector<ProcessInfo> getRunningProcesses() const override;

    /**
     * @brief Get a list of processes by executable path
     *
     * @param path Path to the executable
     * @return std::vector<ProcessHandle> List of processes matching the path
     * @throw ProcessLauncherException
     */
    std::vector<ProcessHandle> getProcessByPath(const std::filesystem::path& path) const override;

private:
    /**
     * @brief Create a process using CreateProcess API
     *
     * @param startInfo Information about the process to start
     * @return ProcessHandle Handle to the started process if successful
     * @throw ProcessLauncherException
     */
    ProcessHandle startWithCreateProcess(const ProcessStartInfo& startInfo);

    /**
     * @brief Create a process using ShellExecuteEx API
     *
     * @param startInfo Information about the process to start
     * @return ProcessHandle Handle to the started process if successful
     * @throw ProcessLauncherException
     */
    ProcessHandle startWithShellExecute(const ProcessStartInfo& startInfo);
};
}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_SRC_WINDOWS_PROCESS_MANAGER_WINDOWS_HPP