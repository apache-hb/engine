#pragma once

#include "engine/container/unique.h"

#include "engine/base/logging.h"
#include "engine/window/window.h"

#include "engine/render/world.h"
#include "engine/render/camera.h"

namespace simcoe::render {
    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };

    std::vector<std::byte> loadShader(std::string_view path);

    struct Context;

    struct RenderScene {
        RenderScene(rhi::TextureSize resolution) : resolution(resolution) { }

        virtual ID3D12CommandList *populate(Context *ctx) = 0;
        virtual ID3D12CommandList *attach(Context *ctx) = 0;
        virtual ~RenderScene() = default;

        rhi::TextureSize resolution;
    };

    struct Context {
        struct Create {
            Window *window; // window to attach to
            RenderScene *scene; // scene to display
            size_t frames = 2;
        };

        Context(Create &&info);

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
        rhi::Buffer uploadTexture(rhi::CommandList &commands, rhi::TextureSize size, std::span<const std::byte> data);

        rhi::CpuHandle getRenderTarget() { return renderTargetSet.cpuHandle(frames); }

    private:
        Window *window;
        RenderScene *scene;
        size_t frames;

        // output resolution
        rhi::TextureSize resolution;

        void create();
        void createRenderTargets();

        void updateViewports(rhi::TextureSize post, rhi::TextureSize scene);

        void waitForFrame();
        void waitOnQueue(rhi::CommandQueue &queue, size_t value);

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
        rhi::DescriptorSet postHeap;
        
        // sync objects
        rhi::Fence fence;
        size_t fenceValue = 0;
        size_t frameIndex;
    };
}