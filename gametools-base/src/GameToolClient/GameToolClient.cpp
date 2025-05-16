//
// Created by Right on 25/5/14 星期三 16:51.
//

#include "GameToolClient.h"
#include <iostream>

GameToolClient::GameToolClient()
        : mainPipeHandle_(INVALID_HANDLE_VALUE),
          serverPipeHandle_(INVALID_HANDLE_VALUE),
          running_(false) {
}

GameToolClient::~GameToolClient() {
    // 注销应用
    unregisterApp();

    // 停止管道服务器
    running_ = false;

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    if (mainPipeThread_.joinable()) {
        mainPipeThread_.join();
    }

    // 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (HANDLE handle : clients_) {
            CloseHandle(handle);
        }
        clients_.clear();
    }

    // 关闭管道句柄
    if (serverPipeHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(serverPipeHandle_);
        serverPipeHandle_ = INVALID_HANDLE_VALUE;
    }

    if (mainPipeHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(mainPipeHandle_);
        mainPipeHandle_ = INVALID_HANDLE_VALUE;
    }
}

bool GameToolClient::connectToElectron() {
    // 连接到Electron的主管道
    std::string mainPipeName = "\\\\.\\pipe\\electron_main_pipe";

    // 等待管道可用
    if (WaitNamedPipe(mainPipeName.c_str(), 5000)) {
        mainPipeHandle_ = CreateFile(
                mainPipeName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
        );

        if (mainPipeHandle_ != INVALID_HANDLE_VALUE) {
            // 启动读取线程
            mainPipeThread_ = std::thread(&GameToolClient::mainPipeReadThread, this);
            return true;
        }
    }

    std::cerr << "连接到Electron失败，请确保Electron应用已启动。" << std::endl;
    return false;
}

bool GameToolClient::registerApp(
        const std::string& appId,
        const std::string& name,
        const std::string& description,
        const std::string& iconPath,
        const std::string& pipeName,
        const std::string& httpUrl,
        const std::string& localPath
) {
    // 保存应用信息
    appId_ = appId;
    name_ = name;
    appPipeName_ = pipeName;

    // 连接到Electron
    if (!connectToElectron()) {
        return false;
    }

    // 创建注册消息
    json registerMsg = {
            {"command", "register"},
            {"appId", appId},
            {"appInfo", {
                                {"name", name},
                                {"description", description},
                                {"iconPath", iconPath},
                                {"pipeName", pipeName}
                        }}
    };

    // 添加可选参数
    if (!httpUrl.empty()) {
        registerMsg["appInfo"]["httpUrl"] = httpUrl;
    }

    if (!localPath.empty()) {
        registerMsg["appInfo"]["localPath"] = localPath;
    }

    // 发送注册消息
    std::string message = registerMsg.dump() + "\n";
    DWORD bytesWritten;

    if (!WriteFile(
            mainPipeHandle_,
            message.c_str(),
            static_cast<DWORD>(message.size()),
            &bytesWritten,
            NULL
    )) {
        std::cerr << "发送注册消息失败: " << GetLastError() << std::endl;
        return false;
    }

    // 注册成功，启动应用管道服务器
    return startPipeServer();
}

bool GameToolClient::unregisterApp() {
    if (mainPipeHandle_ == INVALID_HANDLE_VALUE || appId_.empty()) {
        return false;
    }

    // 创建注销消息
    json unregisterMsg = {
            {"command", "unregister"},
            {"appId", appId_}
    };

    // 发送注销消息
    std::string message = unregisterMsg.dump() + "\n";
    DWORD bytesWritten;

    bool result = WriteFile(
            mainPipeHandle_,
            message.c_str(),
            static_cast<DWORD>(message.size()),
            &bytesWritten,
            NULL
    ) != 0;

    // 关闭主管道连接
    if (mainPipeHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(mainPipeHandle_);
        mainPipeHandle_ = INVALID_HANDLE_VALUE;
    }

    return result;
}

void GameToolClient::setMessageHandler(MessageCallback handler) {
    messageHandler_ = handler;
}

bool GameToolClient::startPipeServer() {
    if (running_ || appPipeName_.empty()) {
        return false;
    }

    // 创建命名管道服务器
    std::string fullPipeName = "\\\\.\\pipe\\" + appPipeName_;

    serverPipeHandle_ = CreateNamedPipe(
            fullPipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096,
            4096,
            0,
            NULL
    );

    if (serverPipeHandle_ == INVALID_HANDLE_VALUE) {
        std::cerr << "创建管道服务器失败: " << GetLastError() << std::endl;
        return false;
    }

    // 启动服务器线程
    running_ = true;
    serverThread_ = std::thread(&GameToolClient::pipeServerThread, this);

    return true;
}

bool GameToolClient::sendMessage(const json& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);

    if (clients_.empty()) {
        return false;
    }

    // 将消息发送给所有客户端
    std::string messageStr = message.dump() + "\n";
    bool success = false;

    for (HANDLE client : clients_) {
        DWORD bytesWritten;
        if (WriteFile(
                client,
                messageStr.c_str(),
                static_cast<DWORD>(messageStr.size()),
                &bytesWritten,
                NULL
        )) {
            success = true;
        }
    }

    return success;
}

void GameToolClient::pipeServerThread() {
    while (running_) {
        // 等待客户端连接
        if (ConnectNamedPipe(serverPipeHandle_, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            // 创建新的管道实例
            std::string fullPipeName = "\\\\.\\pipe\\" + appPipeName_;
            HANDLE clientPipe = CreateNamedPipe(
                    fullPipeName.c_str(),
                    PIPE_ACCESS_DUPLEX,
                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                    PIPE_UNLIMITED_INSTANCES,
                    4096,
                    4096,
                    0,
                    NULL
            );

            if (clientPipe != INVALID_HANDLE_VALUE) {
                // 保存客户端连接
                {
                    std::lock_guard<std::mutex> lock(clientsMutex_);
                    clients_.push_back(serverPipeHandle_);
                }

                // 使用新的管道实例等待下一个连接
                serverPipeHandle_ = clientPipe;

                // 启动读取线程
                std::thread([this, pipe = serverPipeHandle_]() {
                    char buffer[4096];
                    DWORD bytesRead;
                    std::string messageBuffer;

                    while (running_) {
                        if (ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL)) {
                            if (bytesRead > 0) {
                                // 将接收到的数据添加到消息缓冲区
                                messageBuffer.append(buffer, bytesRead);

                                // 处理完整的消息（以换行符分隔）
                                size_t pos;
                                while ((pos = messageBuffer.find('\n')) != std::string::npos) {
                                    std::string message = messageBuffer.substr(0, pos);
                                    messageBuffer.erase(0, pos + 1);

                                    try {
                                        // 解析JSON消息
                                        json jsonMessage = json::parse(message);

                                        // 准备响应
                                        json response;

                                        // 调用消息处理器
                                        if (messageHandler_) {
                                            messageHandler_(jsonMessage, response);

                                            // 如果响应不为空，发送响应
                                            if (!response.empty()) {
                                                std::string responseStr = response.dump() + "\n";
                                                DWORD bytesWritten;
                                                WriteFile(
                                                        pipe,
                                                        responseStr.c_str(),
                                                        static_cast<DWORD>(responseStr.size()),
                                                        &bytesWritten,
                                                        NULL
                                                );
                                            }
                                        }
                                    } catch (const std::exception& e) {
                                        std::cerr << "处理消息时出错: " << e.what() << std::endl;
                                    }
                                }
                            }
                        } else {
                            // 客户端断开连接或发生错误
                            break;
                        }
                    }

                    // 客户端断开连接，清理
                    {
                        std::lock_guard<std::mutex> lock(clientsMutex_);
                        auto it = std::find(clients_.begin(), clients_.end(), pipe);
                        if (it != clients_.end()) {
                            clients_.erase(it);
                        }
                    }

                    CloseHandle(pipe);
                }).detach();
            }
        } else {
            // 连接失败，稍等后重试
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void GameToolClient::mainPipeReadThread() {
    char buffer[4096];
    DWORD bytesRead;
    std::string messageBuffer;

    while (mainPipeHandle_ != INVALID_HANDLE_VALUE) {
        if (ReadFile(mainPipeHandle_, buffer, sizeof(buffer), &bytesRead, NULL)) {
            if (bytesRead > 0) {
                // 将接收到的数据添加到消息缓冲区
                messageBuffer.append(buffer, bytesRead);

                // 处理完整的消息（以换行符分隔）
                size_t pos;
                while ((pos = messageBuffer.find('\n')) != std::string::npos) {
                    std::string message = messageBuffer.substr(0, pos);
                    messageBuffer.erase(0, pos + 1);

                    try {
                        // 解析JSON消息
                        json jsonMessage = json::parse(message);

                        // 处理特定的命令，例如注册响应
                        if (jsonMessage.contains("status")) {
                            std::string status = jsonMessage["status"];

                            if (status == "success") {
                                std::cout << "操作成功: " << jsonMessage["message"] << std::endl;
                            } else if (status == "error") {
                                std::cerr << "操作失败: " << jsonMessage["error"] << std::endl;
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "处理主管道消息时出错: " << e.what() << std::endl;
                    }
                }
            }
        } else {
            // 主管道断开连接或发生错误
            break;
        }
    }
}