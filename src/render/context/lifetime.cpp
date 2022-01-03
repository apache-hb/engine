#include "render/render.h"

#include "render/objects/factory.h"
#include "render/objects/commands.h"

#include <array>
#include <DirectXMath.h>

using namespace engine::render;

constexpr auto kSwapSampleCount = 1;
constexpr auto kSwapSampleQuality = 0;

constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

void Context::createDevice() {
    device = getAdapter().newDevice<ID3D12Device4>(D3D_FEATURE_LEVEL_11_0, L"render-device");
}

void Context::destroyDevice() {
    device.tryDrop("device");
}

void Context::createViews() {
    // get our current internal and external resolution
    auto [displayWidth, displayHeight] = getDisplayResolution();
    auto [internalWidth, internalHeight] = getInternalResolution();

    // calculate required scaling to prevent displaying
    // our drawn image outside the display area
    auto widthRatio = float(internalWidth) / float(displayWidth);
    auto heightRatio = float(internalHeight) / float(displayHeight);

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) { 
        x = widthRatio / heightRatio; 
    } else { 
        y = heightRatio / widthRatio; 
    }

    auto left = float(displayWidth) * (1.f - x) / 2.f;
    auto top = float(displayHeight) * (1.f - y) / 2.f;
    auto width = float(displayWidth * x);
    auto height = float(displayHeight * y);

    // set our viewport and scissor rects
    postView = View(top, left, width, height);
    sceneView = View(0.f, 0.f, float(internalWidth), float(internalHeight));

    displayResolution = Resolution(displayWidth, displayHeight);
    sceneResolution = Resolution(internalWidth, internalHeight);
}

void Context::createDirectCommandQueue() {
    directCommandQueue = device.newCommandQueue(L"direct-command-queue", { commands::kDirect });
}

void Context::destroyDirectCommandQueue() {
    directCommandQueue.tryDrop("direct-command-queue");
}

void Context::createUploadCommandQueue() {
    copyCommandQueue = device.newCommandQueue(L"copy-command-queue", { commands::kCopy });
}

void Context::destroyUploadCommandQueue() {
    copyCommandQueue.tryDrop("copy-command-queue");
}

void Context::createSwapChain() {
    auto& factory = getFactory();
    const auto [width, height] = getDisplayResolution();
    const auto format = getFormat();
    const auto bufferCount = getBufferCount();
    const auto flags = factory.flags();

    const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width = width,
        .Height = height,
        .Format = format,
        .SampleDesc = { kSwapSampleCount, kSwapSampleQuality },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = bufferCount,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = flags
    };

    auto swapchain1 = factory.createSwapChain(directCommandQueue, getWindow().getHandle(), swapChainDesc);

    if (auto swapchain3 = swapchain1.as<IDXGISwapChain3>(); swapchain3) {
        swapchain = swapchain3;
    } else {
        throw engine::Error("failed to create swapchain");
    }

    frameIndex = swapchain->GetCurrentBackBufferIndex();
}

void Context::destroySwapChain() {
    swapchain.tryDrop("swapchain");
}

void Context::createRtvHeap() {
    rtvHeap = device.newDescriptorHeap(L"rtv-heap", {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = requiredRtvHeapSize()
    });
}

void Context::destroyRtvHeap() {
    rtvHeap.tryDrop("rtv-heap");
}

void Context::createCbvHeap() {
    cbvHeap = device.newDescriptorHeap(L"cbv-heap", {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = requiredCbvHeapSize(),
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    });
}

void Context::destroyCbvHeap() {
    cbvHeap.tryDrop("cbv-heap");
}

void Context::createSceneTarget() {
    const auto format = getFormat();
    const auto [width, height] = getInternalResolution();
    const auto clearValue = CD3DX12_CLEAR_VALUE(format, kClearColour);
    const auto targetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ format,
        /* width = */ width,
        /* height = */ height,
        /* arraySize = */ 1,
        /* mipLevels = */ 1,
        /* sampleCount = */ kSwapSampleCount,
        /* sampleQuality = */ kSwapSampleQuality,
        /* flags = */ D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );
    
    sceneTarget = device.newCommittedResource(
        L"scene-target", kDefaultProps, D3D12_HEAP_FLAG_NONE,
        targetDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue
    );

    device->CreateRenderTargetView(sceneTarget.get(), nullptr, sceneTargetRtvCpuHandle());
    device->CreateShaderResourceView(sceneTarget.get(), nullptr, sceneTargetCbvCpuHandle());
}

void Context::destroySceneTarget() {
    sceneTarget.tryDrop("scene-target");
}

void Context::createFrameData() {
    const auto frameCount = getBufferCount();
    frameData = new FrameData[frameCount];

    for (UINT i = 0; i < frameCount; i++) {
        auto *frame = frameData + i;

        frame->fenceValue = 0;

        check(swapchain->GetBuffer(i, IID_PPV_ARGS(&frame->target)), "failed to get swapchain buffer");
        device->CreateRenderTargetView(frame->target.get(), nullptr, rtvHeapCpuHandle(i));
    
        frame->target.rename(std::format(L"frame-target-{}", i));

        for (auto alloc = 0; alloc < Allocator::Total; alloc++) {
            auto allocator = frame->allocators[alloc];
            auto self = Allocator::Index(alloc);

            const auto type = Allocator::getType(self);
            const auto name = Allocator::getName(self);

            frame->allocators[alloc] = device.newCommandAllocator(std::format(L"frame-allocator-{}-{}", name, i), type);
        }
    }
}

void Context::destroyFrameData() {
    for (UINT i = 0; i < getBufferCount(); i++) {
        auto *frame = frameData + i;

        for (auto alloc = 0; alloc < Allocator::Total; alloc++) {
            auto allocator = frame->allocators + alloc;
            allocator->tryDrop("allocator");
        }

        frame->target.release();
    }

    delete[] frameData;
}

void Context::createFence() {
    const auto frame = getCurrentFrame();
    fence = device.newFence(L"fence", frameData[frame].fenceValue);

    frameData[frame].fenceValue = 1;

    if ((fenceEvent = CreateEvent(nullptr, false, false, nullptr)) == nullptr) {
        throw win32::Error("failed to create fence event");
    }
}

void Context::destroyFence() {
    fence.tryDrop("fence");
    CloseHandle(fenceEvent);
}

void Context::createCopyCommandList() {
    copyCommandList = device.newCommandList(L"copy-command-list", commands::kCopy, getAllocator(Allocator::Copy));
}

void Context::destroyCopyCommandList() {
    copyCommandList.tryDrop("copy-command-list");

    for (auto& resource : copyResources) {
        resource.tryDrop("copy-resource");
    }
}

void Context::createCopyFence() {
    copyFence = device.newFence(L"copy-fence", 0);
    copyFenceValue = 1;

    if ((copyFenceEvent = CreateEvent(nullptr, false, false, nullptr)) == nullptr) {
        throw win32::Error("failed to create copy fence event");
    }
}

void Context::destroyCopyFence() {
    copyFence.tryDrop("copy-fence");
    CloseHandle(copyFenceEvent);
}
