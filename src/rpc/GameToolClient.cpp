//
// Created by Right on 25/5/14 星期三 16:51.
//

#include <iostream>
#include <utility>
#include "sauri/rpc/sauri_app.h"
#include "sauri/logger_helper/logger_helper.h"

// SauriApplication.cpp modifications
SauriApplication::SauriApplication(
        std::string appId,
        std::string name,
        std::string description,
        std::string iconPath,
        std::string appPipeName,
        std::string httpUrl,
        std::string localPath,
        std::string mainPipeName,
        int workerThreads
) : appId_(std::move(appId)),
    name_(std::move(name)),
    description_(std::move(description)),
    iconPath_(std::move(iconPath)),
    appPipeName_(std::move(appPipeName)),
    mainPipeName_(std::move(mainPipeName)),
    httpUrl_(std::move(httpUrl)),
    localPath_(std::move(localPath)),
    worker_threads_(workerThreads),
    running_(false) {
    InitializeLogger(7, "logs");
//    if (mainPipeName_.empty()) {
//        mainPipeName_ = appId_ + "_main_pipe";
//    }
    client_ = std::make_shared<NamedPipeClient>(io_context_client_, mainPipeName_);
    server_ = std::make_shared<NamedPipeServer>(io_context_server_, appPipeName_);
    client_->set_message_handler([this](const std::string &message) {
        std::cout << "client: " << message << std::endl;
        // 处理消息
        // {"status":"success","message":"Registration request received, initiating handshake"}
        try {
            auto msg = json::parse(message);
            if (msg.contains("status")) {
                std::string status = msg["status"];
                if (status == "success") {
                    LOG(INFO) << "[D] " << "Registration status: " << msg["status"];
                } else {
                    LOG(INFO) << "[D] " << "Registration status: " << msg["error"];
                }
            }
        } catch (std::exception &e) {
            LOG(INFO) << "[E] " << "Error parsing message: " << e.what();
            return;
        }
    });


    // 消息处理
    server_->set_message_handler([this](const std::string &message) {
        std::cout << "server recv: " << message << std::endl;
        try {
            auto msg = json::parse(message);
            auto baseMsg = msg.get<BaseRpcMessage>();
            std::cout << "timestamp: " << baseMsg.timestamp << std::endl;
            if (baseMsg.type == "handshake") {
                auto handshakeMsg = baseMsg.payload.get<HandshakeMessage>();
                // 处理握手消息
                std::cout << "Handshake step: " << handshakeMsg.step << std::endl;
                handleHandshake(handshakeMsg);
            } else if (baseMsg.type == "rpc-event") {

            } else if (baseMsg.type == "rpc-request") {
                handleRpcRequest(baseMsg);
            } else if (baseMsg.type == "rpc-response") {
                (int) 2;
            }

        } catch (std::exception &e) {
            LOG(INFO) << "[E] " << "Error parsing message: " << e.what();
            return;
        }
        // 回复该客户端
//        server->write(client_id, "服务器收到: " + message);

        // 如果消息是"broadcast"，则向所有客户端广播
        if (message == "broadcast") {
//            server->broadcast("这是一条广播消息");
        }
    });
}

SauriApplication::~SauriApplication() {
    stopWorkerThreads();
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

bool SauriApplication::initialize() {
    // Start the pipe server first
    LOG(INFO) << "[D] " << "initialize";
    startWorkerThreads();
    return startPipeServer();
}

bool SauriApplication::registerSelf() {
    // Connect to Electron
    if (!connectToDock()) {
        return false;
    }
    bind("exit", []() {
        LOG(INFO) << "[D] " << "exit";
        exit(0);
    });
    // Create registration message
    RegisterMsg msg{
            .command = "register",
            .appId = appId_,
            .appInfo = {
                    .name = name_,
                    .description = description_,
                    .icon = iconPath_,
                    .pipeName = appPipeName_,
                    .functions = std::move(get_map_keys(function_map_)),
                    .events = event_list_
            }
    };

    // Add optional parameters
    if (!httpUrl_.empty()) {
        msg.appInfo.httpUrl = httpUrl_;
    }

    if (!localPath_.empty()) {
        msg.appInfo.localPath = localPath_;
    }

    // Send registration message
    std::string message = msg.toJson().dump() + "\n";
    LOG(INFO) << "[D] " << "client send: " << message;
    client_->write(message);

    return true;
}

bool SauriApplication::connectToDock() {
    try {
        if (!client_->connect()) {
            LOG(INFO) << "[E] " << "Error connecting to dock";
            return false;
        }
        clientThread_ = std::thread([this]() {
            try {
                io_context_client_.run();
            } catch (std::exception &ec) {
                LOG(INFO) << "[E] " << "io_context_client_: " << ec.what();
            }
        });
        return true;
    } catch (const std::exception &e) {
        LOG(INFO) << "[E] " << "Error connecting to dock: " << e.what();
    }
    return false;
}

bool SauriApplication::handleHandshake(const HandshakeMessage &message) {
    if (message.step == 1) {
        auto step2 = CreateHandshakeMessage(appId_, {
                {"step", 2}
        });
        sendMessage(json(step2));
    }
    if (message.step == 3) {
        client_->close();
        return true;
    }
    return false;
}

bool SauriApplication::unregisterApp() {
    if (!server_->is_connected()) {
        return false;
    }

    // 创建注销消息
    json unregisterMsg = {
            {"command", "unregister"},
            {"appId",   appId_}
    };

    // 发送注销消息
    std::string message = unregisterMsg.dump() + "\n";
    return true;
}

bool SauriApplication::startPipeServer() {
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
            try {
                io_context_server_.run();
            } catch (std::exception &ec) {
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

bool SauriApplication::sendMessage(const BaseRpcMessage &message) {
    if (server_->is_connected()) {
        std::string msg = json(message).dump() + "\n";
        LOG(INFO) << "[D] " << "server send: " << msg;
        server_->write(msg);
        return true;
    }
    return false;
}

bool SauriApplication::sendMessage(const json &message) {
    if (server_->is_connected()) {
        std::string msg = message.dump() + "\n";
        LOG(INFO) << "[D] " << "server send: " << msg;
        server_->write(msg);
        return true;
    }
    return false;
}

void SauriApplication::exec() {
    // 等待IO线程结束
    if (clientThread_.joinable()) {
        clientThread_.join();
        LOG(INFO) << "[D] " << "clientThread_ join";
    }
    if (serverThread_.joinable()) {
        serverThread_.join();
        LOG(INFO) << "[D] " << "serverThread_ join";
    }
}

void SauriApplication::handleRpcRequest(const BaseRpcMessage &msg) {
    // Make a copy of the message for the task
    BaseRpcMessage msgCopy = msg;

    // Add task to queue
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        tasks_.emplace([this, msgCopy]() {
            RpcResponse response;
            try {
                auto request = msgCopy.payload.get<RpcRequest>();
                response.id = request.id;

                if (function_map_.find(request.method) != function_map_.end()) {
                    try {
                        response.result = function_map_[request.method](request.params);
                    }
                    catch (const std::exception &e) {
                        response.hasError = true;
                        response.error.code = static_cast<int>(RpcErrorCode::function_internal_error);
                        response.error.message = e.what();
                    }
                } else {
                    response.hasError = true;
                    response.error.code = static_cast<int>(RpcErrorCode::function_not_found);
                    response.error.message = "Method '" + request.method + "' not found";
                }
            }
            catch (const json::exception &e) {
                response.id = "unknown";
                response.hasError = true;
                response.error.code = static_cast<int>(RpcErrorCode::payload_invalid);
                response.error.message = "Invalid payload: " + std::string(e.what());
            }

            auto responseMessage = CreateResponseMessage(msgCopy.appId, json(response));
            sendMessage(responseMessage);
        });
    }

    // Notify one worker thread that a new task is available
    task_cv_.notify_one();
}

/*
void SauriApplication::handleRpcRequest(const BaseRpcMessage &msg) {
    RpcResponse response;
    try {
        auto request = msg.payload.get<RpcRequest>();
        // 处理请求
        response.id = request.id;
        // 检查方法是否存在
        if (function_map_.find(request.method) != function_map_.end()) {
            try {
                // 调用注册的方法
                response.result = function_map_[request.method](request.params);
            }
            catch (const std::exception &e) {
                // 方法执行出错
                response.hasError = true;
                response.error.code = static_cast<int>(RpcErrorCode::function_internal_error);
                response.error.message = e.what();
            }
        } else {
            // 方法不存在
            response.hasError = true;
            response.error.code = static_cast<int>(RpcErrorCode::function_not_found);
            response.error.message = "Method '" + request.method + "' not found";
        }

    }
    catch (const json::exception &e) {
        // JSON 解析错误
        response.id = "unknown";
        response.hasError = true;
        response.error.code = static_cast<int>(RpcErrorCode::payload_invalid);
        response.error.message = "Invalid payload: " + std::string(e.what());

    }

    auto responseMessage = CreateResponseMessage(msg.appId, json(response));
    sendMessage(responseMessage);
}
*/
void SauriApplication::emitEvent(const std::string &event_name, const json &data) {
    if (!event_list_.contains(event_name)) {
        LOG(INFO) << "[E] " << "Event not declared: " << event_name;
        return;
    }
    if (server_->is_connected()) {
        RpcEvent event{
                .id = to_string(uuids::uuid_system_generator{}()),
                .event = event_name,
                .data = data
        };
        auto message = CreateEventMessage(appId_, event);
        sendMessage(message);
    }
}

void SauriApplication::declareEvent(const std::string &event_name) {
    event_list_.emplace(event_name);
}

void SauriApplication::declareEvents(const std::vector<std::string> &event_names) {
    for (const auto &event_name: event_names) {
        declareEvent(event_name);
    }
}

void SauriApplication::startWorkerThreads() {
    for (size_t i = 0; i < num_workers_; ++i) {
        worker_threads_.emplace_back([this] {
            while (true) {
                std::function < void() > task;
                {
                    std::unique_lock<std::mutex> lock(task_mutex_);
                    task_cv_.wait(lock, [this] { return shutdown_ || !tasks_.empty(); });

                    if (shutdown_ && tasks_.empty()) {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

void SauriApplication::stopWorkerThreads() {
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        shutdown_ = true;
    }
    task_cv_.notify_all();
    for (auto &thread: worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
}

