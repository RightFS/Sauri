//
// Created by Right on 25/5/14 星期三 16:51.
//

#pragma once

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include "rpc/NamedPipeClient.h"
#include "rpc/NamedPipeServer.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameToolApplication {
public:
    using MessageCallback = std::function<void(const json&, json&)>;

    // Constructor with app information
    GameToolApplication(
            std::string  appId,
            std::string  name,
            std::string  description,
            std::string  iconPath,
            std::string  appPipeName,
            std::string  httpUrl = "",
            std::string  localPath = "",
            std::string  mainPipeName = "electron_main_pipe"
    );
    ~GameToolApplication();

    // Initialize local pipe server
    bool initialize();

    // Connect to Dock and register the app
    bool registerApp();

    // 注销应用
    bool unregisterApp();

    // 设置消息处理回调
    void setMessageHandler(MessageCallback handler);

    // 启动应用管道服务器
    bool startPipeServer();

    // 发送消息给对应的Dock窗口
    bool sendMessage(const json& message);

    void exec();

private:
    // 连接到Dock主管道
    bool connectToDock();

    bool handleHandshake(const json& message);

    // App information stored as member variables
    std::string appId_;
    std::string appPipeName_;
    std::string mainPipeName_;
    std::string name_;
    std::string description_;
    std::string iconPath_;
    std::string httpUrl_;
    std::string localPath_;
    MessageCallback messageHandler_;

    // 主管道连接
    HANDLE mainPipeHandle_;

    // 应用管道服务器
    HANDLE serverPipeHandle_;
    std::thread serverThread_;
    std::atomic<bool> running_;

    // 连接的客户端
    std::mutex clientsMutex_;
    std::vector<HANDLE> clients_;

    // 主管道读取线程
    std::thread clientThread_;

    std::shared_ptr<NamedPipeClient> client_;
    std::shared_ptr<NamedPipeServer> server_;
    asio::io_context io_context_client_;
    asio::io_context io_context_server_;
};