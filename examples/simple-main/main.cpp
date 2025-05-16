//
// Created by Right on 25/5/15 星期四 15:46.
//
#include <iostream>
#include "GameToolClient.h"
int foo(){
    return 0;
}

int main(int argc, char** argv){
    foo();
    // 创建客户端
    GameToolApplication app(
            "unique_app_id",               // App ID
            "我的应用",                    // App name
            "应用描述",                    // Description
            "图标路径",                    // Icon
            R"(\\.\pipe\electron_main_pipe)",                 // Pipe name
            "http://localhost:8081/index.html", // HTTP URL
            ""                             // Local path (optional)
    );

// Initialize the pipe server first
    if (app.initialize()) {
        // Then register with Electron
        app.registerApp();

        // Set message handler
        app.setMessageHandler([](const json& message, json& response) {
            // Handle messages from Electron
            std::cout << "Received message: " << message.dump() << std::endl;
        });
        app.exec();
    }
}