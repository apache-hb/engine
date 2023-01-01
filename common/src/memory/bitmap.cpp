#include "engine/memory/bitmap.h"

using namespace simcoe::memory;

BitMap::BitMap(size_t bits) : Super(bits) { }

size_t BitMap::alloc(size_t count) {
    ASSERT(count > 0);
    
    for (size_t i = 0; i < size; i++) {
        if (isRangeFree(i, count)) {
            setRange(i, count);
            return i;
        }
    }
    return SIZE_MAX;
}

bool BitMap::isRangeFree(size_t start, size_t length) const {
    for (size_t i = 0; i < length; i++) {
        if (testBit(start + i)) {
            return false;
        }
    }
    return true;
}

void BitMap::setRange(size_t start, size_t length) {
    for (size_t i = 0; i < length; i++) {
        setBit(start + i);
    }
}

bool BitMap::testBit(size_t bit) const {
    ASSERT(bit < size);

    return data[getWord(bit)] & getMask(bit);
}

void BitMap::setBit(size_t bit) {
    ASSERT(bit < size);

    data[getWord(bit)] |= getMask(bit);
}

AtomicBitMap::AtomicBitMap(size_t bits) : Super(bits) { }

size_t AtomicBitMap::alloc() {
    for (size_t i = 0; i < size; i++) {
        if (testSetBit(i)) {
            return i;
        }
    }
    return SIZE_MAX;
}

void AtomicBitMap::release(size_t index) {
    ASSERT(testBit(index));
    clearBit(index);
}

bool AtomicBitMap::testBit(size_t bit) const {
    ASSERT(bit < size);

    return data[getWord(bit)] & getMask(bit);
}

void AtomicBitMap::setBit(size_t bit) {
    ASSERT(bit < size);

    data[getWord(bit)] |= getMask(bit);
}

void AtomicBitMap::clearBit(size_t bit) {
    ASSERT(bit < size);

    data[getWord(bit)] &= ~getMask(bit);
}

bool AtomicBitMap::testSetBit(size_t bit) {
    ASSERT(bit < size);

    const uint64_t mask = getMask(bit);

    return data[getWord(bit)].fetch_or(mask) & mask;
}
