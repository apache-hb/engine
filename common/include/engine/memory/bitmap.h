#pragma once

#include "engine/base/container/unique.h"
#include <stdint.h>

namespace simcoe::memory {
    namespace detail {
        template<typename T>
        struct Storage {
            constexpr Storage(size_t bits): size(bits), data(new T[wordCount()]) { 
                reset();
            }

            constexpr size_t getSize() const { return size; }

        protected:
            const size_t size;
            UniquePtr<T[]> data;

            constexpr T getMask(size_t bit) const { return T(1) << (bit % kBits); }
            constexpr size_t getWord(size_t bit) const { return bit / kBits; }

        private:
            constexpr static inline size_t kBits = sizeof(T) * CHAR_BIT;

            constexpr size_t wordCount() const { 
                return getSize() / kBits + 1;
            }

            void reset() {
                std::fill_n(data.get(), wordCount(), 0);
            }
        };
    }

    struct BitMap final : detail::Storage<uint64_t> {
        using Super = detail::Storage<uint64_t>;
        using Super::Super;

        BitMap(size_t size);

        size_t alloc(size_t count = 1);
        void release(size_t index, size_t count = 1);

        bool testBit(size_t bit) const;

    private:
        bool isRangeFree(size_t start, size_t length) const;

        void setRange(size_t start, size_t length);
        void setBit(size_t bit);
        void clearBit(size_t bit);
    };

    struct AtomicBitMap final : detail::Storage<std::atomic_uint64_t> {
        using Super = detail::Storage<std::atomic_uint64_t>;
        using Super::Super;

        AtomicBitMap(size_t size);

        size_t alloc();
        void release(size_t index);

        bool testBit(size_t bit) const;

    private:
        void setBit(size_t bit);
        void clearBit(size_t bit);

        // set a bit and return true if it was previously clear
        bool testSetBit(size_t bit);
    };
}
