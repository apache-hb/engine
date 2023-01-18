#pragma once

#include "simcoe/core/macros.h"

#include <format>

namespace simcoe {
    struct PanicInfo {
        const char *pzFile;
        const char *pzFunction;
        size_t line;
    };

    NORETURN panic(const PanicInfo &panic, const char *pzMessage);
}

#define ASSERTF(expr, ...) \
    do { \
        if (!(expr)) { \
            simcoe::PanicInfo panic { .pzFile = __FILE__, .pzFunction = FUNCNAME, .line = __LINE__}; \
            simcoe::panic(panic, std::format(__VA_ARGS__).c_str()); \
        } \
    } while (0)

#define ASSERTM(expr, msg) ASSERTF(expr, msg)
#define ASSERT(expr) ASSERTM(expr, #expr)
