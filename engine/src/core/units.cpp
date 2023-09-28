#include "simcoe/core/units.h"
#include "simcoe/core/util.h"

#include <format>
#include <vector>

std::string simcoe::units::Memory::string() const {
    if (bytes == 0) { return "0b"; }

    std::vector<std::string> parts;
    size_t total = bytes;

    for (int fmt = eLimit - 1; fmt >= 0; fmt--) {
        size_t size = total / kSizes[fmt];
        if (size > 0) {
            parts.push_back(std::format("{}{}", size, kNames[fmt]));
            total %= kSizes[fmt];
        }
    }

    return util::join("+", parts);
}
