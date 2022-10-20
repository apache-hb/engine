#pragma once

#include "engine/base/container/resource.h"

namespace simcoe {
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

}
