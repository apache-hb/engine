#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <span>

namespace strings {
    std::string join(std::span<std::string> parts, std::string_view sep);
}
