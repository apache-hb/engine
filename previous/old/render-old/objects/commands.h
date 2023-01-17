#pragma once

#include "render/util.h"

namespace engine::render {
    struct Commands : Object<ID3D12GraphicsCommandList> {
        using Super = Object<ID3D12GraphicsCommandList>;
        using Super::Super;
    };
}
