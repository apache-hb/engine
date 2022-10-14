#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

rhi::SwapChain CommandQueue::newSwapChain(Window *window, size_t buffers) {
    auto [width, height] = window->size();
    HWND handle = window->handle();

    BOOL tearing = false;
    if (!SUCCEEDED(gFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(BOOL)))) {
        tearing = false;
    }

    const DXGI_SWAP_CHAIN_DESC1 kDesc {
        .Width = static_cast<UINT>(width),
        .Height = static_cast<UINT>(height),
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = static_cast<UINT>(buffers),
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
    };

    IDXGISwapChain1 *swapchain;
    DX_CHECK(gFactory->CreateSwapChainForHwnd(
        get(),
        handle,
        &kDesc,
        nullptr,
        nullptr,
        &swapchain
    ));

    DX_CHECK(gFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));

    IDXGISwapChain3 *swapchain3;
    DX_CHECK(swapchain->QueryInterface(IID_PPV_ARGS(&swapchain3)));
    ASSERT(swapchain->Release() == 1);

    return rhi::SwapChain(swapchain3, tearing);
}

void CommandQueue::signal(rhi::Fence &fence, size_t value) {
    DX_CHECK(get()->Signal(fence.get(), value));
}

void CommandQueue::execute(std::span<ID3D12CommandList*> lists) {
    get()->ExecuteCommandLists(UINT(lists.size()), lists.data());
}
