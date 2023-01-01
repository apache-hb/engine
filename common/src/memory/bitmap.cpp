#include "engine/memory/bitmap.h"

using namespace simcoe::memory;

namespace {
    // TODO: this assumes 64-bit elements
    constexpr size_t wordCount(size_t bits) {
        return bits / 64 + 1;
    }
}

BitMap::BitMap(size_t size)
    : size(size)
    , bits(new uint64_t[wordCount(size)])
{ 
    std::fill_n(bits.get(), wordCount(size), 0);
}

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

    return bits[bit / 64] & (1ull << (bit % 64));
}

void BitMap::setBit(size_t bit) {
    ASSERT(bit < size);

    bits[bit / 64] |= (1ull << (bit % 64));
}

AtomicBitMap::AtomicBitMap(size_t size)
    : size(size)
    , bits(new std::atomic_uint64_t[wordCount(size)])
{ 
    std::fill_n(bits.get(), wordCount(size), 0);
}

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

    return bits[bit / 64] & (1ull << (bit % 64));
}

void AtomicBitMap::setBit(size_t bit) {
    ASSERT(bit < size);

    bits[bit / 64] |= (1ull << (bit % 64));
}

void AtomicBitMap::clearBit(size_t bit) {
    ASSERT(bit < size);

    bits[bit / 64] &= ~(1ull << (bit % 64));
}

bool AtomicBitMap::testSetBit(size_t bit) {
    ASSERT(bit < size);

    const uint64_t mask = 1ull << (bit % 64);

    return bits[bit / 64].fetch_or(mask) & mask;
}
