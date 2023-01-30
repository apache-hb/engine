#pragma once

#include <memory>

namespace simcoe::memory {
    namespace detail {
        template<typename T, T Empty>
        struct Storage {
            constexpr Storage(size_t bits): size(bits), pSlots(new T[wordCount()]) { 
                reset();
            }

            constexpr size_t getSize() const { return size; }

        protected:
            constexpr void reset() {
                std::fill_n(pSlots.get(), getSize(), Empty);
            }
            
            size_t size;
            std::unique_ptr<T[]> pSlots;
        };
    }

    template<typename T, T Empty = T()>
    struct SlotMap final : detail::Storage<T, Empty> {
        using Super = detail::Storage<T, Empty>;
        using Super::Super;

        enum struct Index : size_t { eInvalid = SIZE_MAX };

        SlotMap(size_t size) : Super(size) { }

        Index alloc(const T& value) {
            for (size_t i = 0; i < Super::getSize(); ++i) {
                if (Super::pSlots[i] == Empty) {
                    Super::pSlots[i] = value;
                    return Index(i);
                }
            }

            return Index::eInvalid;
        }

        void release(Index index) {
            ASSERT(get(index) != Empty);
            set(index, Empty);
        }

        bool test(Index index, const T& expected) const {
            return Super::pSlots[size_t(index)] == expected;
        }

        T get(Index index) const {
            ASSERT(index != Index::eInvalid);

            return Super::pSlots[size_t(index)];
        }

        void set(Index index, const T& value) {
            ASSERT(index != Index::eInvalid);

            Super::pSlots[size_t(index)] = value;
        }
    };
}