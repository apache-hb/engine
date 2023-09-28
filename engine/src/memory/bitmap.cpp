#include "simcoe/memory/bitmap.h"

#include "simcoe/core/panic.h"

using namespace simcoe;
using namespace simcoe::memory;

// BitMap

bool BitMap::testSet(size_t index) {
    if (test(Index(index))) {
        return false;
    }

    set(index);
    return true;
}

// AtomicBitMap

bool AtomicBitMap::testSet(size_t index) {
    auto mask = getMask(index);

    return !(pBits[getWord(index)].fetch_or(mask) & mask);
}
