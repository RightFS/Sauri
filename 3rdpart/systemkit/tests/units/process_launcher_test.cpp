/**
 * @file process_launcher_test.cpp
 * @brief Unit tests for process launcher functionality
 *
 * @author UnixCodor
 * @date 2025-03-26
 */

#include "systemkit/systemkit.hpp"

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

using namespace leigod::system_kit;

class ProcessLauncherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建进程启动器
        launcher = IComponentFactory::getInstance()->createProcessLauncher();
        permHandler = IComponentFactory::getInstance()->createPermissionHandler();

        // 设置标准测试程序路径
#ifdef _WIN32
        echoProgram = "cmd.exe";
        echoArgs = {"/c", "echo", "Hello, World!"};
        sleepProgram = "cmd.exe";
        sleepArgs = {};
        failProgram = "invalid_program_xyz.exe";
#else
        echoProgram = "/bin/echo";
        echoArgs = {"Hello, World!"};
        sleepProgram = "sleep";
        sleepArgs = {"5"};  // 5秒
        failProgram = "/usr/bin/invalid_program_xyz";
#endif

        // 检查管理员权限
        ASSERT_NO_THROW(hasAdminRights = permHandler->isRunningAsAdministrator());
    }

    std::shared_ptr<IProcessLauncher> launcher;
    std::shared_ptr<IPermissionHandler> permHandler;

    std::string echoProgram;
    std::vector<std::string> echoArgs;
    std::string sleepProgram;
    std::vector<std::string> sleepArgs;
    std::string failProgram;

    bool hasAdminRights;
};

// 测试 start 基本功能
TEST_F(ProcessLauncherTest, Start) {
    // 创建进程启动信息
    ProcessStartInfo startInfo;
    startInfo.executablePath = echoProgram;
    startInfo.arguments = echoArgs;

    // 启动进程
    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 等待进程完成
    bool succ = false;
    ASSERT_NO_THROW(succ = launcher->wait(handle, 5000));  // 5秒超时

    // 验证退出码
    EXPECT_TRUE(succ) << "Process should exit with code 0";
}

// 测试启动无效进程
TEST_F(ProcessLauncherTest, StartInvalidProcess) {
    // 创建进程启动信息
    ProcessStartInfo startInfo;
    startInfo.executablePath = failProgram;

    // 尝试启动进程
    ProcessHandle handle;
    ASSERT_THROW(handle = launcher->start(startInfo), ProcessLauncherException);
}

// 测试 terminate 功能
TEST_F(ProcessLauncherTest, Terminate) {
    // 启动长时间运行的进程
    ProcessStartInfo startInfo;
    startInfo.executablePath = sleepProgram;
    startInfo.arguments = sleepArgs;

    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 确认进程正在运行
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    bool running = false;
    ASSERT_NO_THROW(running = launcher->isRunning(handle))
        << "Failed to check if process is running";
    EXPECT_TRUE(running) << "Process should be running before termination";

    // 终止进程
    ASSERT_NO_THROW(launcher->terminate(handle)) << "Failed to terminate process";

    // 等待一段时间确保进程已终止
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 验证进程已终止
    ASSERT_NO_THROW(running = launcher->isRunning(handle))
        << "Failed to check if process is running after termination";
    EXPECT_FALSE(running) << "Process should not be running after termination";
}

// 测试 wait 功能
TEST_F(ProcessLauncherTest, Wait) {
    // 启动短时间运行的进程
    ProcessStartInfo startInfo;
    startInfo.executablePath = echoProgram;
    startInfo.arguments = echoArgs;

    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 等待进程完成 - 应该很快完成
    bool succ = false;
    ASSERT_NO_THROW(succ = launcher->wait(handle, 5000));  // 5秒超时
    EXPECT_TRUE(succ) << "Unicode process should exit with code 0";

    // 验证进程已经不在运行
    bool running = true;
    ASSERT_NO_THROW(running = launcher->isRunning(handle))
        << "Failed to check if process is running";
    EXPECT_FALSE(running) << "Process should not be running after wait";
}

// 测试 wait 超时功能
TEST_F(ProcessLauncherTest, WaitTimeout) {
    // 启动长时间运行的进程
    ProcessStartInfo startInfo;
    startInfo.executablePath = sleepProgram;
    startInfo.arguments = sleepArgs;  // 5秒

    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 等待进程，但设置较短的超时
    int succ = -1;
    ASSERT_THROW(succ = launcher->wait(handle, 500), ProcessLauncherException);  // 0.5秒

    // 验证进程仍在运行
    bool running = true;
    ASSERT_NO_THROW(running = launcher->isRunning(handle))
        << "Failed to check if process is running";
    EXPECT_TRUE(running) << "Process should be running";
    // 终止进程并清理
    ASSERT_NO_THROW(launcher->terminate(handle)) << "Failed to terminate process";
}

// 测试 isRunning 功能

TEST_F(ProcessLauncherTest, IsRunning) {
    // 启动长时间运行的进程
    ProcessStartInfo startInfo;
    startInfo.executablePath = sleepProgram;
    startInfo.arguments = sleepArgs;

    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 检查进程是否运行
    bool running = true;
    ASSERT_NO_THROW(running = launcher->isRunning(handle))
        << "Failed to check if process is running";
    EXPECT_TRUE(running) << "Process should be running";

    // 终止进程
    ASSERT_NO_THROW(launcher->terminate(handle)) << "Failed to terminate process";

    // 等待一段时间确保进程已终止
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 再次检查进程是否运行
    running = true;
    ASSERT_NO_THROW(running = launcher->isRunning(handle))
        << "Failed to check if process is running";
    EXPECT_FALSE(running) << "Process should be running";
}

// 测试 getRunningProcesses 功能

TEST_F(ProcessLauncherTest, GetRunningProcesses) {
    // 获取当前运行的进程列表
    std::vector<ProcessInfo> processes;
    ASSERT_NO_THROW(processes = launcher->getRunningProcesses())
        << "Failed to get running processes";

    // 验证有进程返回
    EXPECT_FALSE(processes.empty()) << "No running processes returned";

    // 输出一些进程信息
    std::cout << "Found " << processes.size() << " running processes" << std::endl;
    std::cout << "Sample processes:" << std::endl;

    int displayCount = 0;
    for (const auto& process : processes) {
        if (displayCount < 5) {
            std::cout << "  PID: " << process.processId << ", Name: " << process.executablePath
                      << std::endl;
            displayCount++;
        }
    }
}

// 测试中文支持

TEST_F(ProcessLauncherTest, UnicodeSupportInPaths) {
    // 创建带有中文参数的进程启动信息
    ProcessStartInfo startInfo;
#ifdef _WIN32
    startInfo.executablePath = "cmd.exe";
    startInfo.arguments = {"/c", "echo", "中文测试 - Unicode Test"};
#else
    startInfo.executable = "/bin/echo";
    startInfo.arguments = {"中文测试 - Unicode Test"};
#endif

    // 启动进程
    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 等待进程完成
    bool succ = false;
    ASSERT_NO_THROW(succ = launcher->wait(handle, 5000));  // 5秒超时
    EXPECT_TRUE(succ) << "Unicode process should exit with code 0";
}

// 测试自定义环境变量
TEST_F(ProcessLauncherTest, CustomEnvironmentVariables) {
    ProcessStartInfo startInfo;
#ifdef _WIN32
    // 创建进程启动信息
    startInfo.executablePath = "cmd.exe";
    startInfo.arguments = {"/c", "echo", "%TEST_VAR%"};
    startInfo.environmentVariables["TEST_VAR"] = "CustomValue";
#else
    // Unix版本
    startInfo.executable = "/bin/sh";
    startInfo.arguments = {"-c", "echo $TEST_VAR"};
    startInfo.environmentVariables["TEST_VAR"] = "CustomValue";
#endif

    // 启动进程
    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 等待进程完成
    bool succ = false;
    ASSERT_NO_THROW(succ = launcher->wait(handle, 5000));  // 5秒超时
    EXPECT_TRUE(succ) << "Unicode process should exit with code 0";
}

// 测试自定义工作目录

TEST_F(ProcessLauncherTest, CustomWorkingDirectory) {
    // 获取临时目录作为工作目录
    std::filesystem::path workDir = std::filesystem::temp_directory_path();

    // 创建进程启动信息
#ifdef _WIN32
    ProcessStartInfo startInfo;
    startInfo.executablePath = "cmd.exe";
    startInfo.arguments = {"/c", "echo", "%CD%"};
    startInfo.workingDirectory = workDir.string();
#else
    ProcessStartInfo startInfo;
    startInfo.executable = "/bin/pwd";
    startInfo.workingDirectory = workDir.string();
#endif

    // 启动进程
    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // 等待进程完成
    bool succ = false;
    ASSERT_NO_THROW(succ = launcher->wait(handle, 5000));  // 5秒超时
    EXPECT_TRUE(succ) << "Unicode process should exit with code 0";
}

// Test invalid process handle errors
TEST_F(ProcessLauncherTest, InvalidProcessHandleErrors) {
    // Create an invalid process handle
    ProcessHandle invalidHandle;
    invalidHandle.nativeHandle = 0;
    invalidHandle.processId = 0;

    // Try to terminate with invalid handle
    ASSERT_THROW(launcher->terminate(invalidHandle), ProcessLauncherException);

    // Try to wait with invalid handle
    ASSERT_THROW(launcher->wait(invalidHandle, 1000), ProcessLauncherException);

    // Try to check if running with invalid handle
    ASSERT_THROW(launcher->isRunning(invalidHandle), ProcessLauncherException);

    // Test with a handle that was valid but process has been terminated
    ProcessStartInfo startInfo;
    startInfo.executablePath = echoProgram;
    startInfo.arguments = echoArgs;

    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // Wait for the process to complete
    ASSERT_NO_THROW(launcher->wait(handle, 5000));

    // Now try to terminate the already exited process
    ASSERT_THROW(launcher->terminate(handle), ProcessLauncherException);
}

// Test with extremely long command line
TEST_F(ProcessLauncherTest, ExtremelyLongCommandLine) {
    ProcessStartInfo startInfo;
    startInfo.executablePath = echoProgram;

    // Generate extremely long argument (over 32KB)
    std::string longArg(33000, 'A');
    startInfo.arguments = {longArg};

    // This should throw an engine due to command line length limit
    ASSERT_THROW(launcher->start(startInfo), ProcessLauncherException);
}

// Test empty executable path
TEST_F(ProcessLauncherTest, EmptyExecutablePath) {
    ProcessStartInfo startInfo;
    startInfo.executablePath = "";

    ASSERT_THROW(launcher->start(startInfo), ProcessLauncherException);
}

// Test for permission errors (requires a process we don't have permission to access)
TEST_F(ProcessLauncherTest, AccessDeniedErrors) {
    if (!hasAdminRights) {
        // This test specifically tests behavior when not running with admin rights
        std::vector<ProcessInfo> processes;
        ASSERT_NO_THROW(processes = launcher->getRunningProcesses());

        // Look for a system process that we likely don't have permission to terminate
        ProcessHandle systemProcess;
        systemProcess.nativeHandle = 0;

        for (const auto& proc : processes) {
            std::string procName = proc.executablePath;
            std::transform(procName.begin(), procName.end(), procName.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            // Try to find a system process
            if (procName.find("system") != std::string::npos ||
                procName.find("svchost") != std::string::npos) {
                // Try to open the process to get a handle
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, proc.processId);
                if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
                    systemProcess.nativeHandle = reinterpret_cast<uintptr_t>(hProcess);
                    systemProcess.processId = proc.processId;
                    break;
                }
            }
        }

        if (systemProcess.nativeHandle != 0) {
            // Attempt to terminate a system process should fail with access denied
            ASSERT_THROW(launcher->terminate(systemProcess), ProcessLauncherException);
        } else {
            GTEST_SKIP() << "Could not find a suitable system process for permission test";
        }
    } else {
        GTEST_SKIP() << "Test requires non-admin rights";
    }
}

// Test timeout behavior
TEST_F(ProcessLauncherTest, DetailedTimeoutTest) {
    // Start a long-running process
    ProcessStartInfo startInfo;
    startInfo.executablePath = sleepProgram;
#ifdef _WIN32
    // ping with -n is more reliable for testing - pings localhost with 20 second delay
    startInfo.arguments = {"/c", "ping", "127.0.0.1", "-n", "20"};
#else
    startInfo.arguments = {"10"};  // 10 seconds
#endif

    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    // Verify process is actually running before testing timeouts
    bool running = false;
    ASSERT_NO_THROW(running = launcher->isRunning(handle));
    ASSERT_TRUE(running) << "Process should be running before timeout tests";

    // Try with very short timeout (should throw)
    ASSERT_THROW(launcher->wait(handle, 100), ProcessLauncherException);

    // Verify process is still running after the first timeout
    ASSERT_NO_THROW(running = launcher->isRunning(handle));
    ASSERT_TRUE(running) << "Process should still be running after first timeout";

    // Try with a slightly longer timeout (still should throw)
    ASSERT_THROW(launcher->wait(handle, 500), ProcessLauncherException);

    // Clean up
    ASSERT_NO_THROW(launcher->terminate(handle));
}

// Test non-existent process
TEST_F(ProcessLauncherTest, NonExistentProcessTest) {
    // Create a handle with a likely non-existent process ID
    ProcessHandle nonExistentHandle;
    nonExistentHandle.nativeHandle =
        reinterpret_cast<uintptr_t>(OpenProcess(PROCESS_TERMINATE, FALSE, 99999999));
    nonExistentHandle.processId = 99999999;

    // Operations should fail appropriately
    if (nonExistentHandle.nativeHandle != 0) {
        ASSERT_THROW(launcher->terminate(nonExistentHandle), ProcessLauncherException);
        ASSERT_FALSE(launcher->isRunning(nonExistentHandle));
    } else {
        // OpenProcess already failed, which is expected
        GTEST_SUCCEED() << "Process handle could not be opened for non-existent process (expected)";
    }
}

// Test unusual environment variables
TEST_F(ProcessLauncherTest, UnusualEnvironmentVariables) {
    ProcessStartInfo startInfo;
    startInfo.executablePath = echoProgram;

#ifdef _WIN32
    startInfo.arguments = {"/c", "echo", "%UNUSUAL_VAR_WITH_SPECIAL_CHARS%"};
    // Environment variable with special characters
    startInfo.environmentVariables["UNUSUAL_VAR_WITH_SPECIAL_CHARS"] =
        "Value with spaces, =signs, and \"quotes\"";
    // Empty environment variable
    startInfo.environmentVariables["EMPTY_VAR"] = "";
    // Very long environment variable value
    startInfo.environmentVariables["LONG_VAR"] = std::string(8000, 'X');
#else
    startInfo.arguments = {"-c", "echo $UNUSUAL_VAR_WITH_SPECIAL_CHARS"};
    startInfo.environmentVariables["UNUSUAL_VAR_WITH_SPECIAL_CHARS"] =
        "Value with spaces, =signs, and \"quotes\"";
    startInfo.environmentVariables["EMPTY_VAR"] = "";
    startInfo.environmentVariables["LONG_VAR"] = std::string(8000, 'X');
#endif

    // This should work, but we're testing edge cases with environment variables
    ProcessHandle handle;
    ASSERT_NO_THROW(handle = launcher->start(startInfo));

    bool succ = false;
    ASSERT_NO_THROW(succ = launcher->wait(handle, 5000));
    EXPECT_TRUE(succ);
}

// Test with invalid working directory
TEST_F(ProcessLauncherTest, InvalidWorkingDirectory) {
    ProcessStartInfo startInfo;
    startInfo.executablePath = echoProgram;
    startInfo.arguments = echoArgs;

    // Non-existent working directory
    startInfo.workingDirectory = "/this/path/definitely/does/not/exist/12345";

    // This should throw an engine
    ASSERT_THROW(launcher->start(startInfo), ProcessLauncherException);
}

// Test simultaneous process operations
TEST_F(ProcessLauncherTest, SimultaneousProcessOperations) {
    // Start multiple processes
    const int processCount = 5;
    std::vector<ProcessHandle> handles;

    for (int i = 0; i < processCount; i++) {
        ProcessStartInfo startInfo;
        startInfo.executablePath = sleepProgram;
#ifdef _WIN32
        startInfo.arguments = {"/c", "timeout", "/t", "3"};  // 3 second timeout
#else
        startInfo.arguments = {"3"};  // 3 seconds
#endif

        ProcessHandle handle;
        ASSERT_NO_THROW(handle = launcher->start(startInfo));
        handles.push_back(handle);
    }

    // Verify all are running
    for (const auto& handle : handles) {
        bool running = false;
        ASSERT_NO_THROW(running = launcher->isRunning(handle));
        EXPECT_TRUE(running);
    }

    // Terminate all processes
    for (const auto& handle : handles) {
        ASSERT_NO_THROW(launcher->terminate(handle));
    }

    // Verify all are terminated
    for (const auto& handle : handles) {
        bool running = true;
        ASSERT_NO_THROW(running = launcher->isRunning(handle));
        EXPECT_FALSE(running);
    }
}