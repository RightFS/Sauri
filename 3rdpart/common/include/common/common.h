//
// Created by Administrator on 2025/4/14.
//

#ifndef NNGAME_COMMON_H
#define NNGAME_COMMON_H

#include "common/cache_data.h"
#include "common/error.h"
#include "common/exception.h"
#include "common/task/i_task_controller.h"
#include "common/task/i_task_listener.h"
#include "common/task/task.h"
#include "common/task/task_data.h"
#include "common/task/task_manager.h"
#include "common/utils/path.h"
#include "common/utils/strings.h"

#include <algorithm>

namespace leigod {
namespace common {

// 通用模板函数，自动推导返回类型
template <typename C, typename... Args>
auto SafeCall(C&& cb,
              Args&&... args) noexcept(noexcept(std::forward<C>(cb)(std::forward<Args>(args)...)))
    -> decltype(std::forward<C>(cb)(std::forward<Args>(args)...)) {
    if constexpr (std::is_void_v<decltype(std::forward<C>(cb)(std::forward<Args>(args)...))>) {
        if (cb) {
            std::forward<C>(cb)(std::forward<Args>(args)...);
        }
    } else {
        if (cb) {
            return std::forward<C>(cb)(std::forward<Args>(args)...);
        }
        return {};
    }
}

}  // namespace common
}  // namespace leigod

#endif  // NNGAME_COMMON_H
