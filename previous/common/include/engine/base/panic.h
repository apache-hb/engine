#pragma once

#include "engine/base/macros.h"

#include <string>
#include <string_view>
#include <format>

namespace simcoe {
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
            simcoe::PanicInfo panic { .file = __FILE__, .function = FUNCNAME, .line = __LINE__};                                                        \
            simcoe::panic(panic, std::format(__VA_ARGS__));                                                                           \
        }                                                                                                          \
    } while (0)

#define ASSERTM(expr, msg) ASSERTF(expr, msg)
#define ASSERT(expr) ASSERTM(expr, #expr)

#define HR_CHECK(expr) do { \
        HRESULT hr = (expr); \
        ASSERTF(SUCCEEDED(hr), #expr " => {}", simcoe::win32::hrErrorString(hr)); \
    } while (0)
