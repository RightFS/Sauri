/**
 * @file environment_manager_linux.cpp
 * @brief Environment manager implementation for Linux.
 * @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
 *
 * This file is part of the Leigod System Kit.
 * Created: 2025-03-13 00:25:10 (UTC)
 * Author: chenxu
 */

#include "environment_variables_linux.hpp"

#include <cstdlib>
#include <cstring>

namespace leigod {
namespace manager {

std::string EnvironmentVariablesLinux::getEnv(const std::string& name) const {
    const char* value = std::getenv(name.c_str());
    return (value) ? value : "";
}

void EnvironmentVariablesLinux::setEnv(const std::string& name, const std::string& value) {
    setenv(name.c_str(), value.c_str(), 1);
}

std::unordered_map<std::string, std::string> EnvironmentVariablesLinux::getAllEnv() const {
    std::unordered_map<std::string, std::string> envMap;
    extern char** environ;
    for (char** current = environ; *current; ++current) {
        std::string envEntry(*current);
        size_t pos = envEntry.find('=');
        if (pos != std::string::npos) {
            std::string name = envEntry.substr(0, pos);
            std::string value = envEntry.substr(pos + 1);
            envMap[name] = value;
        }
    }
    return envMap;
}

std::string EnvironmentVariablesLinux::expandEnvVars(const std::string& path) const {
    std::string expandedPath;
    wordexp_t p;
    char** w;
    wordexp(path.c_str(), &p, 0);
    w = p.we_wordv;
    expandedPath = std::string(w[0]);
    wordfree(&p);
    return expandedPath;
}

}  // namespace manager
}  // namespace leigod