#pragma once

#include "common.h"

namespace engine {
    struct DxPipelineState final : rhi::PipelineState {
        DxPipelineState(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline);

        ID3D12RootSignature *getSignature();
        ID3D12PipelineState *getPipelineState();
    private:
        rhi::UniqueComPtr<ID3D12RootSignature> signature;
        rhi::UniqueComPtr<ID3D12PipelineState> pipeline;
    };
}
