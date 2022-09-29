#include "objects/queue.h"

#include "objects/swapchain.h"
#include "objects/commands.h"

using namespace engine;

DxCommandQueue::DxCommandQueue(ID3D12CommandQueue *queue)
    : queue(queue)
{ }

rhi::SwapChain *DxCommandQueue::newSwapChain(Window *window, size_t buffers) {
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
        queue.get(),
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

    return new DxSwapChain(swapchain3, tearing);
}

void DxCommandQueue::signal(rhi::Fence &fence, size_t value) {
    DX_CHECK(queue->Signal(fence.get(), value));
}

void DxCommandQueue::execute(std::span<rhi::CommandList*> lists) {
    UniquePtr<ID3D12CommandList*[]> data(new ID3D12CommandList*[lists.size()]);
    for (size_t i = 0; i < lists.size(); i++) {
        auto *list = static_cast<DxCommandList*>(lists[i]);
        data[i] = list->get();
    }

    queue->ExecuteCommandLists(UINT(lists.size()), data.get());
}
