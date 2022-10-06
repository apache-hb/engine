#pragma once

#include "engine/base/logging.h"
#include "engine/window/window.h"

#include "engine/rhi/rhi.h"

#include "engine/render/camera.h"

#include <vector>

namespace engine::render {
    constexpr auto kFrameCount = 2;
    constexpr auto kRenderTargets = kFrameCount + 1; // +1 for intermediate target

    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };

    std::vector<std::byte> loadShader(std::string_view path);

    struct Vertex {
        math::float3 pos;
        math::float2 uv;
    };

    struct Mesh {
        rhi::PipelineState pso;

        rhi::Buffer vertexBuffer;
        rhi::Buffer indexBuffer;

        rhi::VertexBufferView vertexBufferView;
        rhi::IndexBufferView indexBufferView;
    };

    struct Context;

    struct Scene {
        Scene(rhi::TextureSize resolution) : resolution(resolution) { }

        virtual ID3D12CommandList *populate(Context *ctx) = 0;
        virtual ID3D12CommandList *attach(Context *ctx) = 0;
        virtual ~Scene() = default;

        rhi::TextureSize resolution;
    };

    struct Context {
        struct Create {
            Window *window; // window to attach to
            logging::Channel *channel; // logging channel
            Scene *scene; // scene to display
        };

        Context(Create &&info);

        void begin();
        void end();

        // accessors
        size_t currentFrame() const { return frameIndex; }
        rhi::Device &getDevice() { return device; }

        logging::Channel *getChannel() { return channel; }

        rhi::Buffer uploadData(const void *ptr, size_t size);
        rhi::Buffer uploadTexture(rhi::CommandList &commands, rhi::TextureSize size, std::span<std::byte> data);

        rhi::CpuHandle getRenderTarget() { return renderTargetSet.cpuHandle(kFrameCount); }

    private:
        Window *window;
        logging::Channel *channel;
        Scene *scene;

        void create();

        void updateViewports();

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
        rhi::Buffer renderTargets[kFrameCount];

        rhi::Allocator postAllocators[kFrameCount];

        rhi::CommandList postCommands;
        rhi::View postView;
        Mesh screenQuad;

        rhi::Buffer intermediateTarget;

        // copy commands
        rhi::CommandQueue copyQueue;
        rhi::CommandList copyCommands;
        rhi::Allocator copyAllocator;
        std::vector<rhi::Buffer> pendingCopies;
        size_t currentCopy = 0;

        // scene data set
        // contains texture, imgui data, and the camera buffer
        rhi::DescriptorSet postDataHeap;
        
        // sync objects
        rhi::Fence fence;
        size_t fenceValue = 0;
        size_t frameIndex;
    };
}
