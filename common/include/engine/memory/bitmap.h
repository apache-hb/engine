#pragma once

#include "engine/base/container/unique.h"
#include <stdint.h>

// TODO: extract logic to common base class

namespace simcoe::memory {
    struct BitMap final {
        BitMap(size_t size);

        size_t alloc(size_t count = 1);
        void release(size_t index, size_t count = 1);

        bool testBit(size_t bit) const;

    private:
        bool isRangeFree(size_t start, size_t length) const;

        void setRange(size_t start, size_t length);
        void setBit(size_t bit);
        void clearBit(size_t bit);

        const size_t size;
        UniquePtr<uint64_t[]> bits;
    };

    struct AtomicBitMap final {
        AtomicBitMap(size_t size);

        size_t alloc();
        void release(size_t index);

        bool testBit(size_t bit) const;

    private:
        void setBit(size_t bit);
        void clearBit(size_t bit);

        // set a bit and return true if it was previously clear
        bool testSetBit(size_t bit);

        const size_t size;
        UniquePtr<std::atomic_uint64_t[]> bits;
    };
}
