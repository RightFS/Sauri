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

int main(int argc, char **argv) {
    subtractor s;
    multiplier m;
    // 创建客户端
    GameToolApplication app(
            "unique_app_id",               // App ID
            "我的应用",                    // App name
            "应用描述",                    // Description
            "图标路径",                    // Icon
            "GameToolPipe",                 // Pipe name
            "http://localhost:3000", // HTTP URL
            ""                             // Local path (optional)
    );
    // 简单的加法函数
    app.bind("add", [](double a, double b) -> double {
        std::cout << "Called add(" << a << ", " << b << ")" << std::endl;
        return a + b;
    });
    app.bind("divide", &divide);

    // 字符串连接函数
    app.bind("concat", [](const std::string &a, const std::string &b) -> std::string {
        std::cout << "Called concat(\"" << a << "\", \"" << b << "\")" << std::endl;
        return a + b;
    });

    // ... free functions
    app.bind("div", &divide);
    // ... member functions with captured instances in lambdas
    app.bind("mul", [&m](double a, double b) { return m.multiply(a, b); });
    app.bind("init", []() {
        return "ok";
    });
    app.bind("power", [](double a, double b) {
        return pow(a, b);
    });

    app.bind("alert", [&app](const std::string &message) {
        std::cout << "Alert: " << message << std::endl;
        app.emitEvent("alert", {{"message", std::string(message) + " from C++"}});
    });
    app.declareEvents({"refresh-ui", "messagebox", "alert"});
// Initialize the pipe server first
    if (app.initialize()) {
        // Then register with Electron
        app.registerSelf();

        // Create a thread to read from stdin
        std::thread inputThread([&app]() {
            std::string line;
            std::cout << "Type a message and press Enter to send an alert event (Ctrl+C to exit):" << std::endl;

            while (std::getline(std::cin, line)) {
                if (!line.empty()) {
                    app.emitEvent("alert", {{"message", line}});
                    std::cout << "Alert sent: " << line << std::endl;
                }
                std::cout << "Type another message (Ctrl+C to exit):" << std::endl;
            }
        });

        // Detach the thread to let it run independently
        inputThread.detach();
        app.exec();
    }
}