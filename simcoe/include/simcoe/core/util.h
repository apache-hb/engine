#pragma once

#include <string_view>

namespace simcoe::util {
    std::string narrow(std::wstring_view wstr);
    std::wstring widen(std::string_view str);
}
