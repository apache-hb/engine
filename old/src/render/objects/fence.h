#pragma once

#include "render/util.h"

namespace engine::render {
    struct Fence : Object<ID3D12Fence> {
        OBJECT(Fence, ID3D12Fence);

        using Value = UINT64;
    };
}
