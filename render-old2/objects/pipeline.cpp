#include "pipeline.h"

namespace engine::render {
    PipelineState::PipelineState(ID3D12Device* device, ShaderLibrary shaders, RootSignature root) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
            .pRootSignature = root.get(),
            .VS = shaders.vertex(),
            .PS = shaders.pixel(),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = shaders.layout(),
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .DSVFormat = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = { 1 }
        };

        check(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(addr())), "failed to create graphics pipeline state");
    }
}
