#pragma once

#include "engine/base/macros.h"

#include <string>
#include <string_view>
#include <format>

namespace engine {
    struct PanicInfo {
        std::string_view file;
        std::string_view function;
        size_t line;
    };

    NORETURN panic(const PanicInfo &panic, const std::string &msg);
}

#define ASSERTF(expr, ...)                                                                                       \
    do                                                                                                             \
    {                                                                                                              \
        if (!(expr))                                                                                               \
        {                                                                                                          \
            engine::PanicInfo panic{__FILE__, FUNCNAME, __LINE__};                                                        \
            engine::panic(panic, std::format(__VA_ARGS__));                                                                           \
        }                                                                                                          \
    } while (0)

#define ASSERTM(expr, msg) ASSERTF(expr, msg)
#define ASSERT(expr) ASSERTM(expr, #expr)
