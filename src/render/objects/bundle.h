#pragma once

#include "render/util.h"

namespace engine::render {
    struct CommandBundle : Object<ID3D12GraphicsCommandList> {
        using Super = Object<ID3D12GraphicsCommandList>;

        CommandBundle() = default;
        CommandBundle(Super super, Object<ID3D12CommandAllocator> allocator)
            : Super(super)
            , allocator(allocator)
        { }

        void tryDrop(std::string_view name);
        
    private:
        Object<ID3D12CommandAllocator> allocator;
    };
}
