#pragma once

#include "engine/base/container/unique.h"

#include "engine/memory/bitmap.h"

#include "engine/base/logging.h"
#include "engine/rhi/rhi.h"
#include "engine/window/window.h"

#include "engine/render/world.h"
#include "engine/render/camera.h"

namespace simcoe::render {
    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };
    constexpr size_t kHeapSize = 1024;

    std::vector<std::byte> loadShader(std::string_view path);

    struct Context;

    struct RenderScene {
        RenderScene(rhi::TextureSize resolution) : resolution(resolution) { }

        virtual ID3D12CommandList *populate() = 0;
        virtual ID3D12CommandList *attach(Context *ctx) = 0;
        virtual ~RenderScene() = default;

        rhi::TextureSize resolution;
    };

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

    using SlotMap = memory::DebugBitMap<DescriptorSlot::Slot, DescriptorSlot::eEmpty>;

    struct Context {
        struct Create {
            Window *window; // window to attach to
            RenderScene *scene; // scene to display
            size_t frames = 2;
        };

        Context(Create &&info);
        ~Context();

        // end api
        void begin();
        void end();

        void resizeScene(rhi::TextureSize size);
        void resizeOutput(rhi::TextureSize size);

        // accessors
        size_t currentFrame() const { return frameIndex; }
        size_t frameCount() const { return frames; }
        rhi::Device &getDevice() { return device; }

        rhi::Buffer uploadData(const void *ptr, size_t size);
        rhi::Buffer uploadTexture(rhi::CpuHandle handle, rhi::CommandList &commands, rhi::TextureSize size, std::span<const std::byte> data);

        rhi::CpuHandle getRenderTarget() { return renderTargetSet.cpuHandle(frames); }

        rhi::CpuHandle getCbvCpuHandle(size_t offset) { return cbvHeap.cpuHandle(offset); }
        rhi::GpuHandle getCbvGpuHandle(size_t offset) { return cbvHeap.gpuHandle(offset); }

        SlotMap& getAlloc() { return cbvAlloc; }
        rhi::DescriptorSet &getHeap() { return cbvHeap; }

    private:
        Window *window;
        RenderScene *scene;
        size_t frames;

        // output resolution
        rhi::TextureSize resolution;

        void create();
        void createRenderTargets();

        void createScene();

        void updateViewports(rhi::TextureSize post, rhi::TextureSize scene);

        void waitForFrame();
        void waitOnQueue(rhi::CommandQueue &queue, size_t value);

        void executeCopy(size_t signal);

        void beginCopy();
        size_t endCopy(); // return the value to wait for to signal that all copies are complete

        void beginPost();
        void endPost();

        rhi::Device device;

        // presentation queue
        rhi::CommandQueue directQueue;
        rhi::SwapChain swapchain;

        rhi::DescriptorSet renderTargetSet;
        UniquePtr<rhi::Buffer[]> renderTargets;

        UniquePtr<rhi::Allocator[]> postAllocators;

        rhi::CommandList postCommands;
        rhi::View postView;

        // fullscreen quad
        rhi::PipelineState pso;
        rhi::Buffer vertexBuffer;
        rhi::Buffer indexBuffer;
        rhi::VertexBufferView vertexBufferView;
        rhi::IndexBufferView indexBufferView;

        rhi::Buffer intermediateTarget;

        // copy commands
        rhi::CommandQueue copyQueue;
        rhi::CommandList copyCommands;
        rhi::Allocator copyAllocator;
        std::vector<rhi::Buffer> pendingCopies;
        size_t currentCopy = 0;

        // scene data set
        // contains texture, imgui data, and the camera buffer
        rhi::DescriptorSet cbvHeap;
        SlotMap cbvAlloc;

        // sync objects
        rhi::Fence fence;
        size_t fenceValue = 0;
        size_t frameIndex;

        size_t intermediateHeapOffset;
        size_t imguiHeapOffset;
    };
}
