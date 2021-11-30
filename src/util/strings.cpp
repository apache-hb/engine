#include "strings.h"

#include <sstream>

namespace strings {
    std::string join(const std::vector<std::string> &parts, const std::string &sep) {
        std::stringstream ss;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i != 0) { ss << sep; }
            ss << parts[i];
        }
        return ss.str();
    }
}
