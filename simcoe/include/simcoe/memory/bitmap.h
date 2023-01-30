#pragma once

#include <memory>

namespace simcoe::memory {
    namespace detail {
        template<typename T, typename P>
        struct Storage {
            enum struct Index : size_t { eInvalid = SIZE_MAX };

            constexpr Storage(size_t bits): size(bits), pBits(new T[wordCount()]) { 
                reset();
            }

            constexpr size_t getSize() const { return size; }

            constexpr bool test(Index index) const { 
                return pBits[getWord(size_t(index))] & getMask(size_t(index)); 
            }

            constexpr Index alloc() {
                P *self = static_cast<P*>(this);
                for (size_t i = 0; i < getSize(); i++) {
                    if (self->testSet(i)) {
                        return Index(i);
                    }
                }

                return Index::eInvalid;
            }

            constexpr void release(Index index) {
                clear(size_t(index));
            }
        
            constexpr static inline size_t kBits = sizeof(T) * CHAR_BIT;

        protected:
            constexpr void set(size_t index) {
                pBits[getWord(index)] |= getMask(index);
            }

            constexpr void clear(size_t index) {
                pBits[getWord(index)] &= ~getMask(index);
            }

            constexpr void reset() {
                std::fill_n(pBits.get(), wordCount(), 0);
            }

        protected:
            constexpr T getMask(size_t bit) const { return T(1) << (bit % kBits); }
            constexpr size_t getWord(size_t bit) const { return bit / kBits; }
            constexpr size_t wordCount() const { return getSize() / kBits + 1; }

            size_t size;
            std::unique_ptr<T[]> pBits;
        };
    }

    struct BitMap final : detail::Storage<std::uint64_t, BitMap> {
        using Super = detail::Storage<std::uint64_t, BitMap>;
        using Super::Super;

        bool testSet(size_t index);
    };

    struct AtomicBitMap final : detail::Storage<std::atomic_uint64_t, AtomicBitMap> {
        using Super = detail::Storage<std::atomic_uint64_t, AtomicBitMap>;
        using Super::Super;

        bool testSet(size_t index);
    };
}
