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
            TERABYTES,
            LIMIT
        };

        static constexpr size_t byte = 1;
        static constexpr size_t kilobyte = byte * 1024;
        static constexpr size_t megabyte = kilobyte * 1024;
        static constexpr size_t gigabyte = megabyte * 1024;
        static constexpr size_t terabyte = gigabyte * 1024;

        static constexpr size_t kSizes[LIMIT] = { byte, kilobyte, megabyte, gigabyte, terabyte };
        
        static constexpr std::string_view kNames[LIMIT] = {
            "b", "kb", "mb", "gb", "tb"
        };

        constexpr Memory(size_t memory = 0, Unit unit = BYTES) : bytes(memory * kSizes[unit]) { }
        static constexpr Memory fromBytes(size_t bytes) { return Memory(bytes, BYTES); }
        static constexpr Memory fromKilobytes(size_t kilobytes) { return Memory(kilobytes, KILOBYTES); }
        static constexpr Memory fromMegabytes(size_t megabytes) { return Memory(megabytes, MEGABYTES); }
        static constexpr Memory fromGigabytes(size_t gigabytes) { return Memory(gigabytes, GIGABYTES); }
        static constexpr Memory fromTerabytes(size_t terabytes) { return Memory(terabytes, TERABYTES); }

        constexpr size_t b() const { return bytes; }
        constexpr size_t kb() const { return bytes / kilobyte; }
        constexpr size_t mb() const { return bytes / megabyte; }
        constexpr size_t gb() const { return bytes / gigabyte; }
        constexpr size_t tb() const { return bytes / terabyte; }
    
        std::string string() const;
        
    private:
        size_t bytes;
    };
}
