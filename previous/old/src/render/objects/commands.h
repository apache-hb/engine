#pragma once

#include "fence.h"

namespace engine::render {
    struct CommandList : Object<ID3D12GraphicsCommandList> {
        OBJECT(CommandList, ID3D12GraphicsCommandList);
        
        using Type = D3D12_COMMAND_LIST_TYPE;
        static constexpr auto kDirect = D3D12_COMMAND_LIST_TYPE_DIRECT;
        static constexpr auto kBundle = D3D12_COMMAND_LIST_TYPE_BUNDLE;
        static constexpr auto kCompute = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        static constexpr auto kCopy = D3D12_COMMAND_LIST_TYPE_COPY;
    };
}
