#pragma once

#include "engine/base/logging.h"
#include "engine/base/window.h"

#include "engine/rhi/rhi.h"

#include <vector>

namespace engine::render {
    constexpr auto kFrameCount = 2;

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

        rhi::Device *device;
        rhi::CommandQueue *directQueue;
        rhi::CommandList *directCommands;

        rhi::SwapChain *swapchain;

        rhi::RenderTargetSet *renderTargetSet;

        rhi::RenderTarget *renderTargets[kFrameCount];
        rhi::Allocator *allocators[kFrameCount];

        rhi::Fence *fence;
        size_t fenceValue;
        size_t frameIndex;
    };
}
