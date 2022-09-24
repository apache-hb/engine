#include "objects/swapchain.h"

#include "objects/buffer.h"

using namespace engine;

DxSwapChain::DxSwapChain(IDXGISwapChain3 *swapchain)
    : swapchain(swapchain)
{ }

void DxSwapChain::present() {
    DX_CHECK(swapchain->Present(0, 0));
}

rhi::Buffer *DxSwapChain::getBuffer(size_t index) {
    ID3D12Resource *resource;
    DX_CHECK(swapchain->GetBuffer(UINT(index), IID_PPV_ARGS(&resource)));

    return new DxBuffer(resource);
}

size_t DxSwapChain::currentBackBuffer() {
    return swapchain->GetCurrentBackBufferIndex();
}
