#include "render/context.h"

using namespace engine::render;

Context::Context(Create&& create)
    : info(create)
    , events(info.budget.queueSize)
    , frameData(new FrameData[info.buffers])
{ }

void Context::create() {
    createDevice();
    createViews();
    createCommandQueues();
    createSwapChain();
    createRtvHeap();
    createCbvHeap();
    createFrameData();
    createFence();

    waitForGpu();
}

void Context::destroy() {
    waitForGpu();

    destroyFence();
    destroyFrameData();
    destroyCbvHeap();
    destroyRtvHeap();
    destroySwapChain();
    destroyCommandQueues();
    destroyDevice();
}

void Context::present() {
    auto interval = info.vsync ? 1 : 0;
    auto flags = info.vsync ? 0 : getFactory().presentFlags();
    check(swapchain->Present(interval, flags), "failed to present swapchain");

    nextFrame();
}

void Context::addEvent(Event&& event) {
    events.push(std::move(event));
}

void Context::bindRtv(Resource resource, DescriptorHeap::CpuHandle handle) {
    device->CreateRenderTargetView(resource.get(), nullptr, handle);
}

void Context::bindCbv(Resource resource, UINT size, DescriptorHeap::CpuHandle handle) {
    D3D12_CONSTANT_BUFFER_VIEW_DESC view = {
        .BufferLocation = resource.gpuAddress(),
        .SizeInBytes = size
    };
    device->CreateConstantBufferView(&view, handle);
}

void Context::bindSrv(Resource resource, DescriptorHeap::CpuHandle handle) {
    device->CreateShaderResourceView(resource.get(), nullptr, handle);
}

