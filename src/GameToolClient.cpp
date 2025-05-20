//
// Created by Right on 25/5/14 星期三 16:51.
//

#include "GameToolClient.h"
#include <iostream>
#include <utility>
#include "logger_helper.h"

// GameToolApplication.cpp modifications
GameToolApplication::GameToolApplication(
        std::string appId,
        std::string name,
        std::string description,
        std::string iconPath,
        std::string appPipeName,
        std::string httpUrl,
        std::string localPath,
        std::string mainPipeName
) : appId_(std::move(appId)),
    name_(std::move(name)),
    description_(std::move(description)),
    iconPath_(std::move(iconPath)),
    appPipeName_(std::move(appPipeName)),
    mainPipeName_(std::move(mainPipeName)),
    httpUrl_(std::move(httpUrl)),
    localPath_(std::move(localPath)),
    mainPipeHandle_(INVALID_HANDLE_VALUE),
    serverPipeHandle_(INVALID_HANDLE_VALUE),
    running_(false) {
    InitializeLogger(7, "logs");

    client_ = std::make_shared<NamedPipeClient>(io_context_client_, mainPipeName_);
    server_ = std::make_shared<NamedPipeServer>(io_context_server_, appPipeName_);
    client_->set_message_handler([this](const std::string &message) {
        std::cout << "client: " << message << std::endl;
        // 处理消息
        if (messageHandler_) {
//            messageHandler_(message);
        }
    });


    // 消息处理
    server_->set_message_handler([](const std::string &message) {
        std::cout << "server: "<< message << std::endl;

        // 回复该客户端
//        server->send(client_id, "服务器收到: " + message);

        // 如果消息是"broadcast"，则向所有客户端广播
        if (message == "broadcast") {
//            server->broadcast("这是一条广播消息");
        }
    });
}

GameToolApplication::~GameToolApplication() {
    // 注销应用
    unregisterApp();

    // 停止管道服务器
    running_ = false;
    // 停止服务器
    server_->stop();
    io_context_server_.stop();

    // 等待服务器线程结束
    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    if (clientThread_.joinable()) {
        clientThread_.join();
    }
}

bool GameToolApplication::initialize() {
    // Start the pipe server first
    LOG(INFO) << "[D] " << "initialize";
    return startPipeServer();
}

bool GameToolApplication::registerApp() {
    // Connect to Electron
    if (!connectToDock()) {
        return false;
    }

    // Create registration message
    json registerMsg = {
            {"command", "register"},
            {"appId",   appId_},
            {"appInfo", {
                                {"name", name_},
                                {"description", description_},
                                {"iconPath", iconPath_},
                                {"pipeName", appPipeName_}
                        }}
    };

    // Add optional parameters
    if (!httpUrl_.empty()) {
        registerMsg["appInfo"]["httpUrl"] = httpUrl_;
    }

    if (!localPath_.empty()) {
        registerMsg["appInfo"]["localPath"] = localPath_;
    }

    // Send registration message
    std::string message = registerMsg.dump() + "\n";

    client_->send(message);

    return true;
}

bool GameToolApplication::connectToDock() {
    try {
        if(!client_->connect()){
            LOG(INFO)<< "[E] " << "Error connecting to dock";
            return false;
        }
        clientThread_ = std::thread([this]() {
            try{
                io_context_client_.run();
            }catch (std::exception ec){
                LOG(INFO) << "[E] " << "io_context_client_: " << ec.what();
            }
        });
        return true;
    } catch (const std::exception &e) {
        LOG(INFO) << "[E] " << "Error connecting to dock: " << e.what();
    }
    return false;
}

bool GameToolApplication::unregisterApp() {
    if (mainPipeHandle_ == INVALID_HANDLE_VALUE || appId_.empty()) {
        return false;
    }

    // 创建注销消息
    json unregisterMsg = {
            {"command", "unregister"},
            {"appId",   appId_}
    };

    // 发送注销消息
    std::string message = unregisterMsg.dump() + "\n";
    DWORD bytesWritten;

    bool result = WriteFile(
            mainPipeHandle_,
            message.c_str(),
            static_cast<DWORD>(message.size()),
            &bytesWritten,
            nullptr
    ) != 0;

    // 关闭主管道连接
    CloseHandle(mainPipeHandle_);
    mainPipeHandle_ = INVALID_HANDLE_VALUE;

    return result;
}

void GameToolApplication::setMessageHandler(MessageCallback handler) {
    messageHandler_ = std::move(handler);
}

bool GameToolApplication::startPipeServer() {
    if (running_ || appPipeName_.empty()) {
        return false;
    }
    try {
        // 启动服务器
        server_->start();
        // 设置运行标志
        running_ = true;
        // 运行服务器（可以在单独的线程中运行）
        serverThread_ = std::thread([this]() {
            try{
                io_context_server_.run();
            }catch (std::exception ec){
                LOG(INFO) << "[E] " << "io_context_server_: " << ec.what();
            }
        });
    }
    catch (std::exception &e) {
        LOG(INFO) << "[E] " << "Error starting server: " << e.what();
        return false;
    }
    return true;
}

bool GameToolApplication::sendMessage(const json &message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);

    if (clients_.empty()) {
        return false;
    }

    // 将消息发送给所有客户端
    std::string messageStr = message.dump() + "\n";
    bool success = false;

    for (HANDLE client: clients_) {
        DWORD bytesWritten;
        if (WriteFile(
                client,
                messageStr.c_str(),
                static_cast<DWORD>(messageStr.size()),
                &bytesWritten,
                nullptr
        )) {
            success = true;
        }
    }

    return success;
}

void GameToolApplication::exec() {
    // 等待IO线程结束
    if (clientThread_.joinable()) {
        clientThread_.join();
    }
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}
