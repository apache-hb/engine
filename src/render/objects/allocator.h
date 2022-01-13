#pragma once

#include "render/util.h"

namespace engine::render {
    struct Allocator : Object<ID3D12CommandAllocator> {
        OBJECT(Allocator, ID3D12CommandAllocator);
    };
}
