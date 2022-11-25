#pragma once

#include "engine/base/container/common.h"
#include "engine/base/container/resource.h"

namespace simcoe {
    template<typename T, typename Delete = DefaultDelete<T>>
    struct UniquePtr : public UniqueResource<typename RemoveArray<T>::Type*, nullptr, Delete> {
        using Super = UniqueResource<typename RemoveArray<T>::Type*, nullptr, Delete>;
        using Super::Super;

        using Type = typename RemoveArray<T>::Type;

        UniquePtr(size_t size) requires (std::is_array_v<T>) 
            : Super(new Type[size]) 
        { }

        [[nodiscard]] Type &operator[](size_t index) requires(std::is_array_v<T>) {
            return at(index);
        }

        [[nodiscard]] const Type &operator[](size_t index) const requires(std::is_array_v<T>) {
            return at(index);
        }

        [[nodiscard]] Type &at(size_t index) requires(std::is_array_v<T>) {
            return Super::get()[index];
        }

        [[nodiscard]] const Type &at(size_t index) const requires(std::is_array_v<T>) {
            return Super::get()[index];
        }
        
        [[nodiscard]] Type *operator->() { return Super::get(); }
        [[nodiscard]] const Type *operator->() const { return Super::get(); }

        [[nodiscard]] Type **operator&() { return Super::ref(); }
        [[nodiscard]] const Type**operator&() const { return Super::ref(); }
    };
}
