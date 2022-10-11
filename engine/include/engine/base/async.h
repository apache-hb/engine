#pragma once

#include <future>
#include <type_traits>

namespace engine::async {
    template<typename TResult, typename TFunc, typename... TArgs>
    concept IsSignature = requires(TFunc func, TArgs... args) {
        { func(args...) } -> std::is_convertible<TResult>;
    };

    template<typename T, typename F, typename... A> requires IsSignature<T, F, A...>
    std::future<T> launch(F&& func, A&&... args) {
        return std::async(std::launch::async, func, std::forward<A>(args)...);
    }
}
