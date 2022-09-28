#include "objects/swapchain.h"

#include "objects/buffer.h"

using namespace engine;

DxSwapChain::DxSwapChain(IDXGISwapChain3 *swapchain, bool tearing)
    : swapchain(swapchain)
    , flags(tearing ? DXGI_PRESENT_ALLOW_TEARING : 0)
{ }

void DxSwapChain::present() {
    DX_CHECK(swapchain->Present(0, flags));
}

rhi::Buffer *DxSwapChain::getBuffer(size_t index) {
    ID3D12Resource *resource;
    DX_CHECK(swapchain->GetBuffer(UINT(index), IID_PPV_ARGS(&resource)));

    return new DxBuffer(resource);
}

size_t DxSwapChain::currentBackBuffer() {
    return swapchain->GetCurrentBackBufferIndex();
}
