#pragma once

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
}
