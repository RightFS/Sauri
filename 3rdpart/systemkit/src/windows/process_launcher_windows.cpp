/**
 * @file process_launcher_windows.cpp
 * @brief Windows-specific implementation of the IProcessLauncher interface.
 * @details This file provides the Windows-specific implementation for starting processes using
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * CreateProcess and ShellExecuteEx as a fallback.
 * @date Created: 2025-03-13
 * author
 */

#include "process_launcher_windows.hpp"

#include "common/utils/strings.h"
#include "windows_utils.hpp"

#include <TlHelp32.h>
#include <Windows.h>
#include <algorithm>
#include <map>
#include <memory>
#include <shellapi.h>
#include <string>
#include <vector>

namespace leigod {
namespace system_kit {

using namespace utils;
using namespace common;
using namespace common::utils;

/**
 * @brief RAII wrapper for Windows process handles
 */
class ProcessHandleGuard {
public:
    explicit ProcessHandleGuard(HANDLE handle) : m_handle(handle) {}

    ~ProcessHandleGuard() {
        if (m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr) {
            CloseHandle(m_handle);
        }
    }

    HANDLE get() const {
        return m_handle;
    }

    // Prevent copying
    ProcessHandleGuard(const ProcessHandleGuard&) = delete;
    ProcessHandleGuard& operator=(const ProcessHandleGuard&) = delete;

private:
    HANDLE m_handle;
};

/**
 * @brief Build command line string from executable and arguments
 * @param executablePath Path to executable
 * @param arguments Vector of command line arguments
 * @return Command line string
 */
std::wstring buildCommandLine(const std::string& executablePath,
                              const std::vector<std::string>& arguments) {
    std::wstring commandLine = L"\"" + utf8ToWide(executablePath) + L"\"";

    for (const auto& arg : arguments) {
        commandLine += L" " + utf8ToWide(arg) + L"";
    }

    return commandLine;
}

/**
 * @brief Create environment block from environment variables map
 * @param environmentVariables Map of environment variables
 * @return Environment block as wide string
 */
std::wstring
createEnvironmentBlock(const std::map<std::string, std::string>& environmentVariables) {
    std::wstring result;

    // If no custom environment variables, return empty string
    if (environmentVariables.empty()) {
        return result;
    }

    // Get current environment variables
    LPWCH currentEnv = GetEnvironmentStringsW();
    if (!currentEnv) {
        throw ProcessLauncherException(
            ErrorCode::ProcessError, "Failed to get environment strings: " + getLastErrorMessage());
    }

    // Copy existing environment variables
    std::map<std::wstring, std::wstring> envMap;
    LPWCH envStr = currentEnv;
    while (*envStr) {
        std::wstring entry(envStr);
        size_t equalsPos = entry.find(L'=');

        if (equalsPos != std::wstring::npos && equalsPos > 0) {
            std::wstring name = entry.substr(0, equalsPos);
            std::wstring value = entry.substr(equalsPos + 1);
            envMap[name] = value;
        }

        envStr += entry.length() + 1;
    }
    FreeEnvironmentStringsW(currentEnv);

    // Override with provided environment variables
    for (const auto& pair : environmentVariables) {
        envMap[utf8ToWide(pair.first)] = utf8ToWide(pair.second);
    }

    // Build environment block
    for (const auto& pair : envMap) {
        result += pair.first + L"=" + pair.second + L'\0';
    }

    // Add final null terminator
    result += L'\0';
    return result;
}

/**
 * @brief Convert Windows process priority to our enum
 * @param priority Native Windows priority
 * @return ProcessPriority value
 */
ProcessPriority windowsToPriorityEnum(DWORD priority) {
    switch (priority) {
        case IDLE_PRIORITY_CLASS:
            return ProcessPriority::Idle;
        case BELOW_NORMAL_PRIORITY_CLASS:
            return ProcessPriority::BelowNormal;
        case NORMAL_PRIORITY_CLASS:
            return ProcessPriority::Normal;
        case ABOVE_NORMAL_PRIORITY_CLASS:
            return ProcessPriority::AboveNormal;
        case HIGH_PRIORITY_CLASS:
            return ProcessPriority::High;
        case REALTIME_PRIORITY_CLASS:
            return ProcessPriority::Realtime;
        default:
            return ProcessPriority::Normal;
    }
}

/**
 * @brief Convert our enum to Windows process priority
 * @param priority ProcessPriority value
 * @return Native Windows priority
 */
DWORD priorityEnumToWindows(ProcessPriority priority) {
    switch (priority) {
        case ProcessPriority::Idle:
            return IDLE_PRIORITY_CLASS;
        case ProcessPriority::BelowNormal:
            return BELOW_NORMAL_PRIORITY_CLASS;
        case ProcessPriority::Normal:
            return NORMAL_PRIORITY_CLASS;
        case ProcessPriority::AboveNormal:
            return ABOVE_NORMAL_PRIORITY_CLASS;
        case ProcessPriority::High:
            return HIGH_PRIORITY_CLASS;
        case ProcessPriority::Realtime:
            return REALTIME_PRIORITY_CLASS;
        default:
            return NORMAL_PRIORITY_CLASS;
    }
}

/**
 * @brief Get process information from a handle
 * @param handle Process handle
 * @return ProcessInfo structure with process details
 */
Result<ProcessInfo> getProcessInfo(HANDLE processHandle) {
    ProcessInfo info;

    // Get process ID
    info.processId = GetProcessId(processHandle);
    if (info.processId == 0) {
        return Result<ProcessInfo>::failure(ErrorCode::ProcessOperationFailed,
                                            "Failed to get process ID: " + getLastErrorMessage());
    }

    // Get process name
    WCHAR processName[MAX_PATH];
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(processHandle, 0, processName, &size)) {
        info.executablePath = wideToUtf8(processName);
    } else {
        // Log the error but continue - partial information is still useful
        throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                       "Failed to get process name: " + getLastErrorMessage());
    }

    // Get process priority
    DWORD priorityClass = GetPriorityClass(processHandle);
    if (priorityClass != 0) {
        info.priority = windowsToPriorityEnum(priorityClass);
    } else {
        // Log error but continue
        throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                       "Failed to get process priority: " + getLastErrorMessage());
    }

    return Result<ProcessInfo>::success(info);
}

// Implementation of ProcessLauncherWindows
ProcessHandle ProcessLauncherWindows::start(const ProcessStartInfo& startInfo) {
    try {
        // Validate input
        if (startInfo.executablePath.empty()) {
            throw ProcessLauncherException(ErrorCode::InvalidArgument,
                                           "Executable path cannot be empty");
        }

        // Use the appropriate launch method based on startInfo.launchType
        if (startInfo.launchType == LaunchType::ShellExecutes) {
            return startWithShellExecute(startInfo);
        } else {
            return startWithCreateProcess(startInfo);
        }
    } catch (const ProcessLauncherException&) {
        throw;
    } catch (const std::exception& ex) {
        throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                       "Exception in start: " + std::string(ex.what()));
    }
}

ProcessHandle ProcessLauncherWindows::startWithCreateProcess(const ProcessStartInfo& startInfo) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Configure window install_options
    if (startInfo.createNoWindow) {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }

    // Build command line
    std::wstring commandLine = buildCommandLine(startInfo.executablePath, startInfo.arguments);

    // Handle working directory
    std::wstring workingDir;
    LPWSTR workingDirPtr = nullptr;
    if (!startInfo.workingDirectory.empty()) {
        workingDir = utf8ToWide(startInfo.workingDirectory);
        workingDirPtr = const_cast<LPWSTR>(workingDir.c_str());
    }

    // Handle environment variables
    std::wstring envBlock;
    LPVOID envBlockPtr = nullptr;
    if (!startInfo.environmentVariables.empty()) {
        envBlock = createEnvironmentBlock(startInfo.environmentVariables);
        envBlockPtr = const_cast<LPVOID>(static_cast<LPCVOID>(envBlock.c_str()));
    }

    // Creation flags
    DWORD creationFlags = 0;
    if (startInfo.createNoWindow) {
        creationFlags |= CREATE_NO_WINDOW;
    }

    // Add priority flags
    creationFlags |= priorityEnumToWindows(startInfo.priority);

    creationFlags |= CREATE_UNICODE_ENVIRONMENT;

    // Use a non-const buffer for the command line
    if (commandLine.length() >= 32767) {  // Windows command line length limit
        throw ProcessLauncherException(
            ErrorCode::InvalidArgument,
            "Command line too long: " + std::to_string(commandLine.length()) +
                " characters (maximum allowed is 32766)");
    }
    std::vector<wchar_t> cmdLineBuf(commandLine.length() + 1);
    wcscpy_s(cmdLineBuf.data(), cmdLineBuf.size(), commandLine.c_str());

    // Check if admin privileges are required
    if (startInfo.reqAdmin) {
        SHELLEXECUTEINFOW sei = {0};
        sei.cbSize = sizeof(SHELLEXECUTEINFOW);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"runas";  // Use "runas" to request admin privileges
        sei.lpFile = utf8ToWide(startInfo.executablePath).c_str();
        sei.lpParameters = commandLine.c_str();
        sei.lpDirectory = workingDirPtr;
        sei.nShow = startInfo.showCmd;

        if (!ShellExecuteExW(&sei)) {
            throw ProcessLauncherException(ErrorCode::ProcessStartFailed,
                                           "Failed to start process with admin privileges: " +
                                               getLastErrorMessage());
        }

        ProcessHandle handle;
        handle.nativeHandle = reinterpret_cast<uintptr_t>(sei.hProcess);
        handle.processId = GetProcessId(sei.hProcess);
        return handle;
    }

    // Create the process
    BOOL result = CreateProcessW(nullptr,            // No application name (use command line)
                                 cmdLineBuf.data(),  // Command line
                                 nullptr,            // Process handle not inheritable
                                 nullptr,            // Thread handle not inheritable
                                 FALSE,              // Don't inherit handles
                                 creationFlags,      // Creation flags
                                 envBlockPtr,        // Use provided environment block or parent's
                                 workingDirPtr,      // Use provided working directory or parent's
                                 &si,                // Startup info
                                 &pi                 // Process information
    );

    if (!result) {
        throw ProcessLauncherException(ErrorCode::ProcessStartFailed,
                                       "Failed to start process with CreateProcess: " +
                                           getLastErrorMessage());
    }

    // Close thread handle as we don't need it
    CloseHandle(pi.hThread);

    // Create our process handle
    ProcessHandle handle;
    handle.nativeHandle = reinterpret_cast<uintptr_t>(pi.hProcess);
    handle.processId = pi.dwProcessId;

    return handle;
}

ProcessHandle ProcessLauncherWindows::startWithShellExecute(const ProcessStartInfo& startInfo) {
    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFOW);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;  // We want the process handle

    // Set verb if provided, otherwise use "open"
    std::wstring verb = startInfo.verb.empty() ? L"open" : utf8ToWide(startInfo.verb);
    sei.lpVerb = verb.c_str();

    // Set the file to execute
    std::wstring file = utf8ToWide(startInfo.executablePath);
    sei.lpFile = file.c_str();

    // Set working directory if provided
    std::wstring workingDir;
    if (!startInfo.workingDirectory.empty()) {
        workingDir = utf8ToWide(startInfo.workingDirectory);
        sei.lpDirectory = workingDir.c_str();
    }

    // Set parameters (command line arguments)
    std::wstring parameters;
    if (!startInfo.arguments.empty()) {
        for (const auto& arg : startInfo.arguments) {
            if (!parameters.empty())
                parameters += L" ";
            parameters += L"\"" + utf8ToWide(arg) + L"\"";
        }
        sei.lpParameters = parameters.c_str();
    }

    // Set window show state
    sei.nShow = startInfo.showCmd;

    if (startInfo.reqAdmin) {
        sei.lpVerb = L"runas";  // Use "runas" to request admin privileges
    }

    // Execute the file
    if (!ShellExecuteExW(&sei)) {
        throw ProcessLauncherException(ErrorCode::ProcessStartFailed,
                                       "Failed to start process with ShellExecuteEx: " +
                                           getLastErrorMessage());
    }

    // Check if we got a process handle
    if (sei.hProcess == nullptr) {
        throw ProcessLauncherException(ErrorCode::ProcessStartFailed,
                                       "ShellExecuteEx didn't return a process handle: " +
                                           getLastErrorMessage());
    }

    // Create our process handle
    ProcessHandle handle;
    handle.nativeHandle = reinterpret_cast<uintptr_t>(sei.hProcess);
    handle.processId = GetProcessId(sei.hProcess);

    return handle;
}

static bool isValidHandle(const ProcessHandle& handle) {
    return handle.nativeHandle != 0 &&
           handle.nativeHandle != reinterpret_cast<uintptr_t>(INVALID_HANDLE_VALUE);
}

void ProcessLauncherWindows::terminate(ProcessHandle handle) {
    try {
        if (!isValidHandle(handle)) {
            throw ProcessLauncherException(ErrorCode::InvalidArgument, "Invalid process handle");
        }

        auto processHandle = reinterpret_cast<HANDLE>(handle.nativeHandle);
        ProcessHandleGuard handleGuard(processHandle);

        // Attempt to terminate the process
        if (!TerminateProcess(processHandle, 1)) {
            DWORD error = GetLastError();
            if (error == ERROR_ACCESS_DENIED) {
                throw ProcessLauncherException(ErrorCode::ProcessAccessDenied,
                                               "Access denied when terminating process: " +
                                                   getLastErrorMessage());
            } else if (error == ERROR_INVALID_HANDLE) {
                throw ProcessLauncherException(ErrorCode::ProcessNotFound,
                                               "Process not found or already terminated");
            } else {
                throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                               "Failed to terminate process: " +
                                                   getLastErrorMessage());
            }
        }
    } catch (const ProcessLauncherException&) {
        throw;
    } catch (const std::exception& ex) {
        throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                       "Exception in terminate: " + std::string(ex.what()));
    }
}

bool ProcessLauncherWindows::wait(ProcessHandle handle, int timeoutMs) {
    try {
        if (handle.nativeHandle == 0) {
            throw ProcessLauncherException(ErrorCode::InvalidArgument, "Invalid process handle");
        }

        auto processHandle = reinterpret_cast<HANDLE>(handle.nativeHandle);

        // First, check if the process has already exited
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(processHandle, &exitCode)) {
            throw ProcessLauncherException(ErrorCode::SystemError,
                                           "Failed to get process exit code: " +
                                               getLastErrorMessage());
        }

        // If the process has exited, STILL_ACTIVE (259) is a special value indicating the process
        // is still running
        if (exitCode != STILL_ACTIVE) {
            // Process has exited, return the exit code directly
            return static_cast<int>(exitCode);
        }

        // The process is still running, waiting for it to exit
        DWORD waitResult =
            WaitForSingleObject(processHandle, (timeoutMs < 0) ? INFINITE : timeoutMs);

        if (waitResult == WAIT_TIMEOUT) {
            throw ProcessLauncherException(ErrorCode::Timeout, "Process wait timed out after " +
                                                                   std::to_string(timeoutMs) +
                                                                   " ms");
        } else if (waitResult != WAIT_OBJECT_0) {
            throw ProcessLauncherException(ErrorCode::SystemError,
                                           "Failed to wait for process: " + getLastErrorMessage());
        }

        // Process has exited, get the exit code
        if (!GetExitCodeProcess(processHandle, &exitCode)) {
            throw ProcessLauncherException(ErrorCode::SystemError,
                                           "Failed to get process exit code after wait: " +
                                               getLastErrorMessage());
        }

        return exitCode == 0;
    } catch (const ProcessLauncherException&) {
        throw;
    } catch (const std::exception& ex) {
        throw ProcessLauncherException(ErrorCode::SystemError,
                                       "Exception in wait: " + std::string(ex.what()));
    }
}

bool ProcessLauncherWindows::isRunning(ProcessHandle handle) const {
    try {
        if (!isValidHandle(handle)) {
            throw ProcessLauncherException(ErrorCode::InvalidArgument, "Invalid process handle");
        }

        auto processHandle = reinterpret_cast<HANDLE>(handle.nativeHandle);

        // Get exit code
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(processHandle, &exitCode)) {
            DWORD error = GetLastError();
            if (error == ERROR_INVALID_HANDLE) {
                // Handle is invalid, process is not running
                return false;
            }
            throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                           "Failed to get process exit code: " +
                                               getLastErrorMessage());
        }

        // Additional check - try to wait with zero timeout to confirm process state
        if (exitCode == STILL_ACTIVE) {
            DWORD waitResult = WaitForSingleObject(processHandle, 0);
            if (waitResult == WAIT_OBJECT_0) {
                // Process has actually terminated
                // Update the exit code
                if (!GetExitCodeProcess(processHandle, &exitCode)) {
                    throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                                   "Failed to get updated process exit code: " +
                                                       getLastErrorMessage());
                }
            }
        }

        return exitCode == STILL_ACTIVE;
    } catch (const ProcessLauncherException&) {
        throw;
    } catch (const std::exception& ex) {
        throw ProcessLauncherException(ErrorCode::SystemError,
                                       "Exception in isRunning: " + std::string(ex.what()));
    }
}

std::vector<ProcessInfo> ProcessLauncherWindows::getRunningProcesses() const {
    try {
        std::vector<ProcessInfo> processes;

        // Create snapshot of processes
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                           "Failed to create process snapshot: " +
                                               getLastErrorMessage());
        }

        ProcessHandleGuard snapHandleGuard(hProcessSnap);

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        // Get first process
        if (!Process32FirstW(hProcessSnap, &pe32)) {
            throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                           "Failed to get first process: " + getLastErrorMessage());
        }

        // Iterate through all processes
        do {
            ProcessInfo info;
            info.processId = pe32.th32ProcessID;
            info.executablePath = wideToUtf8(pe32.szExeFile);
            info.priority = ProcessPriority::Normal;  // Default value

            // Try to get more detailed information by opening the process
            HANDLE hProcess =
                OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess != nullptr && hProcess != INVALID_HANDLE_VALUE) {
                ProcessHandleGuard processHandleGuard(hProcess);

                // Get full path
                WCHAR filePath[MAX_PATH];
                DWORD size = MAX_PATH;
                if (QueryFullProcessImageNameW(hProcess, 0, filePath, &size)) {
                    info.executablePath = wideToUtf8(filePath);
                }

                // Get priority class
                DWORD priorityClass = GetPriorityClass(hProcess);
                if (priorityClass != 0) {
                    info.priority = windowsToPriorityEnum(priorityClass);
                }
            }

            processes.push_back(info);

        } while (Process32NextW(hProcessSnap, &pe32));

        return processes;
    } catch (const ProcessLauncherException&) {
        throw;
    } catch (const std::exception& ex) {
        throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                       "Exception in getRunningProcesses: " +
                                           std::string(ex.what()));
    }
}

std::vector<ProcessHandle>
ProcessLauncherWindows::getProcessByPath(const std::filesystem::path& path) const {
    try {
        std::vector<ProcessHandle> processes;

        // Create snapshot of processes
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                           "Failed to create process snapshot: " +
                                               getLastErrorMessage());
        }

        ProcessHandleGuard snapHandleGuard(hProcessSnap);

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        // Get first process
        if (!Process32FirstW(hProcessSnap, &pe32)) {
            throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                           "Failed to get first process: " + getLastErrorMessage());
        }

        // 获取路径的标准形式用于比较
        std::wstring targetPath = path.wstring();
        std::transform(targetPath.begin(), targetPath.end(), targetPath.begin(), ::towlower);

        // Iterate through all processes
        do {
            // 先检查进程名是否匹配
            std::wstring processName(pe32.szExeFile);
            std::transform(processName.begin(), processName.end(), processName.begin(), ::towlower);

            if (targetPath.find(processName) != std::wstring::npos) {
                // 尝试打开进程获取句柄
                HANDLE hProcess =
                    OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
                                FALSE, pe32.th32ProcessID);
                if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
                    // 获取更精确的进程路径进行比较
                    WCHAR filePath[MAX_PATH];
                    DWORD size = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProcess, 0, filePath, &size)) {
                        std::wstring fullPath(filePath);
                        std::transform(fullPath.begin(), fullPath.end(), fullPath.begin(),
                                       ::towlower);

                        if (fullPath.find(targetPath) != std::wstring::npos) {
                            ProcessHandle info;
                            info.processId = pe32.th32ProcessID;
                            // 将进程句柄转换为 uintptr_t 类型
                            info.nativeHandle = reinterpret_cast<uintptr_t>(hProcess);
                            processes.push_back(info);
                            continue;  // 不要关闭句柄，因为它将被返回
                        }
                    }
                    CloseHandle(hProcess);  // 如果不匹配，关闭句柄
                }
            }
        } while (Process32NextW(hProcessSnap, &pe32));

        return processes;
    } catch (const ProcessLauncherException&) {
        throw;
    } catch (const std::exception& ex) {
        throw ProcessLauncherException(ErrorCode::ProcessOperationFailed,
                                       "Exception in getProcessByPath: " + std::string(ex.what()));
    }
}

std::shared_ptr<IProcessLauncher> createProcessLauncher() {
    return std::make_shared<ProcessLauncherWindows>();
}

}  // namespace system_kit
}  // namespace leigod