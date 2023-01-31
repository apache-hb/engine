#pragma once

#include "simcoe/core/macros.h"

#include <format>

namespace simcoe {
    struct PanicInfo {
        const char *pzFile;
        const char *pzFunction;
        size_t line;
    };

    NORETURN panic(const PanicInfo& info, const char *pzMessage);

    NORETURN panic(const PanicInfo& info, const char *pzMessage, auto&&... args) {
        panic(info, std::vformat(pzMessage, std::make_format_args(args...)).c_str());
    }
}

#define PANIC(...) simcoe::panic({ .pzFile = __FILE__, .pzFunction = FUNCNAME, .line = __LINE__}, __VA_ARGS__)

#define ASSERTF(expr, ...) \
    do { \
        if (!(expr)) { \
            simcoe::PanicInfo detail { .pzFile = __FILE__, .pzFunction = FUNCNAME, .line = __LINE__}; \
            simcoe::panic(detail, __VA_ARGS__); \
        } \
    } while (0)

#define ASSERTM(expr, msg) ASSERTF(expr, "{}", msg)
#define ASSERT(expr) ASSERTM(expr, #expr)
