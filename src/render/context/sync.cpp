#include "render/render.h"

using namespace engine::render;

void Context::waitForGpu() {
    const auto frame = getCurrentFrame();
    auto& value = frameData[frame].fenceValue;

    check(directCommandQueue->Signal(fence.get(), value), "failed to signal fence");
    check(fence->SetEventOnCompletion(value, fenceEvent), "failed to set fence event");

    WaitForSingleObject(fenceEvent, INFINITE);

    value++;
}

void Context::nextFrame() {
    const auto frame = getCurrentFrame();
    auto value = frameData[frame].fenceValue;
    check(directCommandQueue->Signal(fence.get(), value), "failed to signal fence");
        
    frameIndex = swapchain->GetCurrentBackBufferIndex();

    auto &current = frameData[frameIndex].fenceValue;

    if (fence->GetCompletedValue() < current) {
        check(fence->SetEventOnCompletion(current, fenceEvent), "failed to set fence event");
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    current = value + 1;
}
