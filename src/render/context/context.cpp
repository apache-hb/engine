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
    createFrameData();
}

void Context::destroy() {
    destroyFrameData();
    destroyRtvHeap();
    destroySwapChain();
    destroyCommandQueues();
    destroyDevice();
}

void Context::present() {

}

void Context::addEvent(Event&& event) {
    events.push(std::move(event));
}

void Context::bindRtv(Resource resource, DescriptorHeap::CpuHandle handle) {
    device->CreateRenderTargetView(resource.get(), nullptr, handle);
}
