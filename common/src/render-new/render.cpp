#include "engine/render-new/render.h"
#include "dx/d3d12.h"
#include "engine/rhi/rhi.h"
#include <array>
#include <debugapi.h>

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
    , copyQueue(*this, info)
    , cbvHeap(device, getMaxHeapSize(info), rhi::DescriptorSet::Type::eConstBuffer, true)
{ 
    createFrameData();
    createCommands();
}

void Context::createFrameData() {
    dsvHeap = device.newDescriptorSet(1, rhi::DescriptorSet::Type::eDepthStencil, false);
}

void Context::createCommands() {
    allocators = new rhi::Allocator[info.frames * CommandSlot::eTotal];
    for (size_t i = 0; i < CommandSlot::eTotal; ++i) {
        for (size_t j = 0; j < info.frames; ++j) {
            allocators[i * info.frames + j] = device.newAllocator(rhi::CommandList::Type::eDirect);
        }
        commands[i] = device.newCommandList(getAllocator(currentFrame(), CommandSlot::Slot(i)), rhi::CommandList::Type::eDirect);
    }
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
    ID3D12CommandList *lists[CommandSlot::eTotal] = { };
    for (size_t i = 0; i < CommandSlot::eTotal; i++) {
        lists[i] = commands[i].get();
    }
    presentQueue.execute(lists);
    presentQueue.present(*this);
}

void Context::beginFrame() {
    auto kDescriptors = std::to_array({ getHeap().get() });

    for (size_t i = 0; i < CommandSlot::eTotal; i++) {
        commands[i].beginRecording(getAllocator(currentFrame(), CommandSlot::Slot(i)));
        commands[i].bindDescriptors(kDescriptors);
        OutputDebugStringA(std::format("begin {}\n", i).c_str());
    }
}

void Context::endFrame() {
    for (size_t i = 0; i < CommandSlot::eTotal; i++) {
        OutputDebugStringA(std::format("close {}\n", i).c_str());
        commands[i].endRecording();
    }
}

void Context::transition(CommandSlot::Slot slot, std::span<const rhi::StateTransition> barriers) {
    commands[slot].transition(barriers);
}
