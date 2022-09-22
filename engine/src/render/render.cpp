#include "engine/render/render.h"
#include "dx/d3d12.h"
#include "engine/base/panic.h"

#include "dx/d3d12sdklayers.h"
#include "dx/d3dx12.h"

using namespace engine::render;

#define CHECK(expr) ASSERT(SUCCEEDED(expr))

Context::Context(engine::Window *window, logging::Channel *channel) {
    UINT factoryFlags = 0;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    CHECK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

    for (UINT i = 0; SUCCEEDED(factory->EnumAdapters1(i, &adapter)); i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        break;
    }

    CHECK(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
    };

    CHECK(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    auto [width, height] = window->size();

    DXGI_SWAP_CHAIN_DESC1 swapDesc = {
        .Width = static_cast<UINT>(width),
        .Height = static_cast<UINT>(height),
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = kFrameCount,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
    };

    Com<IDXGISwapChain1> swapChain1;
    CHECK(factory->CreateSwapChainForHwnd(
        commandQueue.get(),
        window->handle(),
        &swapDesc,
        nullptr,
        nullptr,
        &swapChain1
    ));
    CHECK(factory->MakeWindowAssociation(window->handle(), DXGI_MWA_NO_ALT_ENTER));
    ASSERT((swapChain = swapChain1.as<IDXGISwapChain3>()).valid());
    
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = kFrameCount
        };
        CHECK(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ rtvHeap->GetCPUDescriptorHandleForHeapStart() };

        for (UINT i = 0; i < kFrameCount; i++) {
            CHECK(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            device->CreateRenderTargetView(renderTargets[i].get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvSize);
            
            CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
        }
    }

    CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[frameIndex].get(), nullptr, IID_PPV_ARGS(&commandList)));

    CHECK(commandList->Close());

    {
        CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceValue = 1;

        fenceEvent = CreateEvent(nullptr, false, false, nullptr);
        ASSERTF(fenceEvent != nullptr, "{:x}", HRESULT_FROM_WIN32(GetLastError()));
    }
}

void addTransition(ID3D12GraphicsCommandList *list, ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);

    list->ResourceBarrier(1, &barrier);
}

void Context::begin() {
    CHECK(commandAllocators[frameIndex]->Reset());

    CHECK(commandList->Reset(commandAllocators[frameIndex].get(), pipelineState.get()));

    addTransition(commandList.get(), renderTargets[frameIndex].get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvSize);

    constexpr float kClear[] = { 0.f, 0.2f, 0.4f, 1.f };
    commandList->ClearRenderTargetView(rtvHandle, kClear, 0, nullptr);

    addTransition(commandList.get(), renderTargets[frameIndex].get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    CHECK(commandList->Close());
}

void Context::end() {
    ID3D12CommandList *lists[] = { commandList.get() };
    commandQueue->ExecuteCommandLists(sizeof(lists) / sizeof(ID3D12CommandList*), lists);

    CHECK(swapChain->Present(1, 0));

    waitForFrame();
}

void Context::close() {
    waitForFrame();

    CloseHandle(fenceEvent);
}

void Context::waitForFrame() {
    const auto oldValue = fenceValue;
    CHECK(commandQueue->Signal(fence.get(), oldValue));
    fenceValue += 1;

    if (fence->GetCompletedValue() < oldValue) {
        CHECK(fence->SetEventOnCompletion(oldValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    frameIndex = swapChain->GetCurrentBackBufferIndex();
}
