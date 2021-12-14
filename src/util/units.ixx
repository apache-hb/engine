module;

export module engine.util.units;

import engine.util.strings;
import std.core;

export namespace engine {
    struct Memory {
        enum Unit {
            BYTES,
            KILOBYTES,
            MEGABYTES,
            GIGABYTES,
            LIMIT
        };

        static constexpr size_t byte = 1;
        static constexpr size_t kilobyte = byte * 0x1000;
        static constexpr size_t megabyte = kilobyte * 0x1000;
        static constexpr size_t gigabyte = megabyte * 0x1000;

        static constexpr size_t sizes[LIMIT] = { byte, kilobyte, megabyte, gigabyte };

        constexpr Memory(size_t memory, Unit unit = BYTES) : bytes(memory * sizes[unit]) { }

        constexpr size_t b() const { return bytes; }
        constexpr size_t kb() const { return bytes / kilobyte; }
        constexpr size_t mb() const { return bytes / megabyte; }
        constexpr size_t gb() const { return bytes / gigabyte; }
    
        std::string string() const {
            if (bytes == 0) {
                return "0b";
            }

            for (int fmt = LIMIT - 1; fmt >= 0; fmt--) {
                size_t size = sizes[fmt];
                if (bytes > size) { return format_memory(Unit(fmt)); }
            }
            
            return format_memory(BYTES);
        }

    private:
        std::string format_memory(Memory::Unit most) const {
            size_t total = bytes, gb = SIZE_MAX, mb = SIZE_MAX, kb = SIZE_MAX;
            std::vector<std::string> parts;
            
            switch (most) {
            case Memory::GIGABYTES:
                gb = total / Memory::gigabyte;
                total -= (gb * Memory::gigabyte);
                parts.push_back(std::format("{}gb", gb));

                if (total == 0) { goto result; }

            case Memory::MEGABYTES:
                mb = total / Memory::megabyte;
                total -= (mb * Memory::megabyte);
                parts.push_back(std::format("{}mb", mb));

                if (total == 0) { goto result; }
            
            case Memory::KILOBYTES:
                kb = total / Memory::kilobyte;
                total -= (kb * Memory::kilobyte);
                parts.push_back(std::format("{}kb", kb));

                if (total == 0) { goto result; }

            default:
                parts.push_back(std::format("{}b", total));
            }

        result:
            return strings::join(parts, " + ");
        }

        size_t bytes;
    };
}
