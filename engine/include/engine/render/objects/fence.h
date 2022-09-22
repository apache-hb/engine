#pragma once

#include "engine/render/util.h"

namespace engine::render::d3d12 {
    template<typename T>
    concept IsFence = std::is_base_of_v<ID3D12Fence, T>;

    template<IsFence T = ID3D12Fence>
    struct Fence : Object<T> {
        using Super = Object<T>;
        using Super::Super;

        Fence() : Super(nullptr), event(nullptr) { }

        Fence(T *fence) : Super(fence) {
            ASSERT((event = CreateEvent(nullptr, false, false, nullptr)) != nullptr);
        }

        void waitUntil(UINT64 value) {
            if (Super::get()->GetCompletedValue() < value) {
                CHECK(Super::get()->SetEventOnCompletion(value, event));
                WaitForSingleObject(event, INFINITE);
            }
        }

    private:
        HANDLE event;
    };
}
