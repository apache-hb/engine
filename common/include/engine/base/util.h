#pragma once

#include "engine/base/panic.h"

#include <string>
#include <string_view>
#include <sstream>
#include <span>

namespace simcoe {
    namespace strings {
        std::string narrow(std::wstring_view str);
        std::wstring widen(std::string_view str);

        template<typename T>
        std::string join(std::span<T> items, std::string_view sep) {
            std::stringstream ss;
            for (size_t i = 0; i < items.size(); i++) {
                if (i != 0) {
                    ss << sep;
                }
                ss << items[i];
            }
            return ss.str();
        }
    }

    struct Timer {
        Timer();
        double tick();

    private:
        size_t last;
    };

    template<typename T>
    struct DefaultDelete {
        void operator()(T *ptr) {
            delete ptr;
        }
    };

    template<typename T>
    struct DefaultDelete<T[]> {
        void operator()(T *ptr) {
            delete[] ptr;
        }
    };

    template<typename T>
    struct RemoveArray {
        using Type = T;
    };

    template<typename T>
    struct RemoveArray<T[]> {
        using Type = T;
    };

    template<typename T, T Invalid, typename Delete> requires (std::is_trivially_copy_constructible<T>::value)
    struct UniqueResource {
        UniqueResource(T object = Invalid) { 
            init(object);
        }

        UniqueResource(UniqueResource &&other) { 
            init(other.claim());
        }

        UniqueResource &operator=(UniqueResource &&other) {
            destroy();
            init(other.claim());
            return *this;
        }

        UniqueResource(const UniqueResource&) = delete;
        UniqueResource &operator=(const UniqueResource&) = delete;

        ~UniqueResource() {
            destroy();
        }

        bool valid() const { return data != Invalid; }

        T get() { ASSERT(valid()); return data; }
        const T get() const { ASSERT(valid()); return data; }

        T *ref() { return &data; }
        const T* ref() const { return &data; }

    protected:
        void destroy() {
            if (valid()) {
                kDelete(data);
            }
            data = Invalid;
        }

    private:
        T claim() {
            T out = data;
            data = Invalid;
            return out;
        }

        void init(T it) {
            data = it;
        }

        T data = Invalid;
        static inline Delete kDelete{};
    };

    template<typename T, typename Delete = DefaultDelete<T>>
    struct UniquePtr : public UniqueResource<typename RemoveArray<T>::Type*, nullptr, Delete> {
        using Super = UniqueResource<typename RemoveArray<T>::Type*, nullptr, Delete>;
        using Super::Super;

        using Type = typename RemoveArray<T>::Type;

        UniquePtr(size_t size) requires (std::is_array_v<T>) 
            : Super(new Type[size]) 
        {}
        
        Type &operator[](size_t index) requires(std::is_array_v<T>) {
            return Super::get()[index];
        }

        const Type &operator[](size_t index) const requires(std::is_array_v<T>) {
            return Super::get()[index];
        }

        Type *operator->() { return Super::get(); }
        const Type *operator->() const { return Super::get(); }

        Type **operator&() { return Super::ref(); }
        const Type**operator&() const { return Super::ref(); }
    };

    struct BitSet {
        BitSet(size_t size)
            : size(size)
            , bits(size / sizeof(uint8_t))
        {}
        
        void set(size_t bit) {
            get(bit) |= (bit % 8);
        }

        void clear(size_t bit) {
            get(bit) &= ~(bit % 8);
        }

        bool query(size_t bit) const {
            return get(bit) & (bit % 8);
        }

        size_t getSize() const {
            return size;
        }
    private:
        uint8_t &get(size_t cell) {
            ASSERT(cell <= size);
            return bits[(cell / sizeof(uint8_t))];
        }

        uint8_t get(size_t cell) const {
            ASSERT(cell <= size);
            return bits[(cell / sizeof(uint8_t))];
        }
        
        size_t size;
        UniquePtr<uint8_t[]> bits;
    };
}
