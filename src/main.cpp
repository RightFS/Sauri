//
// Created by Right on 25/5/19 星期一 14:52.
//
#include "rpc/GameToolClient.h"

#include <logger_helper.h>

INITIALIZE_EASYLOGGINGPP
double divide(double a, double b) {
    if (b == 0.0) {
        throw std::runtime_error("Division by zero");
    }
    return a / b;
}
struct subtractor {
    double operator()(double a, double b) {
        return a - b;
    }
};

struct multiplier {
    double multiply(double a, double b) {
        return a * b;
    }
};

int main(int argc, char** argv){
    subtractor s;
    multiplier m;
    // 创建客户端
    GameToolApplication app(
            "unique_app_id",               // App ID
            "我的应用",                    // App name
            "应用描述",                    // Description
            "图标路径",                    // Icon
            "GameToolPipe",                 // Pipe name
            "http://localhost:8081/index.html", // HTTP URL
            ""                             // Local path (optional)
    );
    // 简单的加法函数
    app.bind("add", [](double a, double b) -> double {
        std::cout << "Called add(" << a << ", " << b << ")" << std::endl;
        return a + b;
    });
    app.bind("divide", &divide);

    // 字符串连接函数
    app.bind("concat", [](const std::string& a, const std::string& b) -> std::string {
        std::cout << "Called concat(\"" << a << "\", \"" << b << "\")" << std::endl;
        return a + b;
    });

    app.bind("sub", s);
    // ... free functions
    app.bind("div", &divide);
    // ... member functions with captured instances in lambdas
    app.bind("mul", [&m](double a, double b) { return m.multiply(a, b); });
    app.bind("init", []() {
        return "ok";
    });
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