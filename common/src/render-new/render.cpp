#include "engine/render-new/render.h"
#include "engine/rhi/rhi.h"

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

std::vector<std::byte> render::loadShader(std::string_view path) {
    UniquePtr<Io> io { Io::open(path, Io::eRead) };
    return io->read<std::byte>();
}

Context::Context(const ContextInfo& info)
    : info(info)
    , device(rhi::getDevice())
    , presentQueue(device, info)
    , copyQueue(device, info)
    , cbvHeap(device, getMaxHeapSize(info), rhi::DescriptorSet::Type::eConstBuffer, true)
{ 
    createFrameData();
    createCommands();
}

void Context::createFrameData() {
    dsvHeap = device.newDescriptorSet(1, rhi::DescriptorSet::Type::eDepthStencil, false);
}

void Context::createCommands() {
    allocators = new rhi::Allocator[info.frames];
    for (size_t i = 0; i < info.frames; ++i) {
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }
    commands = device.newCommandList(allocators[currentFrame()], rhi::CommandList::Type::eDirect);
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
    ID3D12CommandList *lists[] = { commands.get() };
    presentQueue.execute(lists);
    presentQueue.present(*this);
}

void Context::beginFrame() {
    commands.beginRecording(allocators[currentFrame()]);
}

void Context::endFrame() {
    commands.endRecording();
}

void Context::transition(std::span<const rhi::StateTransition> barriers) {
    if (barriers.empty()) { return; }

    commands.transition(barriers);
}
