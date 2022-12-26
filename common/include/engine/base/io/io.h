#pragma once

#include <string_view>
#include <vector>
#include <span>

namespace simcoe {
    struct Io {
        enum Mode {
            eRead = (1 << 0),
            eWrite = (1 << 1)
        };

        static Io *open(std::string_view name, Mode mode);

        Io(const Io&) = delete;
        virtual ~Io() = default;

        void write(std::string_view text);

        template<typename T>
        std::vector<T> read(size_t total = SIZE_MAX) {
            if (total == SIZE_MAX) {
                total = size() / sizeof(T);
            }
            std::vector<T> result{total};
            read((void*)result.data(), sizeof(T) * total);
            return result;
        }

        template<typename T>
        size_t write(std::span<T> data) {
            write((const void*)data.data(), data.bytes_size());
        }

        size_t read(void *dst, size_t size);
        size_t write(const void *src, size_t size);
        virtual size_t size() = 0;
        virtual bool valid() const = 0;

        std::string_view name;
        const Mode mode;

    protected:
        virtual size_t innerRead(void *dst, size_t size) = 0;
        virtual size_t innerWrite(const void *src, size_t size) = 0;

        Io(std::string_view name, Mode mode)
            : name(name)
            , mode(mode)
        { }
    };
}
