#pragma once
#ifndef GAME_TOOL_BASE_CALL_IMPL_H
#define GAME_TOOL_BASE_CALL_IMPL_H

#include <nlohmann/json.hpp>
namespace rpc::detail {
    using json = nlohmann::json;

    // Function traits helpers - these remain unchanged
    template<typename T>
    struct function_traits : function_traits<decltype(&T::operator())> {};

    template<typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        using arg_type = std::tuple_element_t<N, args_tuple>;
    };

    template<typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...)> {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        using arg_type = std::tuple_element_t<N, args_tuple>;
    };

    template<typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const> {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        using arg_type = std::tuple_element_t<N, args_tuple>;
    };

    // Implementation detail for calling function with expanded parameters
    template<typename Func, typename... Args, size_t... Is>
    auto call_impl(Func&& func, const std::vector<json>& params, std::index_sequence<Is...>) {
        return std::invoke(std::forward<Func>(func), params[Is].get<std::decay_t<Args>>()...);
    }

    // Primary dispatcher - handles different callable types
    template<typename Func>
    struct callable_dispatcher {
        // For lambdas and generic callable objects
        template<typename F = Func>
        static auto dispatch(F&& func, const std::vector<json>& params) {
            using traits = function_traits<std::decay_t<F>>;
            constexpr size_t arity = traits::arity;

            if (params.size() != arity) {
                throw std::runtime_error("Parameter count mismatch");
            }

            return dispatch_impl(std::forward<F>(func), params,
                                 std::make_index_sequence<arity>{},
                                 static_cast<typename traits::args_tuple*>(nullptr));
        }

        // Helper to extract arguments from lambda/callable
        template<typename F, size_t... Is, typename... Args>
        static auto dispatch_impl(F&& func, const std::vector<json>& params,
                                  std::index_sequence<Is...>, std::tuple<Args...>*) {
            return call_impl<F, Args...>(std::forward<F>(func), params, std::index_sequence<Is...>{});
        }
    };

    // Specialization for function pointers
    template<typename Ret, typename... Args>
    struct callable_dispatcher<Ret(*)(Args...)> {
        static auto dispatch(Ret(*func)(Args...), const std::vector<json>& params) {
            if (params.size() != sizeof...(Args)) {
                throw std::runtime_error("Parameter count mismatch");
            }

            return call_impl<Ret(*)(Args...), Args...>(
                    func, params, std::index_sequence_for<Args...>{}
            );
        }
    };

    // Specialization for member function pointers
    template<typename Class, typename Ret, typename... Args>
    struct callable_dispatcher<Ret(Class::*)(Args...)> {
        static auto dispatch(Ret(Class::*func)(Args...), const std::vector<json>& params) {
            if (params.size() != sizeof...(Args) + 1) { // +1 for the instance
                throw std::runtime_error("Parameter count mismatch");
            }

            Class* instance = params[0].get<Class*>();
            auto reduced_params = std::vector<json>(params.begin() + 1, params.end());

            return call_impl<Ret(Class::*)(Args...), Args...>(
                    [&](Args... args) -> Ret { return (instance->*func)(args...); },
                    reduced_params, std::index_sequence_for<Args...>{}
            );
        }
    };

    // Single unified interface for all callable types
    template<typename Func>
    json call_with_json_params(Func&& func, const std::vector<json>& params) {
        using dispatcher = callable_dispatcher<std::decay_t<Func>>;
        using func_traits = function_traits<std::decay_t<Func>>;
        using return_type = typename func_traits::return_type;

        if constexpr (std::is_void_v<return_type>) {
            dispatcher::dispatch(std::forward<Func>(func), params);
            return {nullptr};
        } else {
            auto result = dispatcher::dispatch(std::forward<Func>(func), params);
            return {result};
        }
    }
}
#endif //GAME_TOOL_BASE_CALL_IMPL_H