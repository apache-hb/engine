#pragma once

#include "engine/base/logging.h"
#include "engine/base/window.h"

#include "engine/rhi/rhi.h"

#include "engine/render/camera.h"

#include <vector>

namespace engine::render {
    constexpr auto kFrameCount = 2;
    constexpr auto kRenderTargets = kFrameCount + 1; // +1 for intermediate target

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

    struct alignas(256) ConstBuffer {
        math::float4x4 mvp = math::float4x4::identity();
    };

    struct Context;

    struct Scene {
        Scene(Context *ctx);
        virtual ~Scene() = default;
    };

    struct Context {
        struct Create {
            Window *window; // window to attach to
            logging::Channel *channel; // logging channel
            rhi::TextureSize resolution = { 1920 * 2, 1080 * 2 }; // internal render resolution
        };

        Context(Create &&info);

        void begin(Camera *camera);
        void end();

    private:
        Create info;

        void create();

        void updateViewports();

        void waitForFrame();
        void waitOnQueue(rhi::CommandQueue &queue, size_t value);

        void beginCopy();
        size_t endCopy(); // return the value to wait for to signal that all copies are complete

        template<typename F>
        size_t recordCopy(F &&func) {
            beginCopy();
            func();
            return endCopy();
        }

        void beginScene();
        void beginPost();

        void endScene();
        void endPost();

        rhi::Buffer uploadData(const void *ptr, size_t size);
        rhi::Buffer uploadTexture(rhi::TextureSize size, std::span<std::byte> data);

        rhi::Device device;

        // scene data
        float aspectRatio;

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

        // scene data
        rhi::CommandList sceneCommands;
        rhi::View sceneView;

        // per frame scene data
        rhi::Allocator sceneAllocators[kFrameCount];

        // fullscreen quad
        Mesh sceneObject;

        // scene data set
        // contains texture, imgui data, and the camera buffer
        rhi::DescriptorSet postDataHeap;
        rhi::DescriptorSet sceneDataHeap;
        
        // const buffer data
        rhi::Buffer constBuffer;
        void *constBufferPtr;
        ConstBuffer constBufferData;

        rhi::Buffer textureBuffer;

        // sync objects
        rhi::Fence fence;
        size_t fenceValue = 0;
        size_t frameIndex;
    };
}
