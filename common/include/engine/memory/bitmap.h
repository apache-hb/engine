#pragma once

#include "engine/base/container/unique.h"
#include <stdint.h>

namespace simcoe::memory {
    struct BitMap {
        BitMap(size_t size);

        size_t alloc(size_t count = 1);
        void release(size_t index, size_t count = 1);

        bool testBit(size_t bit) const;

    private:
        bool isRangeFree(size_t start, size_t length) const;

        void setRange(size_t start, size_t length);
        void setBit(size_t bit);
        void clearBit(size_t bit);

        size_t size;
        UniquePtr<uint64_t[]> bits;
    };
}
