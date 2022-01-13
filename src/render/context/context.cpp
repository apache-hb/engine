#include "render/render.h"

#include "render/objects/factory.h"

#include "render/viewport/viewport.h"
#include "render/scene/scene.h"

using namespace engine::render;

Context::Context(Create&& create): info(create) {
    display = info.getViewport(this);
    scene = info.getScene(this);
}

void Context::create() {
    createDevice(); // create our render context
    
    // attempt to attach to ID3D12InfoQueue1 to report using our own framework
    // this may fail if we're not running on windows 11
    attachInfoQueue();

    createViews(); // calculate our viewport and scissor rects
    createDirectCommandQueue(); // create our direct command queue to present with
    createUploadCommandQueue(); // create our upload command queue to copy with
    createSwapChain(); // create our swap chain to present to an hwnd with

    // create our rtv heap to hold our swap chain buffers
    // as well as our intermediate scene target
    createRtvHeap();

    // create our constant buffers to store most shader visible data
    // this includes our intermediate scene target
    createCbvHeap();

    // create our intermediate render target to render our scenes to
    // this allows us to do resolution scaling and post processing
    createSceneTarget();

    // create our per-backbuffer data
    createFrameData();

    // create our fence for synchronization
    createFence();

    // create our command lists
    createCopyCommandList();

    createCopyFence();

    display->create();
    scene->create();

    finishCopy();
    waitForGpu();
}

void Context::destroy() {
    waitForGpu();

    scene->destroy();
    display->destroy();

    destroyCopyFence();
    destroyCopyCommandList();
    destroyFence();
    destroyFrameData();
    destroySceneTarget();
    destroyRtvHeap();
    destroyCbvHeap();
    destroySwapChain();
    destroyUploadCommandQueue();
    destroyDirectCommandQueue();
    destroyDevice();
}

constexpr auto kFilterFlags = D3D12_MESSAGE_CALLBACK_FLAG_NONE;
constexpr D3D12MessageFunc kCallback = [](UNUSED auto category, auto severity, UNUSED auto id, auto desc, auto* user) {
    auto* channel = reinterpret_cast<engine::log::Channel*>(user);

    switch (severity) {
    case D3D12_MESSAGE_SEVERITY_INFO:
        channel->info("{}", desc);
        break;
    case D3D12_MESSAGE_SEVERITY_WARNING:
        channel->warn("{}", desc);
        break;
    default:
        channel->fatal("{}", desc);
        break;
    }
};

D3D12_MESSAGE_ID kFilters[] = {
    D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
};

D3D12_INFO_QUEUE_FILTER kFilter = {
    .DenyList = {
        .NumIDs = UINT(std::size(kFilters)),
        .pIDList = kFilters
    }
};

void Context::attachInfoQueue() {
    auto infoQueue = getDevice().as<ID3D12InfoQueue1>();
    if (!infoQueue) { return; }

    DWORD cookie = 0;

    HRESULT hrCallback = infoQueue->RegisterMessageCallback(kCallback, kFilterFlags, log::render, &cookie);

    if (FAILED(hrCallback)) {
        log::render->warn("failed to attach to ID3D12InfoQueue1");
    } else {
        log::render->info("attached to ID3D12InfoQueue1");
    }

    HRESULT hrFilter = infoQueue->AddStorageFilterEntries(&kFilter);
    
    if (FAILED(hrFilter)) {
        log::render->warn("failed to add storage filter entries");
    } else {
        log::render->info("added storage filter entries");
    }
}

void Context::bindSrv(Resource resource, CD3DX12_CPU_DESCRIPTOR_HANDLE handle) {
    getDevice()->CreateShaderResourceView(resource.get(), nullptr, handle);
}

void Context::bindCbv(Resource resource, UINT size, CD3DX12_CPU_DESCRIPTOR_HANDLE handle) {
    D3D12_CONSTANT_BUFFER_VIEW_DESC view = {
        .BufferLocation = resource.gpuAddress(),
        .SizeInBytes = size
    };
    
    getDevice()->CreateConstantBufferView(&view, handle);
}

bool Context::present() {
    try {
        auto viewList = display->populate();
        auto sceneList = scene->populate();

        ID3D12CommandList* lists[] = { sceneList, viewList };
        directCommandQueue->ExecuteCommandLists(UINT(std::size(lists)), lists);

        auto interval = info.vsync ? 1 : 0;
        auto flags = info.vsync ? 0 : getFactory().presentFlags();

        check(swapchain->Present(interval, flags), "Present failed");

        nextFrame();
    } catch (...) {
        return false;
    }

    return true;
}
