#pragma once

#include "engine/graph/graph.h"

#include "engine/memory/bitmap.h"
#include "engine/rhi/rhi.h"

namespace simcoe::graph {
    using TransitionVec = std::vector<rhi::StateTransition>;
    struct RenderResource {
        RenderResource(const Info& create) : info(create) { }

        virtual ~RenderResource() = default;

        virtual void transition(rhi::Buffer::State from, rhi::Buffer::State to, TransitionVec& sink);

    protected:
        RenderGraph *getParent() { return info.parent; }
        Info info;
    };
    
    struct DeviceResource final : RenderResource {
        rhi::Device &get() { return device; }
    private:
        rhi::Device device;
    };

    struct TextureResource final : RenderResource {
        rhi::Buffer &get() { return texture; }

        rhi::CpuHandle getCpuHandle() { return cpuHandle; }
        rhi::GpuHandle getGpuHandle() { return gpuHandle; }

        void transition(rhi::Buffer::State from, rhi::Buffer::State to, TransitionVec& sink) override {
            if (from == to) { return; }

            sink.push_back(rhi::newStateTransition(texture, from, to));
        }

    private:
        rhi::CpuHandle cpuHandle;
        rhi::GpuHandle gpuHandle;
        rhi::Buffer texture;
    };

    struct BufferResource final : RenderResource {
        rhi::Buffer &get() { return buffer; }

    private:
        rhi::Buffer buffer;
    };

    struct HeapResource final : RenderResource {
        rhi::DescriptorSet &get() { return heap; }
        size_t alloc() { return slots.alloc(); }

    private:
        rhi::DescriptorSet heap;
        memory::BitMap slots;
    };

    struct SwapChainResource final : RenderResource {
        rhi::CommandQueue &getQueue() { return queue; }
        rhi::SwapChain &getSwapChain() { return swapchain; }

    private:
        rhi::CommandQueue queue;
        rhi::SwapChain swapchain;
    };

    struct FenceResource final : RenderResource {
        rhi::Fence &get() { return fence; }

    private:
        rhi::Fence fence;
    };

    struct CommandResource final : RenderResource {
        CommandResource(const Info& info, rhi::Device& device, rhi::CommandList::Type type);

        rhi::CommandList &get() { return commands; }

    private:
        UniquePtr<rhi::Allocator[]> allocators;
        rhi::CommandList commands;
    };
}
