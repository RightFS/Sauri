//
// Created by Right on 25/5/16 星期五 17:52.
//

#pragma once

#define UUID_SYSTEM_GENERATOR 1

#include <nlohmann/json.hpp>
#include <utility>
#include <uuid.h>
#include <nn_utils.h>
using json = nlohmann::json;

struct RegisterMsg {
    std::string command;
    std::string appId;
    struct AppInfo {
        std::string name;
        std::string description;
        std::string iconPath;
        std::string pipeName;
        std::string httpUrl;
        std::string localPath;
    } appInfo;

    // to json
    json toJson() const {
        json j;
        j["command"] = command;
        j["appId"] = appId;
        j["appInfo"]["name"] = appInfo.name;
        j["appInfo"]["description"] = appInfo.description;
        j["appInfo"]["iconPath"] = appInfo.iconPath;
        j["appInfo"]["pipeName"] = appInfo.pipeName;
        if (!appInfo.httpUrl.empty()) {
            j["appInfo"]["httpUrl"] = appInfo.httpUrl;
        }
        if (!appInfo.localPath.empty()) {
            j["appInfo"]["localPath"] = appInfo.localPath;
        }
        return j;
    }

    // from json
    static RegisterMsg fromJson(const json &j) {
        RegisterMsg msg;
        msg.command = j["command"];
        msg.appId = j["appId"];
        msg.appInfo.name = j["appInfo"]["name"];
        msg.appInfo.description = j["appInfo"]["description"];
        msg.appInfo.iconPath = j["appInfo"]["iconPath"];
        msg.appInfo.pipeName = j["appInfo"]["pipeName"];
        if (j["appInfo"].contains("httpUrl")) {
            msg.appInfo.httpUrl = j["appInfo"]["httpUrl"];
        }
        if (j["appInfo"].contains("localPath")) {
            msg.appInfo.localPath = j["appInfo"]["localPath"];
        }
        return msg;
    }
};

// RPC 请求消息结构
struct RpcRequest {
    std::string id;
    std::string method;
    std::vector<json> params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RpcRequest, id, method, params)
};
struct RpcResponseError {
    int code;
    std::string message;
    json data;
    bool has_error{false};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RpcResponseError, code, message, data, has_error)
};
// RPC 响应消息结构
struct RpcResponse {
    std::string id;
    json result;
    RpcResponseError error;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RpcResponse, id, result, error)
};

struct HandshakeMessage {
    int step = 2;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(HandshakeMessage, step)
};

// Base message class with common fields and serialization
struct BaseRpcMessage {
    std::string type;
    std::string appId;
    std::string id;
    uint64_t timestamp{};
    nlohmann::json payload;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BaseRpcMessage, type, appId, id, timestamp, payload)

};

enum class RpcErrorCode {
    function_not_found = 404,
    function_internal_error = 500,
};

inline BaseRpcMessage CreateRpcMessage(const std::string &appId, const std::string &type, nlohmann::json payload) {
    BaseRpcMessage message;
    message.appId = appId;
    message.type = type;
    auto const id = uuids::uuid_system_generator{}();
    message.id = to_string(id);
    message.timestamp = get_current_time_ms();
    message.payload = std::move(payload);
    return message;
}

inline BaseRpcMessage CreateHandshakeMessage(const std::string &appId, nlohmann::json payload) {
    return CreateRpcMessage(appId, "handshake", std::move(payload));
}