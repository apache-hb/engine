#include "engine/render-new/render.h"
#include "engine/rhi/rhi.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    size_t getRtvHeapSize(size_t frames) {
        return frames + 1; // +1 for scene target
    }
}

// async copy queue

CopyQueue::CopyQueue(Context& ctx)
    : ctx(ctx)
    , queue(ctx.getDevice().newQueue(rhi::CommandList::Type::eCopy))
    , alloc(ctx.getDevice().newAllocator(rhi::CommandList::Type::eCopy))
    , list(ctx.getDevice().newCommandList(alloc, rhi::CommandList::Type::eCopy))
    , fence(ctx.getDevice().newFence())
{ 
    queue.rename("copy queue");
    alloc.rename("copy allocator");
    list.rename("copy list");
    fence.rename("copy fence");
}

void CopyQueue::submit(AsyncAction action, AsyncCallback complete) {
    PendingAction it = { action, complete };
    pending.enqueue(it);
}

void CopyQueue::wait() {
    std::vector<PendingAction> results;
    
    PendingAction action;
    while (pending.try_dequeue(action)) {
        results.push_back(action);
    }

    if (results.empty()) { return; }

    list.beginRecording(alloc);
    for (const auto& [action, _] : results) {
        action->apply(list);
    }
    list.endRecording();

    ID3D12CommandList* lists[] = { list.get() };
    queue.execute(lists);
    queue.signal(fence, index);
    fence.wait(index++);

    for (const auto& [_, complete] : results) {
        complete();
    }
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
    for (size_t i = 0; i < frames; i++) {
        renderTargets[i] = swapchain.getBuffer(i);
        device.createRenderTargetView(renderTargets[i], getFrameHandle(i));
        renderTargets[i].rename(std::format("frame {}", i));
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
