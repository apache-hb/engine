#pragma once

#include "engine/base/container/unique.h"
#include <unknwnbase.h>

namespace simcoe::com {
    template<typename T>
    concept IsComObject = std::is_base_of_v<IUnknown, T>;
    
    template<IsComObject T>
    struct ComDeleter {
        void operator()(T *ptr) const {
            ptr->Release();
        }
    };

    template<IsComObject T>
    struct UniqueComPtr : UniquePtr<T, ComDeleter<T>> {
        using Super = UniquePtr<T, ComDeleter<T>>;
        using Super::Super;

        void release() {
            Super::destroy();
        }
    };
}
