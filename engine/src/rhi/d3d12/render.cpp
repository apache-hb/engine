#include "engine/rhi/rhi.h"

#include "engine/base/panic.h"

#include <dxgi1_6.h>
#include <dxgidebug.h>
#include "dx/d3dx12.h"

#define DX_CHECK(hr) ASSERT(SUCCEEDED(hr))

namespace {
    IDXGIFactory4 *gFactory;
    ID3D12Debug *gDxDebug;
    IDXGIDebug *gDebug;

    void drop(IUnknown *ptr, std::string_view name) {
        ULONG refs = ptr->Release();
        ASSERTF(refs == 0, "failed to drop {}, {} live references", name, refs);
    }
}

using namespace engine;

struct DxAllocator final : rhi::Allocator {
    DxAllocator(ID3D12CommandAllocator *allocator)
        : allocator(allocator)
    { }

    ~DxAllocator() override {
        drop(allocator, "allocator");
    }

    void reset() override {
        DX_CHECK(allocator->Reset());
    }

    ID3D12CommandAllocator *get() { return allocator; }
private:
    ID3D12CommandAllocator *allocator;
};

struct DxFence final : rhi::Fence {
    DxFence(ID3D12Fence *fence, HANDLE event)
        : fence(fence)
        , event(event)
    { }

    ~DxFence() override {
        drop(fence, "fence");
        CloseHandle(event);
    }

    void waitUntil(size_t signal) override {
        if (fence->GetCompletedValue() < signal) {
            DX_CHECK(fence->SetEventOnCompletion(signal, event));
            WaitForSingleObject(event, INFINITE);
        }
    }

    ID3D12Fence *get() { return fence; }

private:
    ID3D12Fence *fence;
    HANDLE event;
};

struct DxRenderTarget final : rhi::RenderTarget {
    DxRenderTarget(ID3D12Resource *resource)
        : resource(resource)
    { }

    ~DxRenderTarget() override {
        resource->Release(); // all render targets share a ref count
    }

    ID3D12Resource *get() { return resource; }

private:
    ID3D12Resource *resource;
};

struct DxRenderTargetSet final : rhi::RenderTargetSet {
    DxRenderTargetSet(ID3D12DescriptorHeap *heap, UINT stride)
        : heap(heap)
        , stride(stride)
    { }

    ~DxRenderTargetSet() override {
        drop(heap, "rtv-heap");
    }

    virtual rhi::CpuHandle cpuHandle(size_t offset) override {
        const auto kCpuHeapStart = heap->GetCPUDescriptorHandleForHeapStart();
        return rhi::CpuHandle(kCpuHeapStart.ptr + (offset * stride));
    }

private:
    ID3D12DescriptorHeap *heap;
    UINT stride;
};

struct DxSwapChain final : rhi::SwapChain {
    DxSwapChain(IDXGISwapChain3 *swapchain)
        : swapchain(swapchain)
    { }

    ~DxSwapChain() override {
        drop(swapchain, "swapchain");
    }

    void present() override {
        DX_CHECK(swapchain->Present(0, 0));
    }

    rhi::RenderTarget *getBuffer(size_t index) override {
        ID3D12Resource *resource;
        DX_CHECK(swapchain->GetBuffer(UINT(index), IID_PPV_ARGS(&resource)));

        return new DxRenderTarget(resource);
    }

    size_t currentBackBuffer() override {
        return swapchain->GetCurrentBackBufferIndex();
    }

private:
    IDXGISwapChain3 *swapchain;
};

struct DxCommandList final : rhi::CommandList {
    DxCommandList(ID3D12GraphicsCommandList *commands)
        : commands(commands)
    { }

    ~DxCommandList() override {
        drop(commands, "command-list");
    }

    void beginRecording(rhi::Allocator *allocator) override {
        auto *alloc = static_cast<DxAllocator*>(allocator);
        DX_CHECK(commands->Reset(alloc->get(), nullptr));
    }
    
    void endRecording() override {
        DX_CHECK(commands->Close());
    }

    void setRenderTarget(rhi::CpuHandle target) override {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv { size_t(target) };
        commands->OMSetRenderTargets(1, &rtv, false, nullptr);
    }

    void clearRenderTarget(rhi::CpuHandle target, const math::float4 &colour) override {
        D3D12_CPU_DESCRIPTOR_HANDLE handle { size_t(target) };
        const float kClear[] = { colour.x, colour.y, colour.z, colour.w };
        commands->ClearRenderTargetView(handle, kClear, 0, nullptr);
    }

    ID3D12GraphicsCommandList *get() { return commands; }
private:
    ID3D12GraphicsCommandList *commands;
};

struct DxCommandQueue final : rhi::CommandQueue {
    DxCommandQueue(ID3D12CommandQueue *queue)
        : queue(queue)
    { }

    ~DxCommandQueue() override {
        drop(queue, "command queue");
    }

    rhi::SwapChain *newSwapChain(Window *window, size_t buffers) override {
        auto [width, height] = window->size();
        HWND handle = window->handle();

        const DXGI_SWAP_CHAIN_DESC1 kDesc = {
            .Width = static_cast<UINT>(width),
            .Height = static_cast<UINT>(height),
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = {
                .Count = 1
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = static_cast<UINT>(buffers),
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        };

        IDXGISwapChain1 *swapchain;
        DX_CHECK(gFactory->CreateSwapChainForHwnd(
            queue,
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

        return new DxSwapChain(swapchain3);
    }

    void signal(rhi::Fence *fence, size_t value) override {
        auto *it = static_cast<DxFence*>(fence);
        DX_CHECK(queue->Signal(it->get(), value));
    }

    void execute(std::span<rhi::CommandList*> lists) override {
        auto **data = new ID3D12CommandList*[lists.size()];
        for (size_t i = 0; i < lists.size(); i++) {
            auto *list = static_cast<DxCommandList*>(lists[i]);
            data[i] = list->get();
        }

        queue->ExecuteCommandLists(UINT(lists.size()), data);
    }

private:
    ID3D12CommandQueue *queue;
};

struct DxDevice final : rhi::Device {
    DxDevice(ID3D12Device *device)
        : device(device)
    { }

    ~DxDevice() override {
        drop(device, "device");
    }

    rhi::Fence *newFence() override {
        HANDLE event = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(event != nullptr);

        ID3D12Fence *fence;
        DX_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        return new DxFence(fence, event);
    }

    rhi::CommandQueue *newQueue() override {
        const D3D12_COMMAND_QUEUE_DESC kDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
        };

        ID3D12CommandQueue *queue;
        DX_CHECK(device->CreateCommandQueue(&kDesc, IID_PPV_ARGS(&queue)));

        return new DxCommandQueue(queue);
    }

    rhi::CommandList *newCommandList(rhi::Allocator *allocator) override {
        auto *alloc = static_cast<DxAllocator*>(allocator);

        ID3D12GraphicsCommandList *commands;
        DX_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc->get(), nullptr, IID_PPV_ARGS(&commands)));
        DX_CHECK(commands->Close());

        return new DxCommandList(commands);
    }
    
    rhi::Allocator *newAllocator() override {
        ID3D12CommandAllocator *allocator;
        DX_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));

        return new DxAllocator(allocator);
    }

    rhi::RenderTargetSet *newRenderTargetSet(size_t count) override {
        const D3D12_DESCRIPTOR_HEAP_DESC kRtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = static_cast<UINT>(count)
        };

        ID3D12DescriptorHeap *rtvHeap;
        DX_CHECK(device->CreateDescriptorHeap(&kRtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        UINT rtvStride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        return new DxRenderTargetSet(rtvHeap, rtvStride);
    }

    void newRenderTarget(rhi::RenderTarget *target, rhi::CpuHandle rtvHandle) override {
        auto *dxTarget = static_cast<DxRenderTarget*>(target);
        D3D12_CPU_DESCRIPTOR_HANDLE handle { size_t(rtvHandle) };
        device->CreateRenderTargetView(dxTarget->get(), nullptr, handle);
    }

private:
    ID3D12Device *device;
};

rhi::Device *rhi::getDevice() {
    DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&gDebug)));

    UINT factoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&gDxDebug)))) {
        gDxDebug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    DX_CHECK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&gFactory)));

    IDXGIAdapter1 *adapter;
    for (UINT i = 0; SUCCEEDED(gFactory->EnumAdapters1(i, &adapter)); i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        break;
    }

    ID3D12Device *device;
    DX_CHECK(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    return new DxDevice(device);
}
