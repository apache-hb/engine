#pragma once

#include "engine/base/logging.h"
#include "engine/base/window.h"

#include "engine/rhi/rhi.h"

#include <vector>

namespace engine::render {
    constexpr auto kFrameCount = 2;

    struct Vertex {
        math::float3 pos;
        math::float4 colour;
    };

    struct alignas(256) ConstBuffer {
        math::float3 offset = { 0.25f, 0.0f, 0.f };
    };

    struct Context {
        struct Create {
            Window *window;
            logging::Channel *channel;
        };

        Context(Create &&info);
        ~Context();

        void begin();
        void end();

    private:
        Create info;

        void create();
        void destroy();

        void waitForFrame();
        void waitOnQueue(rhi::CommandQueue *queue, size_t value);

        void beginCopy();
        size_t endCopy(); // return the value to wait for to signal that all copies are complete

        template<typename F>
        size_t recordCopy(F &&func) {
            beginCopy();
            func();
            return endCopy();
        }

        rhi::Buffer *uploadData(const void *ptr, size_t size);

        rhi::Viewport viewport;

        rhi::Device *device;

        // presentation data
        rhi::SwapChain *swapchain;
        rhi::DescriptorSet *renderTargetSet;

        // presentation queue
        rhi::CommandQueue *directQueue;
        rhi::CommandList *directCommands;

        // copy commands
        rhi::CommandQueue *copyQueue;
        rhi::CommandList *copyCommands;
        rhi::Allocator *copyAllocator;
        std::vector<rhi::Buffer*> pendingCopies;
        size_t currentCopy = 0;

        // per frame data
        rhi::Buffer *renderTargets[kFrameCount];
        rhi::Allocator *allocators[kFrameCount];

        // rendering state
        rhi::PipelineState *pipeline;
        rhi::Buffer *vertexBuffer;
        rhi::Buffer *indexBuffer;

        rhi::DescriptorSet *constBufferSet;
        rhi::Buffer *constBuffer;
        void *constBufferPtr;
        ConstBuffer constBufferData;

        rhi::VertexBufferView vertexBufferView;
        rhi::IndexBufferView indexBufferView;

        // sync objects
        rhi::Fence *fence;
        size_t fenceValue;
        size_t frameIndex;
    };
}
