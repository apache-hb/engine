#pragma once

#include "engine/base/panic.h"
#include <type_traits>

namespace simcoe {
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

        operator bool() const { return valid(); }
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
}
