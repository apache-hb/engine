#include "simcoe/render/context.h"
#include "dx/d3d12.h"
#include "simcoe/core/util.h"

using namespace simcoe;
using namespace simcoe::render;

Context::Context(system::Window &window, const Info& info) 
    : window(window)
    , info(info)
    , cbvHeap(info.heapSize)
{
    newFactory();
    newDevice();
    newPresentQueue();
    newSwapChain();
    newRenderTargets();
    newCommandList();
    newDescriptorHeap();
    newFence();
}

Context::~Context() {
    deleteFence();
    deleteDescriptorHeap();
    deleteCommandList();
    deleteRenderTargets();
    deleteSwapChain();
    deletePresentQueue();
    deleteDevice();
    deleteFactory();
}

void Context::begin() {
    // populate command list
    HR_CHECK(commandAllocators[frameIndex]->Reset());
    HR_CHECK(pCommandList->Reset(commandAllocators[frameIndex], nullptr));
    
    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissor);

    CD3DX12_RESOURCE_BARRIER toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargets[frameIndex],
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    pCommandList->ResourceBarrier(1, &toRenderTarget);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRenderTargetHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
    const float clearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    ID3D12DescriptorHeap *ppHeaps[] = { cbvHeap.getHeap() };
    
    pCommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    pCommandList->ClearRenderTargetView(rtvHandle, clearColour, 0, nullptr);
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

void Context::end() {
    CD3DX12_RESOURCE_BARRIER toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargets[frameIndex],
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    pCommandList->ResourceBarrier(1, &toPresent);
    
    HR_CHECK(pCommandList->Close());

    // execute command list
    ID3D12CommandList* ppCommandLists[] = { pCommandList };
    pPresentQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    HR_CHECK(pSwapChain->Present(0, bTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0));
    nextFrame();
}

void Context::newFactory() {
    UINT flags = 0;

    if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug)); SUCCEEDED(hr)) {
        pDebug->EnableDebugLayer();
        flags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    HR_CHECK(CreateDXGIFactory2(flags, IID_PPV_ARGS(&pFactory)));
}

void Context::deleteFactory() {
    RELEASE(pFactory);
    RELEASE(pDebug);

    IDXGIDebug1 *pDebug1 = nullptr;
    if (HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug1)); SUCCEEDED(hr)) {
        pDebug1->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        RELEASE(pDebug1);
    } else {
        log.warn("DXGIGetDebugInterface1() = {}", system::hrString(hr));
    }
}

void Context::newDevice() {
    if (info.adapter == SIZE_MAX) {
        selectDefaultAdapter();
    } else {
        selectAdapter(info.adapter);
    }

    DXGI_ADAPTER_DESC1 desc;
    HR_CHECK(pAdapter->GetDesc1(&desc));

    log.info("creating device with adapter {}", util::narrow(desc.Description));

    HR_CHECK(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)));
}

void Context::deleteDevice() {
    RELEASE(pDevice);
    RELEASE(pAdapter);
}

void Context::selectAdapter(size_t index) {
    log.info("selecting adapter #{}", index);
    HR_CHECK(pFactory->EnumAdapters1(UINT(index), &pAdapter));
    ASSERT(pAdapter != nullptr);
}

void Context::selectDefaultAdapter() {
    log.info("selecting default adapter");
    HR_CHECK(pFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)));
    ASSERT(pAdapter != nullptr);
}

void Context::newPresentQueue() {
    const D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
    };

    HR_CHECK(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pPresentQueue)));
}

void Context::deletePresentQueue() {
    RELEASE(pPresentQueue);
}

void Context::newSwapChain() {
    auto [width, height] = window.size();
    
    // TODO: respect info.resolution and info.displayMode
    viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    scissor = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

    if (!SUCCEEDED(pFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bTearingSupported, sizeof(bTearingSupported)))) {
        bTearingSupported = FALSE;
    }

    DXGI_SWAP_CHAIN_DESC1 desc = {
        .Width = static_cast<UINT>(width),
        .Height = static_cast<UINT>(height),
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = static_cast<UINT>(info.frames),
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = bTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
    };

    IDXGISwapChain1 *pSwapChain1 = nullptr;
    HR_CHECK(pFactory->CreateSwapChainForHwnd(pPresentQueue, window.getHandle(), &desc, nullptr, nullptr, &pSwapChain1));
    HR_CHECK(pFactory->MakeWindowAssociation(window.getHandle(), DXGI_MWA_NO_ALT_ENTER));

    HR_CHECK(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain)));
    RELEASE(pSwapChain1);
}

void Context::deleteSwapChain() {
    RELEASE(pSwapChain);
}

void Context::newRenderTargets() {
    rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = static_cast<UINT>(info.frames),
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    HR_CHECK(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRenderTargetHeap)));

    renderTargets.resize(info.frames);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRenderTargetHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < info.frames; i++) {
        HR_CHECK(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
        pDevice->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }
}

void Context::deleteRenderTargets() {
    for (auto &pRenderTarget : renderTargets) {
        RELEASE(pRenderTarget);
    }

    RELEASE(pRenderTargetHeap);
}

void Context::newCommandList() {
    commandAllocators.resize(info.frames);
    
    for (UINT i = 0; i < info.frames; i++) {
        HR_CHECK(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
    }

    HR_CHECK(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0], nullptr, IID_PPV_ARGS(&pCommandList)));
    HR_CHECK(pCommandList->Close());
}

void Context::deleteCommandList() {
    RELEASE(pCommandList);

    for (auto &pCommandAllocator : commandAllocators) {
        RELEASE(pCommandAllocator);
    }
}

void Context::newFence() {
    HR_CHECK(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    ASSERT(fenceEvent != nullptr);

    waitForFence();
}

void Context::deleteFence() {
    CloseHandle(fenceEvent);
    RELEASE(pFence);
}

void Context::waitForFence() {
    HR_CHECK(pPresentQueue->Signal(pFence, fenceValue));

    HR_CHECK(pFence->SetEventOnCompletion(fenceValue, fenceEvent));
    WaitForSingleObject(fenceEvent, INFINITE);

    fenceValue += 1;
}

void Context::nextFrame() {
    HR_CHECK(pPresentQueue->Signal(pFence, fenceValue));

    frameIndex = pSwapChain->GetCurrentBackBufferIndex();

    if (pFence->GetCompletedValue() < fenceValue) {
        HR_CHECK(pFence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    fenceValue += 1;
}

void Context::newDescriptorHeap() {
    cbvHeap.newHeap(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Context::deleteDescriptorHeap() {
    cbvHeap.deleteHeap();
}
