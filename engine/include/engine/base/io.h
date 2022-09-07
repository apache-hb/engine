#pragma once

namespace engine {
    struct Io {
        enum Mode {
            eRead,
            eWrite
        };

        enum Format {
            eText,
            eBinary
        };

        Io(std::string_view name, Mode mode, Format format);

        virtual size_t read(void *dst, size_t size) = 0;
        virtual size_t write(const void *src, size_t size) = 0;
    };

    struct File : Io {
        File(std::string_view name);
        
        size_t read(void *dst, size_t size) override;
        size_t write(const void *src, size_t size) override;
    };

    struct Memory : Io {
        size_t read(void *dst, size_t size) override;
        size_t write(const void *src, size_t size) override;
    };

    struct View : Io {
        size_t read(void *dst, size_t size) override;
        size_t write(const void *src, size_t size) override;
    };
}
