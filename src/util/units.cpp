#include "units.h"

#include <format>
#include "strings.h"

namespace units {
    namespace {
        std::string format_memory(size_t bytes, Memory::Unit most) {
            size_t gb = SIZE_MAX, mb = SIZE_MAX, kb = SIZE_MAX;
            std::vector<std::string> parts;
            
            switch (most) {
            case Memory::GIGABYTES:
                gb = bytes / Memory::gigabyte;
                bytes -= (gb * Memory::gigabyte);
                parts.push_back(std::format("{}gb", gb));

                if (bytes == 0) { goto result; }

            case Memory::MEGABYTES:
                mb = bytes / Memory::megabyte;
                bytes -= (mb * Memory::megabyte);
                parts.push_back(std::format("{}mb", mb));
    
                if (bytes == 0) { goto result; }
            
            case Memory::KILOBYTES:
                kb = bytes / Memory::kilobyte;
                bytes -= (kb * Memory::kilobyte);
                parts.push_back(std::format("{}kb", kb));

                if (bytes == 0) { goto result; }

            default:
                parts.push_back(std::format("{}b", bytes));
            }

        result:
            return strings::join(parts, " + ");
        }
    }

    std::string Memory::string() const {
        if (bytes == 0) {
            return "0b";
        }

        for (int fmt = LIMIT - 1; fmt >= 0; fmt--) {
            size_t size = sizes[fmt];
            if (bytes > size) { return format_memory(bytes, (Unit)fmt); }
        }
        
        return format_memory(bytes, BYTES);
    }
}