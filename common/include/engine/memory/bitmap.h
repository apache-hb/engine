#pragma once

#include "engine/base/container/unique.h"
#include <stdint.h>

namespace simcoe::memory {
    struct BitMap {
        BitMap(size_t size);

        size_t alloc(size_t count = 1);

        bool isSet(size_t bit) const;

    private:
        bool isRangeFree(size_t start, size_t length) const;

        void setRange(size_t start, size_t length);
        void setBit(size_t bit);

        size_t size;
        UniquePtr<uint64_t[]> bits;
    };

    template<typename T, T Empty>
    struct DebugBitMap {
        DebugBitMap(size_t size)
            : size(size)
            , used(0)
            , bits(new T[size])
        { 
            for (size_t i = 0; i < size; ++i) {
                bits[i] = Empty;
            }
        }

        size_t alloc(T kind, size_t count = 1) {
            for (size_t i = 0; i < size; i++) {
                if (isRangeFree(i, count)) {
                    setRange(i, count, kind);
                    return i;
                }
            }

            return SIZE_MAX;
        }

        bool isKind(size_t bit, T type) const {
            return bits[bit] == type;
        }

        T getBit(size_t bit) const {
            return bits[bit];
        }

        size_t getSize() const { return size; }
        size_t getUsed() const { return used; }

    private:
        bool isRangeFree(size_t start, size_t length) const {
            for (size_t i = 0; i < length; i++) {
                if (!isKind(start + i, Empty)) {
                    return false;
                }
            }

            return true;
        }

        void setRange(size_t start, size_t length, T kind) {
            for (size_t i = 0; i < length; i++) {
                setBit(start + i, kind);
            }

            used = (used > start + length) ? used : (start + length);
        }

        void setBit(size_t bit, T kind) {
            bits[bit] = kind;
        }

        size_t size;
        size_t used;
        UniquePtr<T[]> bits;
    };
}
