#include "render/render.h"

#include <array>
#include <DirectXMath.h>

namespace engine::render {
    namespace props {
        constexpr auto kDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        constexpr auto kUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    }

    constexpr float kClearColour[] = { 0.f, 0.2f, 0.4f, 1.f };
    constexpr float kBorderColour[] = { 0.f, 0.f, 0.f, 1.f };

    constexpr auto kSwapSampleCount = 1u;
    constexpr auto kSwapSampleQuality = 0u;

    struct ScreenVertex {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT2 texcoord;
    };

    constexpr auto kScreenVerts = std::to_array<ScreenVertex>({
        { { -1.f, -1.f, 0.f, 1.f }, { 0.f, 1.f } },
        { { -1.f,  1.f, 0.f, 1.f }, { 0.f, 0.f } },
        { {  1.f, -1.f, 0.f, 1.f }, { 1.f, 1.f } },
        { {  1.f,  1.f, 0.f, 1.f }, { 1.f, 0.f } }
    });

    void Context::createDevice() {
        /// create our context device
        device = getAdapter().newDevice<ID3D12Device4>(D3D_FEATURE_LEVEL_11_0, L"render-device");

        /// create our required command queues
        directCommandQueue = device.newCommandQueue(L"direct-command-queue", { D3D12_COMMAND_LIST_TYPE_DIRECT });
        copyCommandQueue = device.newCommandQueue(L"copy-command-queue", { D3D12_COMMAND_LIST_TYPE_COPY });
    
        cbvHeap = device.newHeap(L"cbv-heap", {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = requiredCbvHeapSize(),
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        });
    }

    void Context::destroyDevice() {
        cbvHeap.tryDrop("cbv-heap");

        directCommandQueue.tryDrop("direct-command-queue");
        copyCommandQueue.tryDrop("copy-command-queue");
        
        device.tryDrop("device");
    }

    void Context::createBuffers() {
        const auto [width, height] = displayResolution;
        const auto format = getFormat();

        const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,
            .Height = height,
            .Format = format,
            .SampleDesc = { kSwapSampleCount, kSwapSampleQuality },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = getBackBufferCount(),
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = getFactory().flags()
        };

        auto swapchain1 = getFactory().createSwapChain(directCommandQueue, getWindow().getHandle(), swapChainDesc);
    
        if (auto swapchain3 = swapchain1.as<IDXGISwapChain3>(); swapchain3) {
            swapchain = swapchain3;
        } else {
            throw engine::Error("failed to create swapchain");
        }

        frameIndex = swapchain->GetCurrentBackBufferIndex();

        rtvHeap = device.newHeap(L"rtv-heap", {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = requiredRtvHeapSize()
        });

        const auto targetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            format, width, height,
            1u, 1u, kSwapSampleCount, kSwapSampleQuality,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        );

        const auto clearValue = CD3DX12_CLEAR_VALUE(format, kClearColour);
        auto& target = getIntermediateTarget();

        HRESULT hr = device->CreateCommittedResource(
            &props::kDefault, D3D12_HEAP_FLAG_NONE,
            &targetDesc, D3D12_RESOURCE_STATE_PRESENT,
            &clearValue, IID_PPV_ARGS(target.addr())
        );
        check(hr, "failed to create intermediate target");

        device->CreateRenderTargetView(target.get(), nullptr, getIntermediateRtvHandle());
        device->CreateShaderResourceView(target.get(), nullptr, getIntermediateCbvHandle());
        target.rename(L"intermediate-target");
    }

    void Context::destroyBuffers() {
        auto target = getIntermediateTarget();
        target.tryDrop("intermediate-target");
        
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
    }

    void Context::buildViews() {
        // get our current internal and external resolution
        auto [displayWidth, displayHeight] = getWindow().getClientSize();
        auto [internalWidth, internalHeight] = getCurrentResolution();

        // calculate required scaling to prevent displaying
        // our drawn image outside the display area
        auto widthRatio = float(internalWidth) / float(displayWidth);
        auto heightRatio = float(internalHeight) / float(displayHeight);

        float x = 1.f;
        float y = 1.f;

        if (widthRatio < heightRatio) { x = widthRatio / heightRatio; } 
        else { y = heightRatio / widthRatio; }

        auto left = float(displayWidth) * (1.f - x) / 2.f;
        auto top = float(displayHeight) * (1.f - y) / 2.f;
        auto width = float(displayWidth * x);
        auto height = float(displayHeight * y);

        // set our viewport and scissor rects
        sceneView = View(left, top, width, height);
        postView = View(0.f, 0.f, float(internalWidth), float(internalHeight));
    
        displayResolution = Resolution(displayWidth, displayHeight);
        sceneResolution = Resolution(internalWidth, internalHeight);
    }
}
