//
// Created by Administrator on 2024/10/18.
//

#ifndef IM_CROSS_SDK_CACHEDATA_H
#define IM_CROSS_SDK_CACHEDATA_H

#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace leigod {
namespace common {

template <typename K, typename T>
class CacheData {
public:
    ~CacheData() = default;

    void add(K key, const T& value) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        cache_data_[key] = value;
    }

    T& get(K key) {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        return cache_data_[key];
    }

    void remove(K key) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        cache_data_.erase(key);
    }

    bool exists(K key) {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        return cache_data_.find(key) != cache_data_.end();
    }

protected:
    std::shared_mutex cache_mutex_{};
    std::unordered_map<K, T> cache_data_{};
};

}  // namespace common
}  // namespace leigod

#endif  // IM_CROSS_SDK_CACHEDATA_H
