#pragma once

#include "engine/base/window.h"
#include <span>

namespace engine::rhi {
    enum struct CpuHandle : std::size_t {};
    enum struct GpuHandle : std::size_t {};

    struct Allocator {
        virtual void reset() = 0;
        virtual ~Allocator() = default;
    };

    struct RenderTarget {
        virtual ~RenderTarget() = default;
    };
    
    struct Fence {
        virtual void waitUntil(size_t signal) = 0;
        virtual ~Fence() = default;
    };
    
    struct RenderTargetSet {
        virtual CpuHandle cpuHandle(size_t offset = 0) = 0;

        virtual ~RenderTargetSet() = default;
    };

    struct CommandList {
        virtual void beginRecording(Allocator *allocator) = 0;
        virtual void endRecording() = 0;

        virtual void setRenderTarget(CpuHandle target) = 0;
        virtual void clearRenderTarget(CpuHandle target, const math::float4 &colour) = 0;

        virtual ~CommandList() = default;
    };
    
    struct SwapChain {
        virtual void present() = 0;
        virtual RenderTarget *getBuffer(size_t index) = 0;

        virtual size_t currentBackBuffer() = 0;

        virtual ~SwapChain() = default;
    };

    struct CommandQueue {
        virtual SwapChain *newSwapChain(Window *window, size_t buffers) = 0;
        virtual void signal(Fence *fence, size_t value) = 0;
        virtual void execute(std::span<CommandList*> lists) = 0;

        virtual ~CommandQueue() = default;
    };

    struct Device {
        virtual Fence *newFence() = 0;
        virtual CommandQueue *newQueue() = 0;
        virtual CommandList *newCommandList(rhi::Allocator *allocator) = 0;
        virtual Allocator *newAllocator() = 0;

        virtual RenderTargetSet *newRenderTargetSet(size_t count) = 0;
        virtual void newRenderTarget(RenderTarget *resource, CpuHandle rtvHandle) = 0;

        virtual ~Device() = default;
    };

    Device *getDevice();
}
