#pragma once

#include "root.h"
#include "library.h"

namespace engine::render {
    struct PipelineState : Object<ID3D12PipelineState> {
        using Super = Object<ID3D12PipelineState>;

        PipelineState() = default;
        PipelineState(ID3D12Device* device, ShaderLibrary shaders, RootSignature root);
    };
}
