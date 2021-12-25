#pragma once

#include "commands.h"

namespace engine::render {
    struct CommandBundle : Commands {
        using Super = Commands;
        using Super::Super;

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
