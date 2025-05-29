//
// Created by Right on 25/5/27 星期二 11:35.
//

#pragma once
#ifndef GAME_TOOL_BASE_RES_PROCESSOR_H
#define GAME_TOOL_BASE_RES_PROCESSOR_H
#include <filesystem>
#include <fstream>
#include <iostream>
#include "cmrc/cmrc.hpp"
inline bool extractResourcesRecursive(const cmrc::embedded_filesystem &fs, const std::string &extractPath,
                               const std::string &resourceDir = ""){
    try {

        // 遍历当前资源目录
        for (const auto &entry: fs.iterate_directory(resourceDir)) {
            std::string resourcePath = resourceDir.empty() ?
                                       entry.filename() : resourceDir + "/" + entry.filename();

            if (entry.is_directory()) {
                // 递归处理子目录
                std::filesystem::path dirPath = std::filesystem::path(extractPath) / resourcePath;
                std::filesystem::create_directories(dirPath);

                if (!extractResourcesRecursive(fs, extractPath, resourcePath)) {
                    return false;
                }
            } else {
                // 处理文件
                std::filesystem::path targetPath = std::filesystem::path(extractPath) / resourcePath;
                std::filesystem::create_directories(targetPath.parent_path());

                auto resource = fs.open(resourcePath);
                std::ofstream outFile(targetPath, std::ios::binary);
                if (!outFile) {
                    std::cerr << "Error: Cannot write file " << targetPath << std::endl;
                    return false;
                }

                outFile.write(resource.begin(), resource.size());

                if (outFile.good()) {
                    std::cout << "Extracted: " << resourcePath << " -> " << targetPath << std::endl;
                } else {
                    std::cerr << "Error writing file: " << targetPath << std::endl;
                    return false;
                }
            }
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error during recursive extraction: " << e.what() << std::endl;
        return false;
    }
}

inline bool extractResources(const cmrc::embedded_filesystem &fs, const std::string &extractPath){
    std::cout << "Extracting resources to: " << extractPath << std::endl;

    // 确保目标路径存在
    std::filesystem::create_directories(extractPath);

    if (extractResourcesRecursive(fs, extractPath)) {
        std::cout << "Resource extraction completed successfully." << std::endl;
        return true;
    }
    return false;
}
#endif //GAME_TOOL_BASE_RES_PROCESSOR_H
