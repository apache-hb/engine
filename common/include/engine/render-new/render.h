#pragma once

#include "engine/rhi/rhi.h"

#include "engine/memory/bitmap.h"
#include "engine/memory/slotmap.h"
#include "engine/window/window.h"

namespace simcoe::render {
    struct ThreadHandle;
    struct CopyQueue;
    struct PresentQueue;
    struct Context;

    namespace DescriptorSlot {
        enum Slot : unsigned {
            eDebugBuffer,
            eSceneBuffer,
            eObjectBuffer,
            eTexture,

            eImGui,
            eIntermediate,

            eEmpty
        };

        constexpr const char *getSlotName(Slot slot) {
            switch (slot) {
            case eDebugBuffer: return "debug-buffer";
            case eSceneBuffer: return "scene-buffer";
            case eObjectBuffer: return "object-buffer";
            case eTexture: return "texture";

            case eImGui: return "imgui";
            case eIntermediate: return "intermediate";
            
            default: return "empty";
            }
        }
    }

    using SlotMap = memory::SlotMap<DescriptorSlot::Slot, DescriptorSlot::eEmpty>;
    
    struct ContextInfo {
        Window *window;
        size_t frames = 2; // number of backbuffers
        rhi::TextureSize resolution = { 1280, 720 }; // internal resolution

        /// tuning parameters
        size_t threads = 4; // number of worker threads
        size_t maxTextures = 512;
        size_t maxObjects = 512;
    };

    struct ThreadHandle {
        bool valid() const;
        rhi::CommandList& cmd();

    private:
        friend CopyQueue;

        ThreadHandle(size_t index, CopyQueue *parent)
            : index(index)
            , parent(parent)
        { }

        size_t index;
        CopyQueue *parent;
    };

    // multithreaded upload queue
    struct CopyQueue {
        CopyQueue(rhi::Device& device, const ContextInfo& info);

        ThreadHandle getThread();
        void submit(ThreadHandle thread);

        void wait(Context& ctx);

    private:
        friend ThreadHandle;

        enum State : unsigned {
            eAvailable, // this list is available
            eRecording, // this list is recording commands
            eSubmitting // this list is being submitted
        };
        
        size_t threads;
        std::mutex mutex;

        rhi::CommandQueue queue;

        memory::SlotMap<State, eAvailable> alloc;
        UniquePtr<rhi::CommandList[]> lists;
        UniquePtr<rhi::Allocator[]> allocators;

        size_t index = 0;
    };

    // singlethreaded access only
    struct PresentQueue {
        PresentQueue(rhi::Device& device, const ContextInfo& info);

        void present(Context& ctx);
        void execute(std::span<ID3D12CommandList*> lists);

        size_t currentFrame() const { return current; }

        rhi::CpuHandle getFrameHandle(size_t frame) { return rtvHeap.cpuHandle(frame); }
        rhi::CpuHandle getSceneHandle() { return rtvHeap.cpuHandle(0); }

        rhi::Buffer& getRenderTarget() { return renderTargets[current]; }

    private:
        size_t frames;
        size_t current;

        rhi::CommandQueue queue;
        rhi::SwapChain swapchain;

        rhi::DescriptorSet rtvHeap;
        UniquePtr<rhi::Buffer[]> renderTargets;

        size_t frameIndex = 0;
        rhi::Fence fence;
    };

    struct HeapAllocator : SlotMap {
        HeapAllocator(rhi::Device& device, size_t size, rhi::DescriptorSet::Type type, bool shaderVisible) 
            : SlotMap(size)
            , heap(device.newDescriptorSet(size, type, shaderVisible))
        { }

        rhi::CpuHandle cpuHandle(size_t offset, size_t length, DescriptorSlot::Slot type);
        rhi::GpuHandle gpuHandle(size_t offset, size_t length, DescriptorSlot::Slot type);
        
        rhi::DescriptorSet& getHeap() { return heap; }
    private:
        rhi::DescriptorSet heap;
    };

    struct Context {
        Context(const ContextInfo& info);

        // external api
        void update(const ContextInfo& info);
        void present();

        // render graph api
        void submit(ID3D12CommandList *list);

        // imgui related functions
        void imguiInit(size_t offset);
        void imguiFrame();
        void imguiShutdown();

        // trivial getters
        rhi::Device &getDevice() { return device; }
        PresentQueue& getPresentQueue() { return presentQueue; }
        CopyQueue& getCopyQueue() { return copyQueue; }
        HeapAllocator& getHeap() { return cbvHeap; }

        rhi::Buffer& getRenderTarget() { return presentQueue.getRenderTarget(); }
        rhi::CpuHandle getRenderTargetHandle() { return presentQueue.getFrameHandle(currentFrame()); }

        size_t getFrames() const { return info.frames; }
        size_t currentFrame() const { return presentQueue.currentFrame(); }

        // getters
        rhi::TextureSize windowSize() { return info.window->size().as<size_t>(); }
        rhi::TextureSize sceneSize() { return info.resolution; }

    private:
        void createFrameData();

        std::vector<ID3D12CommandList*> lists;

        ContextInfo info;

        rhi::Device device;

        PresentQueue presentQueue;
        CopyQueue copyQueue;

        rhi::DescriptorSet dsvHeap;
        HeapAllocator cbvHeap;
    };
}