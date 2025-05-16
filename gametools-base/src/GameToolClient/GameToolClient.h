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
#include <windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameToolApplication {
public:
    using MessageCallback = std::function<void(const json&, json&)>;

    GameToolApplication();
    ~GameToolApplication();

    // 连接到Electron并注册应用
    bool registerApp(
            const std::string& appId,
            const std::string& name,
            const std::string& description,
            const std::string& iconPath,
            const std::string& pipeName,
            const std::string& httpUrl = "",
            const std::string& localPath = ""
    );

    // 注销应用
    bool unregisterApp();

    // 设置消息处理回调
    void setMessageHandler(MessageCallback handler);

    // 启动应用管道服务器
    bool startPipeServer();

    // 发送消息给对应的Electron窗口
    bool sendMessage(const json& message);

private:
    // 连接到Electron主管道
    bool connectToDock();

    // 管道接收线程
    void pipeServerThread();

    // 主管道读取线程
    void mainPipeReadThread();

    std::string appId_;
    std::string appPipeName_;
    std::string name_;
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
    std::thread mainPipeThread_;
};