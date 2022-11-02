#include "engine/render-new/render.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    constexpr size_t getMaxHeapSize(const ContextInfo& info) {
        return info.maxObjects      // object buffers
             + info.maxTextures     // texture buffers
             + 1                    // dear imgui
             + 1;                   // intermediate buffer
    }
}

Context::Context(const ContextInfo& info)
    : info(info)
    , device(rhi::getDevice())
    , presentQueue(device, info)
    , copyQueue(device, info)
    , cbvHeap(device, getMaxHeapSize(info), rhi::DescriptorSet::Type::eConstBuffer, true)
{ 
    createFrameData();
}

void Context::createFrameData() {
    dsvHeap = device.newDescriptorSet(1, rhi::DescriptorSet::Type::eDepthStencil, false);
}

void Context::submit(ID3D12CommandList *list) {
    lists.push_back(list);
}

void Context::imguiInit(size_t offset) {
    device.imguiInit(info.frames, cbvHeap.getHeap(), offset);
}

void Context::imguiFrame() {
    device.imguiFrame();
    info.window->imguiFrame();
}

void Context::imguiShutdown() {
    device.imguiShutdown();
}

void Context::present() {
    presentQueue.execute(lists);
    presentQueue.present(*this);

    lists.clear();
}
