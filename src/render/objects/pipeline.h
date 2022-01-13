#pragma once

#include "render/util.h"

namespace engine::render {
    struct PipelineState : Object<ID3D12PipelineState> {
        OBJECT(PipelineState, ID3D12PipelineState);
    };
}
