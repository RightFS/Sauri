//
// Created by Right on 25/5/21  17:40.
//

#pragma once
#ifndef GAME_TOOL_BASE_NN_UTILS_H
#define GAME_TOOL_BASE_NN_UTILS_H

#include <chrono>

inline int64_t get_current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

template <typename T,
        typename = std::enable_if_t<
                std::is_same_v<typename T::key_type, std::string>
        >>
inline std::vector<std::string> get_map_keys(const T& map) {
    std::vector<std::string> keys;
    keys.reserve(map.size());

    for (const auto& pair : map) {
        keys.push_back(pair.first);
    }

    return keys;
}

#endif //GAME_TOOL_BASE_NN_UTILS_H
