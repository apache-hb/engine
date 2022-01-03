#include "scene.h"

#include "render/render.h"
#include "render/objects/commands.h"

using namespace engine::render;

constexpr auto kDepthFormat = DXGI_FORMAT_D32_FLOAT;
constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

void Scene3D::create() {
    createCommandList();
    createDsvHeap();
    createDepthStencil();
}

void Scene3D::destroy() {
    destroyDepthStencil();
    destroyDsvHeap();
    destroyCommandList();
}

ID3D12CommandList* Scene3D::populate() {
    auto& allocator = ctx->getAllocator(Allocator::Scene);
    const auto [scissor, viewport] = ctx->getSceneView();
    auto targetHandle = ctx->sceneTargetRtvCpuHandle();
    auto depthHandle = dsvHandle();

    check(allocator->Reset(), "failed to reset allocator");
    check(commandList->Reset(allocator.get(), nullptr), "failed to reset command list");

    commandList->RSSetScissorRects(1, &scissor);
    commandList->RSSetViewports(1, &viewport);

    commandList->OMSetRenderTargets(1, &targetHandle, false, &depthHandle);
    commandList->ClearRenderTargetView(targetHandle, kClearColour, 0, nullptr);
    commandList->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    check(commandList->Close(), "failed to close command list");
    return commandList.get();
}

void Scene3D::createCommandList() {
    auto& device = ctx->getDevice();
    auto& allocator = ctx->getAllocator(Allocator::Scene);
    commandList = device.newCommandList(L"scene-command-list", commands::kDirect, allocator);
    check(commandList->Close(), "failed to close command list");
}

void Scene3D::destroyCommandList() {
    commandList.tryDrop("scene-command-list");
}

void Scene3D::createDsvHeap() {
    auto& device = ctx->getDevice();
    dsvHeap = device.newDescriptorHeap(L"scene-dsv-heap", {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 1
    });
}

void Scene3D::destroyDsvHeap() {
    dsvHeap.tryDrop("scene-dsv-heap");
}

void Scene3D::createDepthStencil() {
    auto& device = ctx->getDevice();
    const auto [width, height] = ctx->getInternalResolution();
    const D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
        .Format = kDepthFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D
    };
    
    const D3D12_CLEAR_VALUE clear = {
        .Format = kDepthFormat,
        .DepthStencil = { .Depth = 1.0f, .Stencil = 0 }
    };

    const auto tex = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ kDepthFormat,
        /* width = */ UINT(width),
        /* height = */ UINT(height),
        /* mipLevels = */ 1,
        /* arraySize = */ 0,
        /* sampleCount = */ 1,
        /* sampleQuality = */ 0,
        /* flags = */ D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    depthStencil = device.newCommittedResource(L"scene-depth-stencil",
        kDefaultProps, D3D12_HEAP_FLAG_NONE,
        tex, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear
    );

    device->CreateDepthStencilView(depthStencil.get(), &desc, dsvHandle());
}

void Scene3D::destroyDepthStencil() {
    depthStencil.tryDrop("scene-depth-stencil");
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Scene3D::dsvHandle() {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart());
}
