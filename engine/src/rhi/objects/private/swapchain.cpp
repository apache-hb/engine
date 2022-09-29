#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace engine;
using namespace engine::rhi;

SwapChain::SwapChain(IDXGISwapChain3 *swapchain, bool tearing)
    : Super(swapchain)
    , flags(tearing ? DXGI_PRESENT_ALLOW_TEARING : 0)
{ }

void SwapChain::present() {
    DX_CHECK(get()->Present(0, flags));
}

rhi::Buffer SwapChain::getBuffer(size_t index) {
    ID3D12Resource *resource;
    DX_CHECK(get()->GetBuffer(UINT(index), IID_PPV_ARGS(&resource)));

    return rhi::Buffer(resource);
}

size_t SwapChain::currentBackBuffer() {
    return get()->GetCurrentBackBufferIndex();
}
