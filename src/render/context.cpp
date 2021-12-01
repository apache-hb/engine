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
        ensure(
            D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)), 
            "d3d12-create-device"
        );
    
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT };

        ensure(
            device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)),
            "create-command-queue"
        );

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

        ensure(factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER), "make-window-association");
    }

    void Context::destroyDevice() {
        commandQueue.tryDrop("command-queue");
        device.tryDrop("device");
    }
}
