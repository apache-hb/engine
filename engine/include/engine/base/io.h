#pragma once

#include <string_view>
#include <vector>
#include <span>

namespace engine {
    struct Io {
        enum Mode {
            eRead,
            eWrite
        };

        static Io *open(std::string_view name, Mode mode);

        Io() = delete;
        Io(const Io&) = delete;
        virtual ~Io() = default;

        void write(std::string_view text);

        template<typename T>
        std::vector<T> read(size_t total) {
            std::vector<T> result{total};
            read((void*)result.data(), sizeof(T) * total);
            return result;
        }

        template<typename T>
        size_t write(std::span<T> data) {
            write((const void*)data.data(), data.bytes_size());
        }

        virtual size_t read(void *dst, size_t size) = 0;
        virtual size_t write(const void *src, size_t size) = 0;

        std::string_view name;

    protected:
        Io(std::string_view name);
    };
}
