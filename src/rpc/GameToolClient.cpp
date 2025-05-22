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
    server_->set_message_handler([this](const std::string &message) {
        std::cout << "server: " << message << std::endl;
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
                (int)2;
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
    RegisterMsg msg{
            .command = "register",
            .appId = appId_,
            .appInfo = {
                    .name = name_,
                    .description = description_,
                    .iconPath = iconPath_,
                    .pipeName = appPipeName_
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

    client_->write(message);

    return true;
}

bool GameToolApplication::connectToDock() {
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

bool GameToolApplication::handleHandshake(const HandshakeMessage &message) {
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

bool GameToolApplication::unregisterApp() {
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

bool GameToolApplication::sendMessage(const json &message) {
    if (server_->is_connected()) {
        std::string msg = message.dump() + "\n";
        LOG(INFO) << "[D] " << "server send: " << msg;
        server_->write(msg);
        return true;
    }
    return false;
}

void GameToolApplication::exec() {
    // 等待IO线程结束
    if (clientThread_.joinable()) {
        clientThread_.join();
        LOG(INFO) << "[D] " << "clientThread_ join";
    }
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}

void GameToolApplication::handleRpcRequest(const BaseRpcMessage &msg) {
    try {
        // 解析 JSON 消息
//        auto json_msg = json::parse(message);
//
//        // 提取 RPC 请求信息
//        auto request = json_msg.get<RpcRequest>();
//        request.id = json_msg["payload"]["id"];
//        request.method = json_msg["payload"]["method"];
//
//        // 提取参数数组
//        auto& params_json = json_msg["payload"]["params"];
//        for (auto& param : params_json) {
//            request.params.push_back(param);
//        }
        auto request = msg.payload.get<RpcRequest>();
        // 处理请求
        RpcResponse response;
        response.id = request.id;

        // 检查方法是否存在
        if (function_map_.find(request.method) != function_map_.end()) {
            try {
                // 调用注册的方法
                response.result = function_map_[request.method](request.params);
            }
            catch (const std::exception& e) {
                // 方法执行出错
                response.error.has_error = true;
                response.error.code = static_cast<int>(RpcErrorCode::function_internal_error);
                response.error.message = e.what();
            }
        }
        else {
            // 方法不存在
            response.error.has_error = true;
            response.error.code = static_cast<int>(RpcErrorCode::function_not_found);
            response.error.message = "Method '" + request.method + "' not found";
        }

        // 创建响应 JSON
        json response_json;
        response_json["type"] = "rpc-response";
        response_json["appId"] = msg.appId;
        response_json["id"] = msg.id;
        response_json["timestamp"] = get_current_time_ms();

        json payload;
        payload["id"] = response.id;

        if (response.error.has_error) {
            json error;
            error["code"] = response.error.code;
            error["message"] = response.error.message;
            if (!response.error.data.is_null()) {
                error["data"] = response.error.data;
            }
            payload["error"] = error;
        }
        else {
            payload["result"] = response.result;
        }

        response_json["payload"] = payload;

        // 发送响应
        sendMessage(response_json.dump());
    }
    catch (const json::exception& e) {
        // JSON 解析错误
        json error_response;
        error_response["type"] = "rpc-response";
        error_response["id"] = "unknown";
        error_response["timestamp"] = get_current_time_ms();

        json payload;
        payload["id"] = "unknown";

        json error;
        error["code"] = -32700;
        error["message"] = std::string("Parse error: ") + e.what();
        payload["error"] = error;

        error_response["payload"] = payload;

        sendMessage(error_response.dump());
    }
}
