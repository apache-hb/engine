#pragma once

#include "engine/base/logging.h"
#include "engine/base/window.h"

#include "engine/rhi/rhi.h"

#include "engine/render/camera.h"

#include <vector>

namespace engine::render {
    constexpr auto kFrameCount = 2;

    struct Vertex {
        math::float3 pos;
        math::float2 uv;
    };

    struct alignas(256) ConstBuffer {
        math::float4x4 mvp = math::float4x4::identity();
    };

    struct Context {
        struct Create {
            Window *window;
            logging::Channel *channel;
        };

        Context(Create &&info);

        void begin(Camera *camera);
        void end();

    private:
        Create info;

        void create();

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

        rhi::Buffer uploadData(const void *ptr, size_t size);
        rhi::Buffer uploadTexture(rhi::TextureSize size, std::span<uint8_t> data);

        rhi::Viewport viewport;
        float aspectRatio;

        rhi::Device device;

        // presentation data
        rhi::SwapChain swapchain;
        rhi::DescriptorSet renderTargetSet;

        // presentation queue
        rhi::CommandQueue directQueue;
        rhi::CommandList directCommands;

        // copy commands
        rhi::CommandQueue copyQueue;
        rhi::CommandList copyCommands;
        rhi::Allocator copyAllocator;
        std::vector<rhi::Buffer> pendingCopies;
        size_t currentCopy = 0;

        // per frame data
        rhi::Buffer renderTargets[kFrameCount];
        rhi::Allocator allocators[kFrameCount];

        // rendering state
        rhi::PipelineState pipeline;
        rhi::Buffer vertexBuffer;
        rhi::Buffer indexBuffer;

        // scene data set
        // contains texture, imgui data, and the camera buffer
        rhi::DescriptorSet dataSet;
        
        // const buffer data
        rhi::Buffer constBuffer;
        void *constBufferPtr;
        ConstBuffer constBufferData;

        rhi::Buffer textureBuffer;

        // out object
        rhi::VertexBufferView vertexBufferView;
        rhi::IndexBufferView indexBufferView;

        // sync objects
        rhi::Fence fence;
        size_t fenceValue;
        size_t frameIndex;
    };
}
