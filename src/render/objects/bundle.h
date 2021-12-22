#pragma once

#include "render/util.h"

namespace engine::render {
    struct CommandBundle : Com<ID3D12GraphicsCommandList> {
        using Super = Com<ID3D12GraphicsCommandList>;

        CommandBundle() = default;
        CommandBundle(Super super, Com<ID3D12CommandAllocator> allocator)
            : Super(super)
            , allocator(allocator)
        { }

        void tryDrop(std::string_view name);
        
    private:
        Com<ID3D12CommandAllocator> allocator;
    };
}
