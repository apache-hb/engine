#pragma once

#include "common.h"

#include <dxgi1_6.h>

namespace engine {
    struct DxSwapChain final : rhi::SwapChain {
        DxSwapChain(IDXGISwapChain3 *swapchain);

        void present() override;

        rhi::Buffer *getBuffer(size_t index) override;

        size_t currentBackBuffer() override;

    private:
        UniqueComPtr<IDXGISwapChain3> swapchain;
    };
}
