#include "dx/d3d12.h"
#include "game/render.h"
#include "game/registry.h"

#include "simcoe/core/units.h"

using namespace game;
using namespace simcoe;

using namespace simcoe::units;

namespace {
    const D3D12_HEAP_PROPERTIES kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_HEAP_PROPERTIES kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
}

ScenePass::ScenePass(const GraphObject& object, Info& info)
    : Pass(object)
    , info(info)
{
    pRenderTargetOut = out<IntermediateTargetEdge>("scene-target", info.renderResolution);

    vs = loadShader("build\\game\\libgame.a.p\\scene.vs.cso");
    ps = loadShader("build\\game\\libgame.a.p\\scene.ps.cso");

    debug = game::debug.newEntry({ "Scene" }, [info] {
        auto [width, height] = info.renderResolution;
        ImGui::Text("Resolution: %zu x %zu", width, height);
    });
}

void ScenePass::start(ID3D12GraphicsCommandList*) {
    pRenderTargetOut->start();

    auto& ctx = getContext();
    auto *pDevice = ctx.getDevice();
    auto& cbvHeap = ctx.getCbvHeap();
    auto& dsvHeap = ctx.getDsvHeap();

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_DESCRIPTOR_RANGE1 textureRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParameters[4];
    // t0[] textures
    rootParameters[0].InitAsDescriptorTable(1, &textureRange);

    // b0 scene buffer
    rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

    // b1 material buffer
    rootParameters[2].InitAsConstants(1, 1, 0);

    // b2 node buffer
    rootParameters[3].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        UINT(std::size(rootParameters)), (D3D12_ROOT_PARAMETER*)rootParameters, 
        UINT(std::size(samplers)), samplers, 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ID3DBlob *pSignature = nullptr;
    ID3DBlob *pError = nullptr;

    HR_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));
    HR_CHECK(pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    RELEASE(pSignature);
    RELEASE(pError);

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = pRootSignature,
        .VS = getShader(vs),
        .PS = getShader(ps),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        .InputLayout = { layout, UINT(std::size(layout)) },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = { 1, 0 },
    };

    HR_CHECK(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState)));

    D3D12_RESOURCE_DESC cameraBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneBuffer));

    HR_CHECK(pDevice->CreateCommittedResource(
        &kUploadProps,
        D3D12_HEAP_FLAG_NONE,
        &cameraBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pSceneBuffer)
    ));

    // create depth stencil
    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        UINT(info.renderResolution.width),
        UINT(info.renderResolution.height),
        1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );
    
    D3D12_CLEAR_VALUE clearValue = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .DepthStencil = { 1.0f, 0 }
    };

    HR_CHECK(pDevice->CreateCommittedResource(
        &kDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&pDepthStencil)
    ));

    sceneHandle = cbvHeap.alloc();
    depthHandle = dsvHeap.alloc();

    D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {
        .BufferLocation = pSceneBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = sizeof(SceneBuffer)
    };

    D3D12_DEPTH_STENCIL_VIEW_DESC stencilDesc = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE
    };

    pDevice->CreateConstantBufferView(&bufferDesc, cbvHeap.cpuHandle(sceneHandle));
    pDevice->CreateDepthStencilView(pDepthStencil, &stencilDesc, dsvHeap.cpuHandle(depthHandle));

    CD3DX12_RANGE read(0, 0);
    HR_CHECK(pSceneBuffer->Map(0, &read, (void**)&pSceneData));
}

void ScenePass::stop() {
    auto& ctx = getContext();

    ctx.getDsvHeap().release(depthHandle);

    ctx.getCbvHeap().release(sceneHandle);
    pSceneData = nullptr;
    pSceneBuffer->Unmap(0, nullptr);
    RELEASE(pSceneBuffer);

    RELEASE(pPipelineState);
    RELEASE(pRootSignature);

    pRenderTargetOut->stop();
}

void ScenePass::execute(ID3D12GraphicsCommandList *pCommands) {
    pSceneData->mvp = info.pCamera->mvp(float4x4::identity(), info.renderResolution.aspectRatio<float>());

    auto& ctx = getContext();
    auto& cbvHeap = ctx.getCbvHeap();
    auto& dsvHeap = ctx.getDsvHeap();

    auto rtv = pRenderTargetOut->cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto dsv = dsvHeap.cpuHandle(depthHandle);

    Display display = createDisplay(info.renderResolution);

    pCommands->RSSetViewports(1, &display.viewport);
    pCommands->RSSetScissorRects(1, &display.scissor);

    pCommands->SetGraphicsRootSignature(pRootSignature);
    pCommands->SetPipelineState(pPipelineState);

    pCommands->OMSetRenderTargets(1, &rtv, false, &dsv);
    pCommands->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);
    pCommands->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    pCommands->SetGraphicsRootDescriptorTable(0, cbvHeap.gpuHandle());
    pCommands->SetGraphicsRootConstantBufferView(1, pSceneBuffer->GetGPUVirtualAddress());

    pCommands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
