#include "game/render.h"

using namespace game;
using namespace simcoe;

namespace {
    constexpr float kClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

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

BlitPass::BlitPass(const GraphObject& object, const Display& display) 
    : Pass(object) 
    , display(display)
{
    // create wires
    pSceneTargetIn = in<render::InEdge>("scene-target", D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);

    pSceneTargetOut = out<render::RelayEdge>("scene-target", pSceneTargetIn);
    pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);

    // load shader objects
    vs = loadShader("build\\game\\game.exe.p\\post.vs.cso");
    ps = loadShader("build\\game\\game.exe.p\\post.ps.cso");
}

void BlitPass::start() {
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
    rootSignatureDesc.Init(_countof(rootParameters), (D3D12_ROOT_PARAMETER*)rootParameters, _countof(samplers), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
        .InputLayout = { layout, _countof(layout) },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .SampleDesc = { 1, 0 },
    };

    HR_CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pBlitPipeline)));

    // upload fullscreen quad vertex and index data
    D3D12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(kScreenQuadVertices));
    D3D12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(kScreenQuadIndices));

    D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    HR_CHECK(device->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        nullptr,
        IID_PPV_ARGS(&pVertexBuffer)
    ));

    HR_CHECK(device->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        nullptr,
        IID_PPV_ARGS(&pIndexBuffer)
    ));

    void *pVertexData = nullptr;
    void *pIndexData = nullptr;

    HR_CHECK(pVertexBuffer->Map(0, nullptr, &pVertexData));
    memcpy(pVertexData, kScreenQuadVertices, sizeof(kScreenQuadVertices));
    
    HR_CHECK(pIndexBuffer->Map(0, nullptr, &pIndexData));
    memcpy(pIndexData, kScreenQuadIndices, sizeof(kScreenQuadIndices));

    pVertexBuffer->Unmap(0, nullptr);
    pIndexBuffer->Unmap(0, nullptr);

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

void BlitPass::execute() {
    auto& ctx = getContext();
    auto cmd = ctx.getCommandList();
    auto rtv = pRenderTargetIn->cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    cmd->RSSetViewports(1, &display.viewport);
    cmd->RSSetScissorRects(1, &display.scissor);
    
    cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
    cmd->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &vertexBufferView);
    cmd->IASetIndexBuffer(&indexBufferView);

    cmd->SetPipelineState(pBlitPipeline);
    cmd->SetGraphicsRootSignature(pBlitSignature);

    cmd->SetGraphicsRootDescriptorTable(0, pSceneTargetIn->gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    cmd->DrawIndexedInstanced(_countof(kScreenQuadIndices), 1, 0, 0, 0);
}
