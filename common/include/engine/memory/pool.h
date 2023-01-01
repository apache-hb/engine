#pragma once

#include "engine/memory/bitmap.h"

namespace simcoe::memory {
    template<typename T>
    struct AtomicPool {
         
    private:
        AtomicBitMap alloc;
        UniquePtr<T[]> data;
    };
}