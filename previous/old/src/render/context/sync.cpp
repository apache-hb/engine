#include "render/context.h"

using namespace engine::render;

void Context::waitForGpu() {
    auto& value = frameData[frameIndex].value;

    check(queues[Queues::Direct]->Signal(fence.get(), value), "failed to signal fence");
    check(fence->SetEventOnCompletion(value, fenceEvent), "failed to set fence event");

    WaitForSingleObject(fenceEvent, INFINITE);

    value += 1;
}

void Context::nextFrame() {
    auto value = frameData[frameIndex].value;
    check(queues[Queues::Direct]->Signal(fence.get(), value), "failed to signal fence");

    frameIndex = swapchain->GetCurrentBackBufferIndex();

    auto& current = frameData[frameIndex].value;
    if (fence->GetCompletedValue() < current) {
        check(fence->SetEventOnCompletion(current, fenceEvent), "failed to set fence event");
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    current = value + 1;
}
