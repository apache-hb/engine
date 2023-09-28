#include "common.h"

#include "simcoe/core/util.h"
#include "simcoe/core/system.h"
#include "simcoe/simcoe.h"

namespace util = simcoe::util;
namespace rhi = simcoe::rhi;

namespace {
    constexpr D3D12_COMMAND_LIST_TYPE getCommandType(rhi::CommandType type) {
        switch (type) {
        case rhi::CommandType::ePresent:
        case rhi::CommandType::eDirect:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;

        case rhi::CommandType::eCopy:
            return D3D12_COMMAND_LIST_TYPE_COPY;

        default: NEVER("invalid command type");
        }
    }

    constexpr DXGI_FORMAT getColour(rhi::ColourFormat colour) {
        switch (colour) {
        case rhi::ColourFormat::eRGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;

        default: NEVER("invalid colour format");
        }
    }

    constexpr D3D12_DESCRIPTOR_HEAP_TYPE getHeapType(rhi::HeapType type) {
        switch (type) {
        case rhi::HeapType::eDepthStencil: return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        case rhi::HeapType::eRenderTarget: return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        default: NEVER("invalid heap type");
        }
    }

    constexpr D3D12_RESOURCE_STATES getResourceState(rhi::ResourceState state) {
        switch (state) {
        case rhi::ResourceState::ePresent: return D3D12_RESOURCE_STATE_PRESENT;
        case rhi::ResourceState::eRenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;

        default: NEVER("invalid resource state");
        }
    }

    d3d::Queue *createQueue(d3d::Device *pDevice, D3D12_COMMAND_LIST_TYPE type) {
        D3D12_COMMAND_QUEUE_DESC desc = { .Type = type };

        d3d::Queue *pObject = nullptr;
        HR_CHECK(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pObject)));
        return pObject;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(rhi::CpuHandle handle) {
        return D3D12_CPU_DESCRIPTOR_HANDLE { size_t(handle) };
    }

    D3D12_RESOURCE_BARRIER createTransitionBarrier(rhi::Barrier barrier) {
        rhi::Transition transition = barrier.data.transition;
        DxSurface *pSurface = static_cast<DxSurface*>(transition.pResource);
        D3D12_RESOURCE_BARRIER result = {
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .Transition = {
                .pResource = pSurface->get(),
                .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                .StateBefore = getResourceState(transition.from),
                .StateAfter = getResourceState(transition.to)
            }
        };

        return result;
    }

    D3D12_RESOURCE_BARRIER createBarrier(rhi::Barrier barrier) {
        switch (barrier.type) {
        case rhi::BarrierType::eTransition:
            return createTransitionBarrier(barrier);

        default:
            NEVER("invalid barrier type");
        }
    }
}

// fence

void DxFence::wait(size_t value) {
    HR_CHECK(get()->SetEventOnCompletion(UINT64(value), event));
    WaitForSingleObject(event, INFINITE);
}

size_t DxFence::getValue() const {
    return size_t(get()->GetCompletedValue());
}

DxFence *DxFence::create(d3d::Device *pDevice) {
    ID3D12Fence *pObject = nullptr;
    HR_CHECK(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pObject)));

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event) {
        HR_CHECK(HRESULT_FROM_WIN32(GetLastError()));
    }

    return new DxFence(pObject, event);
}

// heap

rhi::GpuHandle DxHeap::getGpuHandle(size_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE start = get()->GetGPUDescriptorHandleForHeapStart();
    return rhi::GpuHandle { start.ptr + increment * index };
}

rhi::CpuHandle DxHeap::getCpuHandle(size_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE start = get()->GetCPUDescriptorHandleForHeapStart();
    return rhi::CpuHandle { start.ptr + increment * index };
}

DxHeap *DxHeap::create(d3d::Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t size) {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = type,
        .NumDescriptors = UINT(size),
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    d3d::DescriptorHeap *pObject = nullptr;
    HR_CHECK(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pObject)));

    UINT increment = pDevice->GetDescriptorHandleIncrementSize(type);

    return new DxHeap(pObject, increment);
}

// command list

void DxCommandList::begin() {
    HR_CHECK(pAllocator->Reset());
    HR_CHECK(get()->Reset(pAllocator.get(), nullptr));
}

void DxCommandList::end() {
    HR_CHECK(get()->Close());
}


void DxCommandList::clearRenderTarget(rhi::CpuHandle handle, math::float4 colour) {
    get()->ClearRenderTargetView(getCpuHandle(handle), &colour.x, 0, nullptr);
}

void DxCommandList::transition(std::span<const rhi::Barrier> barriers) {
    std::vector<D3D12_RESOURCE_BARRIER> dxBarriers{barriers.size()};
    for (size_t i = 0; i < barriers.size(); i++) {
        dxBarriers[i] = createBarrier(barriers[i]);
    }

    get()->ResourceBarrier(UINT(dxBarriers.size()), dxBarriers.data());
}

DxCommandList *DxCommandList::create(d3d::Device *pDevice, D3D12_COMMAND_LIST_TYPE type) {
    ID3D12CommandAllocator *pAllocator = nullptr;
    HR_CHECK(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&pAllocator)));

    ID3D12GraphicsCommandList *pCommandList = nullptr;
    HR_CHECK(pDevice->CreateCommandList(0, type, pAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));

    HR_CHECK(pCommandList->Close());

    return new DxCommandList(pCommandList, pAllocator);
}

// command queue

void DxCommandQueue::execute(std::span<rhi::ICommandList*> lists) {
    std::vector<ID3D12CommandList*> dxLists{lists.size()};
    for (size_t i = 0; i < lists.size(); i++) {
        auto *pDxList = static_cast<DxCommandList*>(lists[i]);
        dxLists[i] = pDxList->get();
    }

    get()->ExecuteCommandLists(UINT(dxLists.size()), dxLists.data());
}

void DxCommandQueue::signal(rhi::IFence *pFence, size_t value) {
    auto *pDxFence = static_cast<DxFence *>(pFence);
    HR_CHECK(get()->Signal(pDxFence->get(), UINT64(value)));
}

DxCommandQueue *DxCommandQueue::create(d3d::Device *pDevice, D3D12_COMMAND_LIST_TYPE type) {
    ID3D12CommandQueue *pObject = createQueue(pDevice, type);

    return new DxCommandQueue(pObject);
}

// surface

DxSurface *DxSurface::create(ID3D12Resource *pResource) {
    return new DxSurface(pResource);
}

// display queue

void DxDisplayQueue::present() {
    HR_CHECK(get()->Present(0, tearingSupport ? DXGI_PRESENT_ALLOW_TEARING : 0));
}

size_t DxDisplayQueue::getCurrentFrame() const {
    return get()->GetCurrentBackBufferIndex();
}

rhi::ISurface *DxDisplayQueue::getSurface(size_t index) {
    ID3D12Resource *pResource = nullptr;
    HR_CHECK(get()->GetBuffer(UINT(index), IID_PPV_ARGS(&pResource)));

    return DxSurface::create(pResource);
}

DxDisplayQueue *DxDisplayQueue::create(dxgi::Factory *pFactory, d3d::Queue *pQueue, const rhi::DisplayQueueInfo& info) {
    BOOL tearingSupport = FALSE;
    if (HRESULT hr = pFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupport, sizeof(tearingSupport)); FAILED(hr)) {
        tearingSupport = FALSE;
        simcoe::gRenderLog.warn("failed to check tearing support: {}", simcoe::os::hrString(hr));
    }

    auto [width, height] = info.size.as<UINT>();
    auto handle = info.window.getHandle();

    DXGI_SWAP_CHAIN_DESC1 desc = {
        .Width = width,
        .Height = height,
        .Format = getColour(info.format),
        .SampleDesc = { .Count = 1 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = UINT(info.frames),
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = tearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
    };

    IDXGISwapChain1 *pChain1 = nullptr;
    HR_CHECK(pFactory->CreateSwapChainForHwnd(pQueue, handle, &desc, nullptr, nullptr, &pChain1));
    HR_CHECK(pFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));

    IDXGISwapChain4 *pChain = nullptr;
    HR_CHECK(pChain1->QueryInterface(IID_PPV_ARGS(&pChain)));

    return new DxDisplayQueue(pChain, tearingSupport);
}

// device

rhi::ICommandQueue *DxDevice::createCommandQueue(rhi::CommandType type) {
    return DxCommandQueue::create(get(), getCommandType(type));
}

rhi::ICommandList *DxDevice::createCommandList(rhi::CommandType type) {
    return DxCommandList::create(get(), getCommandType(type));
}

rhi::IDisplayQueue *DxDevice::createDisplayQueue(rhi::ICommandQueue *pQueue, const rhi::DisplayQueueInfo& info) {
    DxCommandQueue *pDxQueue = static_cast<DxCommandQueue*>(pQueue);
    return DxDisplayQueue::create(pFactory, pDxQueue->get(), info);
}

rhi::IFence *DxDevice::createFence() {
    return DxFence::create(get());
}

rhi::IHeap *DxDevice::createHeap(rhi::HeapType type, size_t size) {
    return DxHeap::create(get(), getHeapType(type), size);
}

void DxDevice::mapRenderTarget(rhi::ISurface *pSurface, rhi::CpuHandle handle) {
    auto *pDxSurface = static_cast<DxSurface*>(pSurface);
    get()->CreateRenderTargetView(pDxSurface->get(), nullptr, getCpuHandle(handle));
}

DxDevice *DxDevice::create(dxgi::Factory *pFactory, dxgi::Adapter *pAdapter) {
    d3d::Device *pDevice = nullptr;
    HR_CHECK(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice)));
    return new DxDevice(pFactory, pDevice);
}

// adapter

rhi::IDevice *DxAdapter::createDevice() {
    return DxDevice::create(pFactory, get());
}

DxAdapter *DxAdapter::create(dxgi::Factory *pFactory, dxgi::SimpleAdapter *pAdapter1) {
    dxgi::Adapter *pAdapter = nullptr;
    HR_CHECK(pAdapter1->QueryInterface(IID_PPV_ARGS(&pAdapter)));

    DXGI_ADAPTER_DESC1 desc;
    HR_CHECK(pAdapter->GetDesc1(&desc));

    rhi::AdapterInfo info = {
        .name = util::narrow(desc.Description),
        .type = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? rhi::AdapterType::eSoftware : rhi::AdapterType::eNone,
    };

    return new DxAdapter(pFactory, pAdapter, info);
}

// context

rhi::IAdapter *DxContext::getAdapter(size_t index) {
    dxgi::SimpleAdapter *pAdapterSimple = nullptr;
    HRESULT hr = get()->EnumAdapters1(UINT(index), &pAdapterSimple);
    if (hr == DXGI_ERROR_NOT_FOUND) {
        return nullptr;
    }

    return DxAdapter::create(get(), pAdapterSimple);
}

DxContext *DxContext::create() {
    dxgi::Factory *pObject = nullptr;
    HR_CHECK(CreateDXGIFactory2(0, IID_PPV_ARGS(&pObject)));
    return new DxContext(pObject);
}

extern "C" DLLEXPORT rhi::IContext* rhiGetContext() {
    return DxContext::create();
}
