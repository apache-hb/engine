#include "engine/render-new/render.h"
#include "engine/rhi/rhi.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    size_t getRtvHeapSize(size_t frames) {
        return frames + 1; // +1 for scene target
    }
}

struct AsyncBufferCopy final : AsyncCopy {
    AsyncBufferCopy(rhi::Buffer upload, rhi::Buffer result, size_t size)
        : AsyncCopy(std::move(upload), std::move(result))
        , size(size)
    { }

    void apply(rhi::CommandList& list) override {
        list.copyBuffer(result, upload, size);
    }

private:
    size_t size;
};

struct AsyncTextureCopy final : AsyncCopy {
    AsyncTextureCopy(rhi::Buffer upload, rhi::Buffer result)
        : AsyncCopy(std::move(upload), std::move(result))
    { }

    void apply(rhi::CommandList& list) override {
        list.copyTexture(result, upload);
    }
};

// async copy queue

AsyncCopyQueue::AsyncCopyQueue(Context& ctx)
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

CopyResult AsyncCopyQueue::uploadData(const void* data, size_t size) {
    auto& device = ctx.getDevice();

    rhi::Buffer buffer = device.newBuffer(size, rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::State::eUpload);
    rhi::Buffer upload = device.newBuffer(size, rhi::DescriptorSet::Visibility::eDeviceOnly, rhi::Buffer::State::eCommon);
    buffer.write(data, size);

    buffer.rename("upload buffer");
    upload.rename("upload texture");

    auto result = std::make_shared<AsyncBufferCopy>(std::move(buffer), std::move(upload), size);
    pending.enqueue(result);
    return result;
}

CopyResult AsyncCopyQueue::uploadTexture(const void* data, size_t size, rhi::TextureSize resolution) {
    auto& device = ctx.getDevice();

    rhi::Buffer buffer = device.newBuffer(size, rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::State::eUpload);
    rhi::Buffer upload = device.newBuffer(size, rhi::DescriptorSet::Visibility::eDeviceOnly, rhi::Buffer::State::eCommon);
    buffer.writeTexture(data, resolution); // TODO: this is suspect

    buffer.rename("upload buffer");
    upload.rename("upload texture");

    auto result = std::make_shared<AsyncTextureCopy>(std::move(buffer), std::move(upload));
    pending.enqueue(result);
    return result;
}

void AsyncCopyQueue::wait() {
    std::vector<CopyResult> results;
    
    CopyResult copy;
    while (pending.try_dequeue(copy)) {
        results.push_back(copy);
    }

    if (results.empty()) { return; }

    list.beginRecording(alloc);
    for (const auto& result : results) {
        result->apply(list);
    }
    list.endRecording();

    ID3D12CommandList* lists[] = { list.get() };
    queue.execute(lists);
    queue.signal(fence, index);
    fence.wait(index++);
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
