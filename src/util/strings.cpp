#include "strings.h"

#include <sstream>

namespace strings {
    std::string join(std::span<std::string> parts, std::string_view sep) {
        std::stringstream ss;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i != 0) { ss << sep; }
            ss << parts[i];
        }
        return ss.str();
    }
}
