#pragma once

#include <memory>
namespace simcoe::memory {
    namespace detail {
        template<typename T>
        struct Storage final {
            Storage(size_t size): size(size), pBits(new T[size]) { }

            size_t getSize() const { return size; }
        private:
            size_t size;
            std::unique_ptr<T[]> pBits;
        };
    }
    
    struct BitMap final {
        BitMap(size_t size);

    private:
        detail::Storage<uint64_t> storage;
    };
}
