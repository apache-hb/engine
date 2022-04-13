#include "units.h"

#include <format>
#include "strings.h"

std::string engine::units::Memory::string() const {
    if (bytes == 0) { return "0b"; }

    std::vector<std::string> parts;
    size_t total = bytes;

    for (int fmt = LIMIT - 1; fmt >= 0; fmt--) {
        size_t size = total / kSizes[fmt];
        if (size > 0) {
            parts.push_back(std::format("{}{}", size, kNames[fmt]));
            total %= kSizes[fmt];
        }
    }

    return strings::join(parts, " ");
}
