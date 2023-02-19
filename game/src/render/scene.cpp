#include "game/render.h"

using namespace game;
using namespace simcoe;

namespace {
    constexpr float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };
}

ScenePass::ScenePass(const GraphObject& object, const system::Size& size) 
    : Pass(object)
    , size(size)
{
    pRenderTargetOut = out<SourceEdge>("scene-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void ScenePass::start() {
    auto& ctx = getContext();
    auto device = ctx.getDevice();
    auto& rtvHeap = ctx.getRtvHeap();
    auto& cbvHeap = ctx.getCbvHeap();

    rtvIndex = rtvHeap.alloc();
    cbvIndex = cbvHeap.alloc();

    ID3D12Resource *pResource = nullptr;

    D3D12_CLEAR_VALUE clear = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, kClearColour);

    // create intermediate render target
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ DXGI_FORMAT_R8G8B8A8_UNORM,
        /* width = */ UINT(size.width),
        /* height = */ UINT(size.height),
        /* arraySize = */ 1,
        /* mipLevels = */ 1,
        /* sampleCount = */ 1,
        /* sampleQuality = */ 0,
        /* flags = */ D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    HR_CHECK(device->CreateCommittedResource(
        /* heapProperties = */ &props,
        /* heapFlags = */ D3D12_HEAP_FLAG_NONE,
        /* pDesc = */ &desc,
        /* initialState = */ D3D12_RESOURCE_STATE_RENDER_TARGET,
        /* pOptimizedClearValue = */ &clear,
        /* riidResource = */ IID_PPV_ARGS(&pResource)
    ));

    // create render target view for the intermediate target
    device->CreateRenderTargetView(pResource, nullptr, rtvHeap.cpuHandle(rtvIndex));

    // create shader resource view of the intermediate render target
    device->CreateShaderResourceView(pResource, nullptr, cbvHeap.cpuHandle(cbvIndex));

    pRenderTargetOut->setResource(pResource, cbvHeap.cpuHandle(cbvIndex), cbvHeap.gpuHandle(cbvIndex));
}

void ScenePass::stop() {
    auto& ctx = getContext();
    
    ctx.getCbvHeap().release(cbvIndex);
    ctx.getRtvHeap().release(rtvIndex);

    ID3D12Resource *pResource = pRenderTargetOut->getResource();
    RELEASE(pResource);

    pRenderTargetOut->setResource(nullptr, { }, { });
}

void ScenePass::execute() {
    auto& ctx = getContext();
    auto& rtvHeap = ctx.getRtvHeap();
    auto cmd = getContext().getCommandList();
    auto rtv = rtvHeap.cpuHandle(rtvIndex); // TODO: definetly not right
    Display display = createDisplay(size);

    cmd->RSSetViewports(1, &display.viewport);
    cmd->RSSetScissorRects(1, &display.scissor);

    cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
    cmd->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);
}

