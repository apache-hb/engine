#pragma once

#include "engine/render/async.h"

#include "concurrentqueue.h"

#include <functional>
#include <condition_variable>

namespace camel = moodycamel;

using namespace std::chrono_literals;

namespace simcoe::render {
    struct Context;

    struct ContextInfo {
        Window *window;
        size_t frames = 2; // number of backbuffers
        rhi::TextureSize resolution = { 1280, 720 }; // internal resolution

        /// tuning parameters
        size_t maxTextures = 512;
        size_t maxObjects = 512;
        std::chrono::nanoseconds copyQueueWaitPeroid = 10ms; 
    };

    using AsyncCallback = std::function<void()>;

    struct CopyQueue {
        CopyQueue(rhi::Device& device, const ContextInfo& info);
 
        AsyncAction beginDataUpload(const void* data, size_t size);
        AsyncAction beginTextureUpload(const void* data, size_t size, rhi::TextureSize resolution);
        AsyncAction beginBatchUpload(std::vector<AsyncAction>&& actions);

        void submit(AsyncAction action, AsyncCallback complete = []{});

        void wait();
    private:
        bool waitForPendingWork();
        void signalPendingWork();

        struct PendingAction {
            AsyncAction action;
            AsyncCallback complete;
        };

        rhi::Device& device;

        std::mutex mutex;
        std::condition_variable hasPendingWork;
        std::chrono::nanoseconds waitPeriod;

        camel::ConcurrentQueue<PendingAction> pending;

        rhi::CommandQueue queue;

        rhi::Allocator alloc;
        rhi::CommandList list;

        rhi::Fence fence;
        size_t index = 1;
    };

    struct PresentQueue {
        PresentQueue(rhi::Device& device, const ContextInfo& info);

        void present();
        void execute(std::span<ID3D12CommandList*> lists);

        size_t currentFrame() const { return current; }

        rhi::CpuHandle getFrameHandle(size_t frame) { return rtvHeap.cpuHandle(frame + 1); }
        rhi::CpuHandle getSceneHandle() { return rtvHeap.cpuHandle(0); }

        rhi::Buffer& getRenderTarget() { return renderTargets[current]; }

    private:
        size_t frames;
        size_t current;

        rhi::CommandQueue queue;
        rhi::SwapChain swapchain;

        rhi::DescriptorSet rtvHeap;
        UniquePtr<rhi::Buffer[]> renderTargets;

        size_t frameIndex = 1;
        rhi::Fence fence;
    };
}
