#pragma once

#include "commands.h"

namespace engine::render {
    struct Queue : Object<ID3D12CommandQueue> {
        using Super = Object<ID3D12CommandQueue>;
        
        Queue() = default;
        Queue(ID3D12Device* device, const D3D12_COMMAND_QUEUE_DESC& desc);

        void execute(std::span<Commands> commands);
    };
}
