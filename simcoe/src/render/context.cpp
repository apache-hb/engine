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

void Fence::newFence(ID3D12Device *pDevice) {
    HR_CHECK(pDevice->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hEvent == nullptr) {
        HR_CHECK(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void Fence::deleteFence() {
    CloseHandle(hEvent);
    RELEASE(pFence);
}

void Fence::wait(ID3D12CommandQueue *pQueue) {
    HR_CHECK(pQueue->Signal(pFence, fenceValue));

    if (pFence->GetCompletedValue() < fenceValue) {
        HR_CHECK(pFence->SetEventOnCompletion(fenceValue, hEvent));
        WaitForSingleObject(hEvent, INFINITE);
    }

    fenceValue += 1;
}

void CommandBuffer::newCommandBuffer(ID3D12Device *pDevice, D3D12_COMMAND_LIST_TYPE type) {
    HR_CHECK(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&pAllocator)));
    HR_CHECK(pDevice->CreateCommandList(0, type, pAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));

    fence.newFence(pDevice);
}

void CommandBuffer::deleteCommandBuffer() {
    fence.deleteFence();

    RELEASE(pAllocator);
    RELEASE(pCommandList);
}

void CommandBuffer::execute(ID3D12CommandQueue *pQueue) {
    HR_CHECK(pCommandList->Close());

    ID3D12CommandList *ppCommandLists[] = { pCommandList };
    pQueue->ExecuteCommandLists(UINT(std::size(ppCommandLists)), ppCommandLists);
    fence.wait(pQueue);

    HR_CHECK(pAllocator->Reset());
    HR_CHECK(pCommandList->Reset(pAllocator, nullptr));
}

ID3D12Resource *Context::newBuffer(
    size_t size, 
    const D3D12_HEAP_PROPERTIES *pProps, 
    D3D12_RESOURCE_STATES state,
    D3D12_HEAP_FLAGS flags
) 
{
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    ID3D12Resource *pResource = nullptr;
    HR_CHECK(pDevice->CreateCommittedResource(
        pProps,
        flags,
        &desc,
        state,
        nullptr,
        IID_PPV_ARGS(&pResource)
    ));

    return pResource;
}

CommandBuffer Context::newCommandBuffer(D3D12_COMMAND_LIST_TYPE type) {
    CommandBuffer buffer;
    buffer.newCommandBuffer(pDevice, type);
    return buffer;
}

void Context::submitDirectCommands(CommandBuffer& buffer) {
    buffer.execute(pDirectQueue);
}

void Context::submitCopyCommands(CommandBuffer& buffer) {
    buffer.execute(pCopyQueue);
}

Context::Context(system::Window &window, const Info& info) 
    : window(window)
    , info(info)
    , rtvHeap(getRenderHeapSize())
    , cbvHeap(info.heapSize)
{
    newFactory();
    newDevice();
    newInfoQueue();
    newDirectQueue();
    newCopyQueue();
    newSwapChain();
    newRenderTargets();
    newDescriptorHeap();
    newFence();
}

Context::~Context() {
    deleteFence();
    deleteDescriptorHeap();
    deleteRenderTargets();
    deleteSwapChain();
    deleteCopyQueue();
    deleteDirectQueue();
    deleteInfoQueue();
    deleteDevice();
    deleteFactory();
}

void Context::beginRender() {
    // populate command list
    HR_CHECK(directCommandAllocators[frameIndex]->Reset());
    HR_CHECK(pDirectCommandList->Reset(directCommandAllocators[frameIndex], nullptr));
    
    ID3D12DescriptorHeap *ppHeaps[] = { cbvHeap.getHeap() };
    
    pDirectCommandList->SetDescriptorHeaps(UINT(std::size(ppHeaps)), ppHeaps);
}

void Context::endRender() {
    HR_CHECK(pDirectCommandList->Close());

    // execute command list
    ID3D12CommandList* ppCommandLists[] = { pDirectCommandList };
    pDirectQueue->ExecuteCommandLists(UINT(std::size(ppCommandLists)), ppCommandLists);
}

void Context::beginCopy() {
    HR_CHECK(pCopyAllocator->Reset());
    HR_CHECK(pCopyCommandList->Reset(pCopyAllocator, nullptr));
}

void Context::endCopy() {
    HR_CHECK(pCopyCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = { pCopyCommandList };
    pCopyQueue->ExecuteCommandLists(UINT(std::size(ppCommandLists)), ppCommandLists);

    // wait for copy queue to finish
    copyFence.wait(pCopyQueue);
}

void Context::present() {
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
    selectAdapter(info.adapter);

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
            DoOnceGroup& once = *reinterpret_cast<DoOnceGroup*>(pUser);

            once(id, [&]{
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
        HR_CHECK(pInfoQueue->RegisterMessageCallback(pfn, D3D12_MESSAGE_CALLBACK_FLAG_NONE, &reportOnce, &cookie));

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
    if (index != kDefaultAdapter) {
        gRenderLog.info("selecting adapter #{}", index);
        HRESULT hr = pFactory->EnumAdapters1(UINT(index), &pAdapter);
        if (SUCCEEDED(hr)) {
            return;
        }

        gRenderLog.warn("adapter #{} not found", index);
        info.adapter = kDefaultAdapter;
    }

    selectDefaultAdapter();
}

void Context::selectDefaultAdapter() {
    gRenderLog.info("selecting default adapter");
    HR_CHECK(pFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)));
    ASSERT(pAdapter != nullptr);
}

void Context::newDirectQueue() {
    directCommandAllocators.resize(info.frames);
    
    const D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
    };

    for (UINT i = 0; i < info.frames; i++) {
        HR_CHECK(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&directCommandAllocators[i])));
    }

    HR_CHECK(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pDirectQueue)));
    HR_CHECK(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, directCommandAllocators[0], nullptr, IID_PPV_ARGS(&pDirectCommandList)));
    HR_CHECK(pDirectCommandList->Close());
}

void Context::deleteDirectQueue() {
    RELEASE(pDirectCommandList);

    for (auto &pCommandAllocator : directCommandAllocators) {
        RELEASE(pCommandAllocator);
    }

    RELEASE(pDirectQueue);
}

void Context::newCopyQueue() {
    const D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COPY
    };

    HR_CHECK(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&pCopyAllocator)));
    HR_CHECK(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pCopyQueue)));
    HR_CHECK(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, pCopyAllocator, nullptr, IID_PPV_ARGS(&pCopyCommandList)));
    HR_CHECK(pCopyCommandList->Close());
}

void Context::deleteCopyQueue() {
    RELEASE(pCopyCommandList);
    RELEASE(pCopyAllocator);
    RELEASE(pCopyQueue);
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
    HR_CHECK(pFactory->CreateSwapChainForHwnd(pDirectQueue, window.getHandle(), &desc, nullptr, nullptr, &pSwapChain1));
    HR_CHECK(pFactory->MakeWindowAssociation(window.getHandle(), DXGI_MWA_NO_ALT_ENTER));

    HR_CHECK(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain)));
    RELEASE(pSwapChain1);
}

void Context::deleteSwapChain() {
    RELEASE(pSwapChain);
}

void Context::newRenderTargets() {
    rtvHeap.newHeap(pDevice, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Context::deleteRenderTargets() {
    rtvHeap.deleteHeap();
}

void Context::newFence() {
    presentFence.newFence(pDevice);
    copyFence.newFence(pDevice);

    waitForFence();
}

void Context::deleteFence() {
    presentFence.deleteFence();
    copyFence.deleteFence();
}

void Context::waitForFence() {
    presentFence.wait(pDirectQueue);
    copyFence.wait(pCopyQueue);
}

void Context::nextFrame() {
    frameIndex = pSwapChain->GetCurrentBackBufferIndex();

    presentFence.wait(pDirectQueue);
}

void Context::newDescriptorHeap() {
    cbvHeap.newHeap(pDevice, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Context::deleteDescriptorHeap() {
    cbvHeap.deleteHeap();
}
