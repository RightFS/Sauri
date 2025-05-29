//
// Created by Right on 25/5/19 星期一 14:52.
//
#include <CLI/CLI.hpp>
#include <sauri/sauri.h>
#include "res_processor/res_processor.h"

INITIALIZE_EASYLOGGINGPP


CMRC_DECLARE(res::asar);

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

void printUsage() {
    std::cout << "Usage: program [--extract [path]]\n"
              << "  --extract      Extract resources to the specified path\n"
              << "                 If no path is specified, extract to current directory\n"
              << std::endl;
}


int main(int argc, char **argv) {

    CLI::App appcmd{"Resource Extractor Tool"};

    std::string appid = "unique_app_id";
    appcmd.add_option("--appid", appid, "Application ID")->default_str("unique_app_id");
    std::string main_pipe_name = "leigod_tool_main_pipe";
    appcmd.add_option("--pipe-name", main_pipe_name, "dock pipe name")->default_str("leigod_tool_main_pipe");
    // 添加--extract选项
    bool extract = false;
    appcmd.add_flag("--extract", extract, "Extract resources");

    // 添加路径参数，默认为当前目录"."
    std::string path = ".";
    appcmd.add_option("--path", path, "Extraction path")->default_str(".");

    // 添加版本信息
    appcmd.set_version_flag("--version", "1.0.0");

    // 解析参数
    CLI11_PARSE(appcmd, argc, argv);

    if(extract) {
        // 如果指定了--extract选项，提取资源
        std::cout << "Extracting resources to: " << path << std::endl;
        cmrc::embedded_filesystem fs = cmrc::res::asar::get_filesystem();
        if (!extractResources(fs, path)) {
            std::cerr << "Failed to extract resources." << std::endl;
            return 1;
        }
        std::cout << "Resources extracted successfully." << std::endl;
        return 0;
    }
    subtractor s;
    multiplier m;
    // 创建客户端
    SauriApplication app(
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
        MessageBoxA(0, message.c_str(), "Messagebox from C++", MB_OK | MB_ICONINFORMATION);
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
                    if (line == "exit") {
                        exit(0);
                    }
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