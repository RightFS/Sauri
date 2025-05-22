#pragma once
#ifndef GAME_TOOL_BASE_CALL_IMPL_H
#define GAME_TOOL_BASE_CALL_IMPL_H

#include <nlohmann/json.hpp>
#include <functional>
#include <tuple>
#include <type_traits>
#include <vector>

namespace rpc::detail {
    using json = nlohmann::json;

    // Function traits helpers
    template<typename T>
    struct function_traits;

    // For free functions
    template<typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        using arg_type = std::tuple_element_t<N, args_tuple>;
    };

    // For member functions
    template<typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...)> {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        using arg_type = std::tuple_element_t<N, args_tuple>;
    };

    // For const member functions
    template<typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const> {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        using arg_type = std::tuple_element_t<N, args_tuple>;
    };

    // For lambdas and other callable objects
    template<typename T>
    struct function_traits : function_traits<decltype(&T::operator())> {};

    // Helper to call function with the right arguments extracted from JSON array
    template<typename F, size_t... I>
    auto call_function_helper(F&& f, const std::vector<json>& params, std::index_sequence<I...>) {
        return std::invoke(std::forward<F>(f),
                           params[I].get<std::remove_cv_t<std::remove_reference_t<typename function_traits<std::decay_t<F>>::template arg_type<I>>>>()...);
    }

    // Call function with arguments from JSON array
    template<typename F>
    auto call_with_json_params(F&& f, const std::vector<json>& params) {
        constexpr auto arity = function_traits<std::decay_t<F>>::arity;

        if (params.size() != arity) {
            throw std::runtime_error("Parameter count mismatch. Expected " +
                                     std::to_string(arity) + ", got " +
                                     std::to_string(params.size()));
        }

        if constexpr (std::is_void_v<typename function_traits<std::decay_t<F>>::return_type>) {
            call_function_helper(std::forward<F>(f), params, std::make_index_sequence<arity>{});
            return json(nullptr);
        } else {
            auto result = call_function_helper(std::forward<F>(f), params, std::make_index_sequence<arity>{});
            return json(result);
        }
    }

    // Specialization for function pointers to avoid forwarding issues
    template<typename Ret, typename... Args>
    json call_with_json_params(Ret(*f)(Args...), const std::vector<json>& params) {
        constexpr auto arity = sizeof...(Args);

        if (params.size() != arity) {
            throw std::runtime_error("Parameter count mismatch. Expected " +
                                     std::to_string(arity) + ", got " +
                                     std::to_string(params.size()));
        }

        if constexpr (std::is_void_v<Ret>) {
            call_function_helper(f, params, std::make_index_sequence<arity>{});
            return json(nullptr);
        } else {
            auto result = call_function_helper(f, params, std::make_index_sequence<arity>{});
            return json(result);
        }
    }
}

#endif //GAME_TOOL_BASE_CALL_IMPL_H