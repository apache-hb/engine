#pragma once

#include "engine/render/objects/queue.h"
#include "engine/render/objects/fence.h"

namespace engine::render::d3d12 {
    template<typename T>
    concept IsDevice = std::is_base_of_v<ID3D12Device, T>;

    template<IsDevice T = ID3D12Device>
    struct Device : Object<T> {
        using Super = Object<T>;
        using Super::Super;

        template<IsCommandQueue O = ID3D12CommandQueue>
        CommandQueue<O> newCommandQueue(const D3D12_COMMAND_QUEUE_DESC &desc) {
            CommandQueue<O> queue;
            CHECK(Super::get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
            return queue;
        }

        template<IsFence O = ID3D12Fence>
        Fence<O> newFence() {
            O *fence;
            CHECK(Super::get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
            return fence;
        }
    };
}
