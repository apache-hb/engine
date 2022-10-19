#pragma once

#include "engine/container/unique.h"
#include <stdint.h>

namespace simcoe::memory {
    struct BitMap {
        BitMap(size_t size);

        size_t alloc(size_t count = 1);

    private:
        bool isRangeFree(size_t start, size_t length) const;
        void setRange(size_t start, size_t length);

        bool isSet(size_t bit) const;
        void setBit(size_t bit);

        size_t size;
        UniquePtr<uint64_t[]> bits;
    };
}
