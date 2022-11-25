#pragma once

#include "engine/rhi/rhi.h"

#include "engine/memory/bitmap.h"
#include "engine/memory/slotmap.h"
#include "engine/window/window.h"

#include "concurrentqueue.h"
#include <functional>

namespace camel = moodycamel;

namespace simcoe::render {
    struct ThreadHandle;
    struct CopyQueue;
    struct PresentQueue;
    struct Context;

    using Shader = std::vector<std::byte>;

    Shader loadShader(std::string_view path);

    namespace CommandSlot {
        enum Slot : unsigned {
            ePost,
            eScene,
            eTotal
        };
    }

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
        size_t maxTextures = 512;
        size_t maxObjects = 512;
    };

    struct AsyncCopy {
        AsyncCopy(rhi::Buffer upload, rhi::Buffer result)
            : upload(std::move(upload))
            , result(std::move(result))
        { }

        virtual ~AsyncCopy() = default;
        virtual void apply(rhi::CommandList& list) = 0;

        rhi::Buffer& data() { return result; }

    protected:
        rhi::Buffer upload;
        rhi::Buffer result;
    };

    using CopyResult = std::shared_ptr<AsyncCopy>;

    struct AsyncCopyQueue {
        AsyncCopyQueue(Context& ctx);

        CopyResult uploadData(const void* data, size_t size);
        CopyResult uploadTexture(const void* data, size_t size, rhi::TextureSize resolution);

        void wait();
    private:
        Context& ctx;

        camel::ConcurrentQueue<CopyResult> pending;

        rhi::CommandQueue queue;

        rhi::Allocator alloc;
        rhi::CommandList list;

        rhi::Fence fence;
        size_t index = 0;
    };

    // singlethreaded access only
    struct PresentQueue {
        PresentQueue(rhi::Device& device, const ContextInfo& info);

        void present(Context& ctx);
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

        size_t frameIndex = 0;
        rhi::Fence fence;
    };

    struct HeapAllocator : SlotMap {
        HeapAllocator(rhi::Device& device, size_t size, rhi::DescriptorSet::Type type, bool shaderVisible) 
            : SlotMap(size)
            , heap(device.newDescriptorSet(size, type, shaderVisible))
        { }

        rhi::CpuHandle cpuHandle(size_t offset, size_t length, DescriptorSlot::Slot type) {
            ASSERTF(
                testBit(offset, type), 
                "expecting {} found {} instead at {}..{}", DescriptorSlot::getSlotName(type),
                DescriptorSlot::getSlotName(getBit(offset)), 
                offset, length
            );
            return heap.cpuHandle(offset);
        }

        rhi::GpuHandle gpuHandle(size_t offset, size_t length, DescriptorSlot::Slot type) {
            ASSERTF(
                testBit(offset, type), 
                "expecting {} found {} instead at {}..{}", DescriptorSlot::getSlotName(type),
                DescriptorSlot::getSlotName(getBit(offset)), 
                offset, length
            );
            return heap.gpuHandle(offset);
        }
        
        rhi::DescriptorSet& getHeap() { return heap; }
        ID3D12DescriptorHeap *get() { return heap.get(); }
    private:
        rhi::DescriptorSet heap;
    };

    struct Context {
        Context(const ContextInfo& info);

        // external api
        void update(const ContextInfo& info);
        void present();

        void beginFrame();
        void endFrame();
        void transition(CommandSlot::Slot slot, std::span<const rhi::StateTransition> barriers);

        // imgui related functions
        void imguiInit(size_t offset);
        void imguiFrame();
        void imguiShutdown();

        // trivial getters
        rhi::Device &getDevice() { return device; }
        PresentQueue& getPresentQueue() { return presentQueue; }
        AsyncCopyQueue& getCopyQueue() { return copyQueue; }
        HeapAllocator& getHeap() { return cbvHeap; }

        rhi::Buffer& getRenderTarget() { return presentQueue.getRenderTarget(); }
        rhi::CpuHandle getRenderTargetHandle() { return presentQueue.getFrameHandle(currentFrame()); }

        rhi::CommandList &getCommands(CommandSlot::Slot slot) { return commands[slot]; }

        size_t getFrames() const { return info.frames; }
        size_t currentFrame() const { return presentQueue.currentFrame(); }

        // getters
        rhi::TextureSize windowSize() { return info.window->size().as<size_t>(); }
        rhi::TextureSize sceneSize() { return info.resolution; }

    private:
        void createFrameData();
        void createCommands();

        ContextInfo info;

        rhi::Device device;

        PresentQueue presentQueue;
        AsyncCopyQueue copyQueue;

        rhi::DescriptorSet dsvHeap;
        HeapAllocator cbvHeap;

        rhi::Allocator &getAllocator(size_t index, CommandSlot::Slot slot) {
            return allocators[(index * getFrames()) + slot];
        }

        UniquePtr<rhi::Allocator[]> allocators;
        rhi::CommandList commands[CommandSlot::eTotal];
    };
}
