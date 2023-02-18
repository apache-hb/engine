#include "simcoe/render/context.h"
#include "dx/d3d12.h"
#include "simcoe/core/util.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    const char *severityToString(D3D12_MESSAGE_SEVERITY severity) {
        switch (severity) {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION: return "CORRUPTION";
        case D3D12_MESSAGE_SEVERITY_ERROR: return "ERROR";
        case D3D12_MESSAGE_SEVERITY_WARNING: return "WARNING";
        case D3D12_MESSAGE_SEVERITY_INFO: return "INFO";
        case D3D12_MESSAGE_SEVERITY_MESSAGE: return "MESSAGE";
        default: return "UNKNOWN";
        }
    }

    const char *categoryToString(D3D12_MESSAGE_CATEGORY category) {
        switch (category) {
        case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED: return "APPLICATION_DEFINED";
        case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS: return "MISCELLANEOUS";
        case D3D12_MESSAGE_CATEGORY_INITIALIZATION: return "INITIALIZATION";
        case D3D12_MESSAGE_CATEGORY_CLEANUP: return "CLEANUP";
        case D3D12_MESSAGE_CATEGORY_COMPILATION: return "COMPILATION";
        case D3D12_MESSAGE_CATEGORY_STATE_CREATION: return "STATE_CREATION";
        case D3D12_MESSAGE_CATEGORY_STATE_SETTING: return "STATE_SETTING";
        case D3D12_MESSAGE_CATEGORY_STATE_GETTING: return "STATE_GETTING";
        case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: return "RESOURCE_MANIPULATION";
        case D3D12_MESSAGE_CATEGORY_EXECUTION: return "EXECUTION";
        case D3D12_MESSAGE_CATEGORY_SHADER: return "SHADER";
        default: return "UNKNOWN";
        }
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::getCpuTargetHandle() const {
    auto& data = frameData[frameIndex];
    return rtvHeap.cpuHandle(data.index);
}

D3D12_GPU_DESCRIPTOR_HANDLE Context::getGpuTargetHandle() const {
    auto& data = frameData[frameIndex];
    return rtvHeap.gpuHandle(data.index);
}

ID3D12Resource *Context::getRenderTargetResource() const {
    auto& data = frameData[frameIndex];
    return data.pRenderTarget;
}

Context::Context(system::Window &window, const Info& info) 
    : window(window)
    , info(info)
    , rtvHeap(getRenderHeapSize())
    , cbvHeap(info.heapSize)
    , copyQueue(info.queueSize)
{
    newFactory();
    newDevice();
    newInfoQueue();
    newPresentQueue();
    newSwapChain();
    newRenderTargets();
    newCommandList();
    newDescriptorHeap();
    newCopyQueue();
    newFence();
}

Context::~Context() {
    deleteFence();
    deleteCopyQueue();
    deleteDescriptorHeap();
    deleteCommandList();
    deleteRenderTargets();
    deleteSwapChain();
    deletePresentQueue();
    deleteInfoQueue();
    deleteDevice();
    deleteFactory();
}

void Context::begin() {
    // populate command list
    HR_CHECK(commandAllocators[frameIndex]->Reset());
    HR_CHECK(pCommandList->Reset(commandAllocators[frameIndex], nullptr));
    
    ID3D12DescriptorHeap *ppHeaps[] = { cbvHeap.getHeap() };
    
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

void Context::end() {
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
        gRenderLog.warn("DXGIGetDebugInterface1() = {}", system::hrString(hr));
    }
}

void Context::newDevice() {
    listAdapters();

    if (info.adapter == kDefaultAdapter) {
        selectDefaultAdapter();
    } else {
        selectAdapter(info.adapter);
    }

    DXGI_ADAPTER_DESC1 desc;
    HR_CHECK(pAdapter->GetDesc1(&desc));

    gRenderLog.info("creating device with adapter {}", util::narrow(desc.Description));

    HR_CHECK(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)));
}

void Context::deleteDevice() {
    RELEASE(pDevice);
    RELEASE(pAdapter);
}

void Context::newInfoQueue() {
    if (HRESULT hr = pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue)); SUCCEEDED(hr)) {
        D3D12MessageFunc pfn = [](D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR pDescription, void *pUser) {
            ReportOnceMap& pMap = *reinterpret_cast<ReportOnceMap*>(pUser);
            util::DoOnce& once = pMap[id];

            once([&]{
                const char *categoryStr = categoryToString(category);
                const char *severityStr = severityToString(severity);

                switch (severity) {
                case D3D12_MESSAGE_SEVERITY_CORRUPTION:
                case D3D12_MESSAGE_SEVERITY_ERROR:
                    gRenderLog.fatal("{}: {} ({}): {}", categoryStr, severityStr, UINT(id), pDescription);
                    break;
                case D3D12_MESSAGE_SEVERITY_WARNING:
                    gRenderLog.warn("{}: {} ({}): {}", categoryStr, severityStr, UINT(id), pDescription);
                    break;

                default:
                case D3D12_MESSAGE_SEVERITY_INFO:
                case D3D12_MESSAGE_SEVERITY_MESSAGE:
                    gRenderLog.info("{}: {} ({}): {}", categoryStr, severityStr, UINT(id), pDescription);
                    return;
                }
            });
        };

        DWORD cookie = 0;
        pInfoQueue->RegisterMessageCallback(pfn, D3D12_MESSAGE_CALLBACK_FLAG_NONE, &reportOnceMap, &cookie);

        gRenderLog.info("ID3D12InfoQueue::RegisterMessageCallback() = {}", cookie);
    } else {
        gRenderLog.warn("ID3D12Device::QueryInterface(IID_PPV_ARGS(&pInfoQueue)) = {}", system::hrString(hr));
    }
}

void Context::deleteInfoQueue() {
    RELEASE(pInfoQueue);
}

void Context::listAdapters() {
    IDXGIAdapter1 *it = nullptr;
    for (UINT i = 0; pFactory->EnumAdapters1(i, &it) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc;
        HR_CHECK(it->GetDesc1(&desc));

        gRenderLog.info("adapter #{}: {}", i, util::narrow(desc.Description));
        RELEASE(it);
    }
}

void Context::selectAdapter(size_t index) {
    gRenderLog.info("selecting adapter #{}", index);
    HRESULT hr = pFactory->EnumAdapters1(UINT(index), &pAdapter);
    if (!SUCCEEDED(hr)) {
        gRenderLog.warn("adapter #{} not found. selecting default adapter", index);
        info.adapter = kDefaultAdapter;
        selectDefaultAdapter();
    }
}

void Context::selectDefaultAdapter() {
    gRenderLog.info("selecting default adapter");
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
    rtvHeap.newHeap(pDevice, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    frameData.resize(info.frames);

    for (UINT i = 0; i < info.frames; i++) {
        Heap::Index index = rtvHeap.alloc();
        ID3D12Resource *pRenderTarget = nullptr;
        HR_CHECK(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pRenderTarget)));
        pDevice->CreateRenderTargetView(pRenderTarget, nullptr, rtvHeap.cpuHandle(index));

        pRenderTarget->SetName(std::format(L"rtv#{}", i).c_str());

        frameData[i] = { .index = index, .pRenderTarget = pRenderTarget };
    }
}

void Context::deleteRenderTargets() {
    for (auto &frame : frameData) {
        RELEASE(frame.pRenderTarget);
    }

    rtvHeap.deleteHeap();
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
    cbvHeap.newHeap(pDevice, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Context::deleteDescriptorHeap() {
    cbvHeap.deleteHeap();
}

void Context::newCopyQueue() {
    copyQueue.newQueue(pDevice);
}

void Context::deleteCopyQueue() {
    copyQueue.deleteQueue();
}
