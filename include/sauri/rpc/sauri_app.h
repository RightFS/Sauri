//
// Created by Right on 25/5/14 16:51.
//

#pragma once
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_set>
#include "pipe/named_pipe_client.h"
#include "pipe/named_pipe_server.h"
#include "nlohmann/json.hpp"
#include "model.h"
#include "detail/call_impl.h"

using json = nlohmann::json;

class SauriApplication {
public:
    using MessageCallback = std::function<void(const json &, json &)>;

    // Constructor with app information
    SauriApplication(
            std::string appId,
            std::string name,
            std::string description,
            std::string iconPath,
            std::string appPipeName,
            std::string httpUrl = "",
            std::string localPath = "",
            std::string mainPipeName = "leigod_tool_main_pipe",
            int workerThreads = 4
    );

    ~SauriApplication();

    // Initialize local pipe server
    bool initialize();

    // Connect to Dock and register the app
    bool registerSelf();

    bool unregisterApp();

    bool startPipeServer();

    bool sendMessage(const json &message);

    void exec();

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
    bool connectToDock();

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

    std::thread serverThread_;
    std::atomic<bool> running_;


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