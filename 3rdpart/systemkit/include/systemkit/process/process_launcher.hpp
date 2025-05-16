/**
 * @file process_launcher.hpp
 * @brief Interface for the cross-platform process system_kit.
 * @details This file declares the IProcessLauncher interface for starting processes in a
 * cross-platform manner. Implementations for different platforms should use this interface.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * @date Created: 2025-03-13
 * @author
 * @copyright
 */

#ifndef LEIGOD_INCLUDE_SYSTEMKIT_PROCESS_LAUNCHER_HPP
#define LEIGOD_INCLUDE_SYSTEMKIT_PROCESS_LAUNCHER_HPP

#include "systemkit/exceptions/exceptions.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace leigod {
namespace system_kit {
/**
 * @brief Process priority level
 */
enum class ProcessPriority {
    Idle,         ///< Lowest priority
    BelowNormal,  ///< Below normal priority
    Normal,       ///< Normal priority (default)
    AboveNormal,  ///< Above normal priority
    High,         ///< High priority
    Realtime      ///< Highest priority (use with caution)
};

/**
 * @brief Process launch operation type
 */
enum class LaunchType {
    CreateProcess,  ///< Use CreateProcess (standard process creation)
    ShellExecutes   ///< Use ShellExecuteEx (shell-integrated launch with verb support)
};

/**
 * @brief Handle to a process
 */
struct ProcessHandle {
    uint64_t nativeHandle = 0;  ///< Native handle for the platform (HANDLE on Windows)
    uint32_t processId = 0;     ///< Process identifier
};

/**
 * @brief Information about a running process
 */
struct ProcessInfo {
    uint32_t processId = 0;                              ///< Process identifier
    std::string executablePath;                          ///< Path to the executable
    ProcessPriority priority = ProcessPriority::Normal;  ///< Process priority
};

/**
 * @brief Information needed to start a process
 */
struct ProcessStartInfo {
    std::string executablePath;          ///< Path to the executable
    std::vector<std::string> arguments;  ///< Command line arguments
    std::string workingDirectory;        ///< Working directory for the process (empty = inherit)
    std::map<std::string, std::string> environmentVariables;  ///< Environment variables to set
    bool createNoWindow = false;  ///< True to create the process without a window
    ProcessPriority priority = ProcessPriority::Normal;  ///< Process priority
    LaunchType launchType = LaunchType::CreateProcess;   ///< Process creation method
    std::string verb;       ///< Verb for ShellExecute (e.g., "open", "runas", "print")
    int showCmd = 1;        ///< Show window command (SW_SHOWNORMAL = 1)
    bool reqAdmin = false;  ///< True to request admin privileges
};

/**
 * @class IProcessLauncher
 * @brief Interface for process management operations
 *
 * This interface provides methods to launch, terminate and monitor processes.
 */
class IProcessLauncher {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IProcessLauncher() = default;

    /**
     * @brief Start a new process
     *
     * @param startInfo Information about the process to start
     * @return ProcessHandle Handle to the started process if successful
     * @throw ProcessLauncherException
     */
    virtual ProcessHandle start(const ProcessStartInfo& startInfo) = 0;

    /**
     * @brief Terminate a running process
     *
     * @param handle Handle to the process to terminate
     * @return void Success if the process was terminated
     * @throw ProcessLauncherException
     */
    virtual void terminate(ProcessHandle handle) = 0;

    /**
     * @brief Wait for a process to exit
     *
     * @param handle Handle to the process to wait for
     * @param timeoutMs Maximum time to wait in milliseconds, -1 for infinite
     * @return bool True if the process exited, false if timed out
     * @throw ProcessLauncherException
     */
    virtual bool wait(ProcessHandle handle, int timeoutMs) = 0;

    /**
     * @brief Check if a process is running
     *
     * @param handle Handle to the process to check
     * @return bool True if the process is running, false otherwise
     * @throw ProcessLauncherException
     */
    virtual bool isRunning(ProcessHandle handle) const = 0;

    /**
     * @brief Get a list of running processes
     *
     * @return std::vector<ProcessInfo> List of running processes if successful
     * @throw ProcessLauncherException
     */
    virtual std::vector<ProcessInfo> getRunningProcesses() const = 0;

    /**
     * @brief Get a list of processes by executable path
     *
     * @param path Path to the executable
     * @return std::vector<ProcessHandle> List of processes matching the path
     * @throw ProcessLauncherException
     */
    virtual std::vector<ProcessHandle>
    getProcessByPath(const std::filesystem::path& path) const = 0;
};

}  // namespace system_kit
}  // namespace leigod

#endif  // LEIGOD_INCLUDE_SYSTEMKIT_PROCESS_LAUNCHER_HPP