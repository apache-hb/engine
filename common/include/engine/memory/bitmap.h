#pragma once

#include "engine/container/unique.h"
#include <stdint.h>

namespace simcoe::memory {
    struct BitMap {
        BitMap(size_t size)
            : size(size)
            , bits(new uint64_t[size / 64 + 1])
        { }

        size_t alloc(size_t count = 1) {
            for (size_t i = 0; i < size; i += 64) {
                if (isRangeFree(i, count)) {
                    setRange(i, count);
                    return i;
                }
            }
            return SIZE_MAX;
        }
    private:
        bool isRangeFree(size_t start, size_t length) const {
            for (size_t i = start; i < start + length; ++i) {
                if (bits[i / 64] & (1 << (i % 64))) {
                    return false;
                }
            }
            return true;
        }

        void setRange(size_t start, size_t length) {
            for (size_t i = start; i < start + length; ++i) {
                bits[i / 64] |= (1 << (i % 64));
            }
        }

        size_t size;
        UniquePtr<uint64_t[]> bits;
    };
}
