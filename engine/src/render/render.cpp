#include "engine/render/render.h"
#include <array>

using namespace engine;
using namespace engine::render;

constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };

Context::Context(Create &&info): info(info) {
    create();
}

Context::~Context() {
    destroy();
}

void Context::create() {
    device = rhi::getDevice();

    directQueue = device->newQueue();
    swapchain = directQueue->newSwapChain(info.window, kFrameCount);
    frameIndex = swapchain->currentBackBuffer();

    renderTargetSet = device->newRenderTargetSet(kFrameCount);
    for (size_t i = 0; i < kFrameCount; i++) {
        rhi::RenderTarget *target = swapchain->getBuffer(i);
        device->newRenderTarget(target, renderTargetSet->cpuHandle(i));

        renderTargets[i] = target;
        allocators[i] = device->newAllocator();
    }

    directCommands = device->newCommandList(allocators[frameIndex]);

    fence = device->newFence();
}

void Context::destroy() {
    delete fence;

    delete directCommands;

    for (size_t i = 0; i < kFrameCount; i++) {
        delete renderTargets[i];
        delete allocators[i];
    }
    
    delete renderTargetSet;

    delete swapchain;
    delete directQueue;

    delete device;
}

void Context::begin() {
    const rhi::CpuHandle rtvHandle = renderTargetSet->cpuHandle(frameIndex);

    directCommands->beginRecording(allocators[frameIndex]);

    directCommands->setRenderTarget(rtvHandle, kClearColour);
}

void Context::end() {
    directCommands->endRecording();

    auto commands = std::to_array({ directCommands });
    directQueue->execute(commands);
    swapchain->present();

    waitForFrame();
}

void Context::waitForFrame() {
    const auto value = fenceValue++;

    directQueue->signal(fence, value);
    fence->waitUntil(value);

    frameIndex = swapchain->currentBackBuffer();
}