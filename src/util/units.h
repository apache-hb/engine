#pragma once

#include <stddef.h>
#include <string>

namespace engine::units {
    struct Memory {
        enum Unit {
            BYTES,
            KILOBYTES,
            MEGABYTES,
            GIGABYTES,
            LIMIT
        };

        static constexpr size_t byte = 1;
        static constexpr size_t kilobyte = byte * 1024;
        static constexpr size_t megabyte = kilobyte * 1024;
        static constexpr size_t gigabyte = megabyte * 1024;

        static constexpr size_t kSizes[LIMIT] = { byte, kilobyte, megabyte, gigabyte };

        constexpr Memory(size_t memory = 0, Unit unit = BYTES) : bytes(memory * kSizes[unit]) { }

        constexpr size_t b() const { return bytes; }
        constexpr size_t kb() const { return bytes / kilobyte; }
        constexpr size_t mb() const { return bytes / megabyte; }
        constexpr size_t gb() const { return bytes / gigabyte; }
    
        std::string string() const;
        
    private:
        size_t bytes;
    };
}
