#include "engine/base/util.h"

#include "engine/base/win32.h"

using namespace engine;

std::string engine::narrow(std::wstring_view str) {
    std::string result(str.size() + 1, '\0');
    size_t size = result.size();

    errno_t err = wcstombs_s(&size, result.data(), result.size(), str.data(), str.size());
    if (err != 0) {
        return "";
    }

    result.resize(size - 1);
    return result;
}

std::wstring engine::widen(std::string_view str) {
    auto needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
    std::wstring result(needed, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), (int)result.size());
    return result;
}

namespace {
    size_t getPerfCounter() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return size_t(now.QuadPart);
    }
}

Timer::Timer() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    frequency = size_t(freq.QuadPart);
    last = getPerfCounter();
}

double Timer::tick() {
    size_t now = getPerfCounter();
    double elapsed = double(now - last) / frequency;
    last = now;
    return elapsed;
}