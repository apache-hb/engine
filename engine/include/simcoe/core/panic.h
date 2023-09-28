#pragma once

#include <format>
#include <string_view>

namespace simcoe {
    struct PanicInfo {
        std::string_view file;
        std::string_view fn;
        size_t line;
    };

    [[noreturn]] void panic(const PanicInfo& info, std::string_view msg);
}

#define PANIC(...)                                                     \
    do {                                                               \
        const simcoe::PanicInfo panicData = {__FILE__, __func__, __LINE__}; \
        simcoe::panic(panicData, std::format(__VA_ARGS__));                 \
    } while (false)

#define ASSERTF(expr, ...)      \
    do {                        \
        bool panicResult = (expr);      \
        if (!panicResult) {             \
            PANIC(__VA_ARGS__); \
        }                       \
    } while (false)

#define ASSERT(expr)                              \
    do {                                          \
        bool panicResult = (expr);                        \
        if (!panicResult) {                               \
            PANIC("assertion failed: {}", #expr); \
        }                                         \
    } while (false)

#define ONCE(...)                 \
    do {                          \
        static bool panicNext = false; \
        if (!panicNext) {              \
            panicNext = true;          \
            __VA_ARGS__;          \
        }                         \
    } while (false)

#define NEVER(...) PANIC(__VA_ARGS__)
