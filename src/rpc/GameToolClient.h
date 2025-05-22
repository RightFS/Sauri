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
#include <unordered_set>
#include "pipe/NamedPipeClient.h"
#include "pipe/NamedPipeServer.h"
#include "nlohmann/json.hpp"
#include "../model/model.h"
#include "detail/call_impl.h"

using json = nlohmann::json;

class GameToolApplication {
public:
    using MessageCallback = std::function<void(const json &, json &)>;

    // Constructor with app information
    GameToolApplication(
            std::string appId,
            std::string name,
            std::string description,
            std::string iconPath,
            std::string appPipeName,
            std::string httpUrl = "",
            std::string localPath = "",
            int workerThreads = 4,
            std::string mainPipeName = "electron_main_pipe"
    );

    ~GameToolApplication();

    // Initialize local pipe server
    bool initialize();

    // Connect to Dock and register the app
    bool registerSelf();

    // 注销应用
    bool unregisterApp();

    // 启动应用管道服务器
    bool startPipeServer();

    // 发送消息给对应的Dock窗口
    bool sendMessage(const json &message);

    void exec();

    // 绑定 RPC 方法 - Lambda 版本
    template<typename Func>
    void bind(const std::string &method_name, Func &&func) {
        function_map_[method_name] = [f = std::forward<Func>(func)](const std::vector<json> &params) -> json {
            return rpc::detail::call_with_json_params(f, params);
        };
    }

    void declareEvent(const std::string &event_name);

    void declareEvents(const std::vector<std::string> &event_names);

    void emitEvent(const std::string &event_name, const json &data);

private:
    // 连接到Dock主管道
    bool connectToDock();

    // 处理接收到的 RPC 请求
    void handleRpcRequest(const BaseRpcMessage &message);

    bool handleHandshake(const HandshakeMessage &message);

    void startWorkerThreads();

    void stopWorkerThreads();

    // App information stored as member variables
    std::string appId_;
    std::string appPipeName_;
    std::string mainPipeName_;
    std::string name_;
    std::string description_;
    std::string iconPath_;
    std::string httpUrl_;
    std::string localPath_;

    // 应用管道服务器
    std::thread serverThread_;
    std::atomic<bool> running_;


    // 主管道读取线程
    std::thread clientThread_;

    std::shared_ptr<NamedPipeClient> client_;
    std::shared_ptr<NamedPipeServer> server_;
    asio::io_context io_context_client_;
    asio::io_context io_context_server_;

    std::unordered_map<std::string, std::function<json(const json &)>> function_map_;
    std::unordered_set<std::string> event_list_;

    bool sendMessage(const BaseRpcMessage &message);

    std::vector<std::thread> worker_threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex task_mutex_;
    std::condition_variable task_cv_;
    bool shutdown_ = false;
    const size_t num_workers_ = 4; // Adjust based on your needs
};