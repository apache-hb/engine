#include "engine/memory/bitmap.h"

using namespace simcoe::memory;

BitMap::BitMap(size_t size)
    : size(size)
    , bits(new uint64_t[size / 64 + 1])
{ 
    memset(bits.get(), 0, size / 64 + 1);
}

size_t BitMap::alloc(size_t count) {
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
        if (isSet(start + i)) {
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

bool BitMap::isSet(size_t bit) const {
    return bits[bit / 64] & (1ull << (bit % 64));
}

void BitMap::setBit(size_t bit) {
    bits[bit / 64] |= (1ull << (bit % 64));
}