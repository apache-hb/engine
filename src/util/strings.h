#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <span>

namespace engine::strings {
    std::string join(std::span<std::string> parts, std::string_view sep);
    std::string cformat(const char *fmt, ...);
    std::string cformatv(const char *fmt, va_list args);
    std::vector<std::string_view> split(std::string_view str, std::string_view sep);

    std::string encode(std::wstring_view str);
}
