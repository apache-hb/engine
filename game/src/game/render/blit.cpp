#include "game/render.h"

using namespace game;
using namespace simcoe;

namespace {
    constexpr float kClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    constexpr Vertex kScreenQuadVertices[] = {
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }
    };

    constexpr uint16_t kScreenQuadIndices[] = {
        0, 1, 2,
        0, 2, 3
    };
}

BlitPass::BlitPass(const GraphObject& object, Info& info) 
    : Pass(object, info) 
    , display(createLetterBoxDisplay(info.renderResolution, info.windowResolution))
{
    // create wires
    pSceneTargetIn = in<render::InEdge>("scene-target", D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);

    pSceneTargetOut = out<render::RelayEdge>("scene-target", pSceneTargetIn);
    pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);

    // load shader objects
    vs = info.assets.load("build\\game\\libgame.a.p\\blit.vs.cso");
    ps = info.assets.load("build\\game\\libgame.a.p\\blit.ps.cso");
}

void BlitPass::start(ID3D12GraphicsCommandList*) {
    auto& ctx = getContext();
    auto device = ctx.getDevice();

    // s0 is the sampler
    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_DESCRIPTOR_RANGE1 range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // t0 is the intermediate render target
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsDescriptorTable(1, &range);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        UINT(std::size(rootParameters)), (D3D12_ROOT_PARAMETER*)rootParameters, 
        UINT(std::size(samplers)), samplers, 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ID3DBlob *pSignature = nullptr;
    ID3DBlob *pError = nullptr;

    HR_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));
    HR_CHECK(device->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pBlitSignature)));

    RELEASE(pSignature);
    RELEASE(pError);

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = pBlitSignature,
        .VS = getShader(vs),
        .PS = getShader(ps),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(),
        .InputLayout = { layout, UINT(std::size(layout)) },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .SampleDesc = { 1, 0 },
    };

    HR_CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pBlitPipeline)));

    // upload fullscreen quad vertex and index data
    D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    pVertexBuffer = ctx.newBuffer(
        sizeof(kScreenQuadVertices), 
        &props,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );

    pIndexBuffer = ctx.newBuffer(
        sizeof(kScreenQuadIndices),
        &props,
        D3D12_RESOURCE_STATE_INDEX_BUFFER
    );

    uploadData(pVertexBuffer, sizeof(kScreenQuadVertices), kScreenQuadVertices);
    uploadData(pIndexBuffer, sizeof(kScreenQuadIndices), kScreenQuadIndices);

    vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(kScreenQuadVertices);

    indexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    indexBufferView.SizeInBytes = sizeof(kScreenQuadIndices);
}

void BlitPass::stop() {
    RELEASE(pBlitSignature);
    RELEASE(pBlitPipeline);

    RELEASE(pVertexBuffer);
    RELEASE(pIndexBuffer);
}

void BlitPass::execute(ID3D12GraphicsCommandList *pCommands) {
    auto rtv = pRenderTargetIn->cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    pCommands->RSSetViewports(1, &display.viewport);
    pCommands->RSSetScissorRects(1, &display.scissor);
    
    pCommands->OMSetRenderTargets(1, &rtv, false, nullptr);
    pCommands->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);

    pCommands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommands->IASetVertexBuffers(0, 1, &vertexBufferView);
    pCommands->IASetIndexBuffer(&indexBufferView);

    pCommands->SetPipelineState(pBlitPipeline);
    pCommands->SetGraphicsRootSignature(pBlitSignature);

    pCommands->SetGraphicsRootDescriptorTable(0, pSceneTargetIn->gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    pCommands->DrawIndexedInstanced(UINT(std::size(kScreenQuadIndices)), 1, 0, 0, 0);
}
