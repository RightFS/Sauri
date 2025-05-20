//
// Created by Right on 25/5/16 星期五 17:52.
//

#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct RegisterMsg{
    std::string command;
    std::string appId;
    struct AppInfo{
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
    static RegisterMsg fromJson(const json& j) {
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

struct RpcMessage{
    std::string command;
    json data;
};

struct RpcEvent{
    std::string event;
    json data;
};