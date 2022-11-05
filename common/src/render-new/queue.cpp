#include "engine/render-new/render.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    size_t getRtvHeapSize(size_t frames) {
        return frames;
    }
}

// thread primitive

bool ThreadHandle::valid() const {
    return index != SIZE_MAX && parent != nullptr;
}

rhi::CommandList& ThreadHandle::cmd() {
    ASSERT(valid());
    return parent->lists[index];
}

rhi::Buffer ThreadHandle::uploadData(const void* data, size_t size) {
    auto& device = parent->ctx.getDevice();
    auto& list = cmd();
    rhi::Buffer upload = device.newBuffer(size, rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::State::eUpload);
    rhi::Buffer result = device.newBuffer(size, rhi::DescriptorSet::Visibility::eDeviceOnly, rhi::Buffer::State::eCopyDst);

    upload.rename("upload");
    result.rename("upload result");

    upload.write(data, size);
    list.copyBuffer(result, upload, size);

    std::lock_guard lock(parent->stagingMutex);
    parent->stagingBuffers.push_back(std::move(upload));

    return result;
}

// copy queue

CopyQueue::CopyQueue(Context& ctx, const ContextInfo& info) 
    : threads(info.threads) 
    , ctx(ctx)
    , queue(ctx.getDevice().newQueue(rhi::CommandList::Type::eCopy))
    , alloc(info.threads)
    , fence(ctx.getDevice().newFence())
{
    auto& device = ctx.getDevice();
    lists = new rhi::CommandList[threads];
    allocators = new rhi::Allocator[threads];

    queue.rename("copy queue");
    fence.rename("copy queue fence");

    for (size_t i = 0; i < threads; ++i) {
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eCopy);
        lists[i] = device.newCommandList(allocators[i], rhi::CommandList::Type::eCopy);
        
        allocators[i].rename(std::format("copy allocator {}", i));
        lists[i].rename(std::format("copy queue {}", i));
    }
}

ThreadHandle CopyQueue::getThread() {
    std::lock_guard lock(mutex);

    size_t index = alloc.alloc(eRecording);
    if (index == SIZE_MAX) { return { SIZE_MAX, nullptr }; }
    
    lists[index].beginRecording(allocators[index]);
    return { index, this };
}

void CopyQueue::submit(ThreadHandle thread) {
    std::lock_guard lock(mutex);

    lists[thread.index].endRecording();
    alloc.update(thread.index, eSubmitting);
}

void CopyQueue::wait() {
    std::lock_guard lock(mutex);

    std::vector<ID3D12CommandList*> lists;
    for (size_t i = 0; i < alloc.getSize(); i++) {
        if (alloc.checkBit(i, eSubmitting)) {
            alloc.release(eSubmitting, i);
            lists.push_back(this->lists[i].get());
        }
    }

    if (lists.empty()) { return; }
    queue.execute(lists);
    
    queue.signal(fence, index);
    fence.wait(index);

    index += 1;

    std::lock_guard stagingLock(stagingMutex);
    stagingBuffers.clear();
}

// present queue

PresentQueue::PresentQueue(rhi::Device& device, const ContextInfo& info)
    : frames(info.frames)
    , queue(device.newQueue(rhi::CommandList::Type::eDirect))
    , swapchain(queue.newSwapChain(info.window, frames))
    , rtvHeap(device.newDescriptorSet(getRtvHeapSize(frames), rhi::DescriptorSet::Type::eRenderTarget, false))
    , fence(device.newFence())
{ 
    renderTargets = new rhi::Buffer[frames];
    for (size_t i = 0; i < frames; ++i) {
        renderTargets[i] = swapchain.getBuffer(i);
        device.createRenderTargetView(renderTargets[i], rtvHeap.cpuHandle(i));
    }

    current = swapchain.currentBackBuffer();
}

void PresentQueue::present(UNUSED Context& ctx) {
    swapchain.present();

    queue.signal(fence, frameIndex);
    fence.wait(frameIndex);

    frameIndex += 1;
    current = swapchain.currentBackBuffer();
}

void PresentQueue::execute(std::span<ID3D12CommandList*> lists) {
    queue.execute(lists);
}
