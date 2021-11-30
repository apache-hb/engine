#pragma once

#include <stddef.h>
#include <string>

namespace units {
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

        Memory(size_t bytes) : bytes(bytes) { }

        size_t b() const { return bytes; }
        size_t kb() const { return bytes / kilobyte; }
        size_t mb() const { return bytes / megabyte; }
        size_t gb() const { return bytes / gigabyte; }
    
        std::string string() const;
    private:
        size_t bytes;
    };
}
