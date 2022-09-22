#pragma once

#include "engine/render/util.h"
#include <span>

namespace engine::render::d3d12 {
    template<typename T>
    concept IsCommandQueue = std::is_base_of_v<ID3D12CommandQueue, T>;

    template<IsCommandQueue T = ID3D12CommandQueue>
    struct CommandQueue : Object<T> {
        using Super = Object<T>;
        using Super::Super;

        void execute(std::span<ID3D12CommandList*> commands) {
            ASSERT(commands.size() <= UINT_MAX);

            Super::get()->ExecuteCommandLists(static_cast<UINT>(commands.size()), commands.data());
        }
    };
}
