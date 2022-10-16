#pragma once

#include "util.h"

namespace simcoe::memory {
    struct GpuPool {
        GpuPool(size_t size)
            : slots(size)
        { }

        size_t alloc(size_t count = 1) {
            for (size_t i = 0; i < slots.getSize(); i++) {
                if (hasFreeRange(i, count)) {
                    setRange(i, count);
                    return i;
                }
            }

            return SIZE_MAX;
        }
        
    private:
        bool hasFreeRange(size_t start, size_t length) const {
            for (size_t i = start; i < start + length; i++) {
                if (slots.query(i)) {
                    return false;
                }
            }
            return true;
        }

        void setRange(size_t start, size_t length) {
            for (size_t i = start; i < start + length; i++) {
                slots.set(i);
            }
        }

        BitSet slots;
    };
}
