#include "context.h"

namespace engine::render {
    Context::Context(win32::WindowHandle *window, UINT frames) 
        : window(window)
        , frameCount(frames) 
    { }

    Context::~Context() {
        destroyDevice();
    }

    void Context::selectAdapter(size_t index) {
        adapterIndex = index;

        render->info("selecting adapter {}", index);

        createDevice();
    }

    
    void Context::setWindow(win32::WindowHandle *handle) {
        window = handle;
    }

    void Context::setFrameBuffering(UINT frames) {
        frameCount = frames;
    }

    void Context::createDevice() {
        render->check(
            adapterIndex != SIZE_MAX,
            engine::Error("creating device without a selected adapter")
        );

        auto adapter = currentAdapter();
        device = adapter.createDevice(D3D_FEATURE_LEVEL_11_0);
    
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT };

        ensure(
            device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)),
            "create-command-queue"
        );

        if (auto infoQueue = device.as<d3d12::InfoQueue1>(); infoQueue.valid()) {
            render->info("info queue detected");
            DWORD cookie = 0;
            auto infoQueueCallback = [](D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *ctx) {
                auto *log = reinterpret_cast<logging::Channel*>(ctx);
                log->warn("{}", desc);
            };

            ensure(
                infoQueue->RegisterMessageCallback(infoQueueCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, render, &cookie),
                "register-message-callback"
            );
            render->info("registered message callback with cookie {}", cookie);
        }
        

        auto handle = window->getHandle();
        auto [width, height] = window->getClientSize();

        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {
            .Width = UINT(width),
            .Height = UINT(height),
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { .Count = 1 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = frameCount,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
        };

        dxgi::SwapChain1 swapchain1;
        ensure(
            factory->CreateSwapChainForHwnd(
                commandQueue.get(),
                handle,
                &swapchainDesc,
                nullptr,
                nullptr,
                &swapchain1
            ),
            "create-swapchain-for-hwnd"
        );

        ensure(
            factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER), 
            "make-window-association"
        );
    
        ensure(
            (swapchain = swapchain1.as<dxgi::SwapChain3>()).valid(),
            "swapchain-as-swapchain3"
        );

        swapchain1.release();

        currentFrame = swapchain->GetCurrentBackBufferIndex();
    
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = frameCount
        };

        ensure(
            device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)),
            "create-rtv-descriptor-heap"
        );

        rtvHeapIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
        frameData = new FrameData[frameCount];

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < frameCount; i++) {
            auto *target = frameData + i;
            ensure(
                swapchain->GetBuffer(i, IID_PPV_ARGS(&target->renderTarget)),
                "get-swapchain-buffer"
            );
            device->CreateRenderTargetView(target->renderTarget.get(), nullptr, rtvHandle);

            rtvHandle.Offset(1, rtvHeapIncrement);

            ensure(
                device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&target->commandAllocator)),
                "create-command-allocator"
            );
            target->renderTarget.release();
        }
    }

    void Context::destroyDevice() {
        for (UINT i = 0; i < frameCount; i++) {
            frameData[i].renderTarget.tryDrop("render-target");
            frameData[i].commandAllocator.tryDrop("command-allocator");
        }
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
        commandQueue.tryDrop("command-queue");
        device.tryDrop("device");

        delete[] frameData;
    }
}
