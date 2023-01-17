#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

SwapChain::SwapChain(IDXGISwapChain3 *swapchain, bool tearing)
    : Super(swapchain)
    , flags(tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u)
    , presentFlags(tearing ? DXGI_PRESENT_ALLOW_TEARING : 0)
{ }

void SwapChain::present() {
    HR_CHECK(get()->Present(0, presentFlags));
}

rhi::Buffer SwapChain::getBuffer(size_t index) {
    ID3D12Resource *resource;
    HR_CHECK(get()->GetBuffer(UINT(index), IID_PPV_ARGS(&resource)));

    return rhi::Buffer(resource);
}

void SwapChain::resize(TextureSize resolution, size_t frames) {
    auto [width, height] = resolution;
    HR_CHECK(get()->ResizeBuffers(
        UINT(frames), 
        UINT(width), UINT(height), 
        DXGI_FORMAT_R8G8B8A8_UNORM, 
        flags
    ));
}

size_t SwapChain::currentBackBuffer() {
    return get()->GetCurrentBackBufferIndex();
}
