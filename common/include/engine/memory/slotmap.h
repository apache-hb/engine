#pragma once

#include "engine/base/container/unique.h"
#include <algorithm>
#include <atomic>

namespace simcoe::memory {
    template<typename T, T Empty>
    struct SlotMap {
        SlotMap(size_t size)
            : used(0)
            , size(size)
            , bits(new T[size])
        { 
            for (size_t i = 0; i < size; ++i) {
                bits[i] = Empty;
            }
        }

        size_t alloc(T kind, size_t count = 1) {
            for (size_t i = 0; i < getSize(); i++) {
                if (isRangeFree(i, count)) {
                    setRange(i, count, kind);
                    return i;
                }
            }

            return SIZE_MAX;
        }

        void release(T kind, size_t index, size_t count = 1) {
            for (size_t i = 0; i < count; i++) {
                ASSERT(testBit(index + i, kind));
                setBit(index + i, Empty);
            }
        }

        void update(size_t index, T kind) {
            setBit(index, kind);
        }

        bool testBit(size_t bit, T type) const {
            return getBit(bit) == type;
        }

        T getBit(size_t bit) const {
            ASSERT(bit < getSize());
            return bits[bit];
        }

        size_t getSize() const { return size; }
        size_t getUsed() const { return used; }

    private:
        bool isRangeFree(size_t start, size_t length) const {
            for (size_t i = 0; i < length; i++) {
                if (!testBit(start + i, Empty)) {
                    return false;
                }
            }

            return true;
        }

        void setRange(size_t start, size_t length, T kind) {
            for (size_t i = 0; i < length; i++) {
                setBit(start + i, kind);
            }

            used = std::max(used, start + length);
        }

        void setBit(size_t bit, T kind) {
            ASSERT(bit < getSize());
            bits[bit] = kind;
        }

        size_t used; // highest used bit
        const size_t size; // total size of the bits
        UniquePtr<T[]> bits;
    };

    template<typename T, T Empty>
    struct AtomicSlotMap {
        using AtomicType = std::atomic<T>;

        AtomicSlotMap(size_t size)
            : used(0)
            , size(size)
            , bits(new AtomicType[size])
        { 
            for (size_t i = 0; i < size; ++i) {
                bits[i] = Empty;
            }
        }

        size_t alloc(T kind) {
            for (size_t i = 0; i < getSize(); i++) {
                T expected = Empty;
                if (bits[i].compare_exchange_strong(expected, kind)) {
                    used = std::max(used.load(), i + 1);
                    return i;
                }
            }
            return SIZE_MAX;
        }

        void release(T kind, size_t index) {
            ASSERT(bits[index].compare_exchange_strong(kind, Empty));
            used = std::min(used.load(), index);
        }

        bool testBit(size_t bit, T type) const {
            return getBit(bit) == type;
        }

        T getBit(size_t bit) const {
            ASSERT(bit < getSize());
            return bits[bit];
        }

        void update(size_t index, T kind) {
            bits[index] = kind;
        }

        size_t getSize() const { return size; }
        size_t getUsed() const { return used; }

    private:
        std::atomic_size_t used;
        const size_t size;
        UniquePtr<AtomicType[]> bits;
    };
}
