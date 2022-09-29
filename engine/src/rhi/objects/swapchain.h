#pragma once

#include "common.h"

namespace engine {
    struct DxSwapChain final : rhi::SwapChain {
        DxSwapChain(IDXGISwapChain3 *swapchain, bool tearing);

        void present() override;

        rhi::Buffer *getBuffer(size_t index) override;

        size_t currentBackBuffer() override;

    private:
        rhi::UniqueComPtr<IDXGISwapChain3> swapchain;
        UINT flags;
    };
}
