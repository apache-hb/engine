#pragma once

#define NORETURN [[noreturn]] void

namespace simcoe {
    struct PanicInfo {
        const char *pzFile;
        const char *pzFunction;
        size_t line;
    };

    NORETURN panic(const PanicInfo &panic, const char *pzMessage);
}

#define ASSERT(...)