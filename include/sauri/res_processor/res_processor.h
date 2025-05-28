//
// Created by Right on 25/5/27 星期二 11:35.
//

#pragma once
#ifndef GAME_TOOL_BASE_RES_PROCESSOR_H
#define GAME_TOOL_BASE_RES_PROCESSOR_H
#include <cmrc/cmrc.hpp>
bool extractResourcesRecursive(const cmrc::embedded_filesystem &fs, const std::string &extractPath,
                               const std::string &resourceDir = "");

bool extractResources(const cmrc::embedded_filesystem &fs, const std::string &extractPath);
#endif //GAME_TOOL_BASE_RES_PROCESSOR_H
