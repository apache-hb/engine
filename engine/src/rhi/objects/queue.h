#pragma once

#include "common.h"

namespace engine {
    struct DxCommandQueue final : rhi::CommandQueue {
        DxCommandQueue(ID3D12CommandQueue *queue);
        rhi::SwapChain *newSwapChain(Window *window, size_t buffers) override;

        void signal(rhi::Fence *fence, size_t value) override;
        void execute(std::span<rhi::CommandList*> lists) override;

    private:
        rhi::UniqueComPtr<ID3D12CommandQueue> queue;
    };
}
