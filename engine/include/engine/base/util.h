#pragma once

#include <string>
#include <string_view>

namespace engine {
    std::string narrow(std::wstring_view str);
    std::wstring widen(std::string_view str);
}
