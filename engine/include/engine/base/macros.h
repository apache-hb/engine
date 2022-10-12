#pragma once

#define UNUSED [[maybe_unused]]
#define NORETURN [[noreturn]] void

// not worth the hassle
#ifdef __APPLE__
#    error __APPLE__
#endif

#if defined(__clang__)
#    define CC_CLANG 1
#elif defined(__GNUC__)
#    define CC_GNU 1
#elif defined(_MSC_VER)
#    define CC_MSVC 1
#else
#    error "unknown compiler"
#endif

#if defined(__linux__)
#    define OS_LINUX 1
#elif defined(_WIN32)
#    define OS_WINDOWS 1
#else
#    error "unknown platform"
#endif

#if CC_CLANG || CC_GNU
#    ifndef NORETURN
#        define NORETURN _Noreturn void
#    endif
#elif CC_MSVC
#    ifndef NORETURN
#        define NORETURN __declspec(noreturn) void
#    endif
#endif

#ifdef CC_MSVC
#    define ASSUME(expr) __assume(expr)
#else
#    define ASSUME(expr)                                                                                               \
        do                                                                                                             \
        {                                                                                                              \
            if (!(expr))                                                                                               \
            {                                                                                                          \
                __builtin_unreachable();                                                                               \
            }                                                                                                          \
        } while (0)
#endif

#if CC_GNU
#    define FUNCNAME __PRETTY_FUNCTION__
#endif

#ifndef FUNCNAME
#    define FUNCNAME __func__
#endif

#define BITFLAGS(ID, BASE, ...) \
    enum ID __VA_ARGS__; \
    constexpr ID operator|(ID lhs, ID rhs) { return ID(BASE(lhs) | BASE(rhs)); } \
    constexpr ID operator&(ID lhs, ID rhs) { return ID(BASE(lhs) & BASE(rhs)); }
    