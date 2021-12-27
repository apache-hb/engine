#include "render/render.h"

#include "render/objects/signature.h"

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

    namespace post {
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

        constexpr auto kRanges = std::to_array({
            srvRange(0, 1)
        });

        constexpr auto kParams = std::to_array({
            tableParameter(D3D12_SHADER_VISIBILITY_PIXEL, kRanges)
        });

        constexpr D3D12_STATIC_SAMPLER_DESC kSamplers[] = {{
            .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            .MinLOD = 0.0f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = 0,
            .RegisterSpace = 0,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
        }};

        constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags = 
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        constexpr auto kInput = std::to_array({
            shaderInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, offsetof(ScreenVertex, position)),
            shaderInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, offsetof(ScreenVertex, texcoord))
        });

        constexpr ShaderLibrary::CompileCreate kShaders = {
            .path = L"resources\\shaders\\post-shader.hlsl",
            .vsMain = "vsMain",
            .psMain = "psMain",
            .layout = { kInput }
        };

        constexpr root::Create kRoot = {
            .params = { kParams },
            .samplers = { kSamplers },
            .flags = kFlags
        };
    }



    void Context::createContext() {
        shaders = ShaderLibrary(post::kShaders);
    }

    void Context::destroyContext() {

    }



    void Context::createDevice() {
        /// create our context device
        device = getAdapter().newDevice<ID3D12Device4>(D3D_FEATURE_LEVEL_11_0, L"render-device");

        /// create our required command queues
        directCommandQueue = device.newCommandQueue(L"direct-command-queue", { commands::kDirect });

        cbvHeap = device.newHeap(L"cbv-heap", {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = requiredCbvHeapSize(),
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        });
    }

    void Context::destroyDevice() {
        cbvHeap.tryDrop("cbv-heap");

        directCommandQueue.tryDrop("direct-command-queue");
        
        device.tryDrop("device");
    }



    void Context::createBuffers() {
        const auto [width, height] = displayResolution;
        const auto format = getFormat();
        const auto bufferCount = getBackBufferCount();

        const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,
            .Height = height,
            .Format = format,
            .SampleDesc = { kSwapSampleCount, kSwapSampleQuality },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = bufferCount,
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
            &targetDesc, D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue, IID_PPV_ARGS(target.addr())
        );
        check(hr, "failed to create intermediate target");

        device->CreateRenderTargetView(target.get(), nullptr, getIntermediateRtvHandle());
        device->CreateShaderResourceView(target.get(), nullptr, getIntermediateCbvCpuHandle());
        target.rename(L"intermediate-target");

        frameData = new FrameData[bufferCount];
        for (UINT i = 0; i < bufferCount; i++) {
            auto *frame = frameData + i;

            check(swapchain->GetBuffer(i, IID_PPV_ARGS(&frame->target)), "failed to get swapchain buffer");
            device->CreateRenderTargetView(frame->target.get(), nullptr, getRenderTargetCpuHandle(i));
            frame->target.rename(std::format(L"target-{}", i));

            frame->fenceValue = 0;

            for (auto alloc = 0; alloc < Allocator::Total; alloc++) {
                auto index = Allocator::Index(alloc);
                const auto name = std::format(L"allocator-{}-{}", Allocator::name(index), i);
                frame->allocators[alloc] = device.newAllocator(name, Allocator::type(index));
            }
        }

        check(device->CreateFence(frameData[getCurrentFrameIndex()].fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "failed to create fence");
        frameData[getCurrentFrameIndex()].fenceValue = 1;
        if ((fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr) {
            throw win32::Error("failed to create fence event");
        }
    }

    void Context::destroyBuffers() {
        const auto bufferCount = getBackBufferCount();
        for (UINT i = 0; i < bufferCount; i++) {
            auto *frame = frameData + i;

            // all back buffers share the same ref count
            // release them instead of dropping them
            frame->target.release();

            for (auto alloc = 0; alloc < Allocator::Total; alloc++) {
                frame->allocators[alloc].tryDrop("allocator");
            }
        }

        delete[] frameData;

        auto target = getIntermediateTarget();
        target.tryDrop("intermediate-target");
        
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
    }



    void Context::createPipeline() {
        screenBuffer = uploadVertexBuffer<post::ScreenVertex>(L"screen-quad", { post::kScreenVerts });


        copyCommands.close();
        auto commands = std::to_array({ copyCommands });
        copyCommandQueue.execute(commands);
        waitForGpu(copyCommandQueue);


        rootSignature = device.newRootSignature(L"screen-root", post::kRoot);
        pipelineState = device.newGraphicsPSO(L"screen-pso", shaders, rootSignature);

        directCommands = device.newCommandList(L"direct-commands", getAllocator(Allocator::Direct), pipelineState, commands::kDirect);
        directCommands.close();
    }

    void Context::destroyPipeline() {
        waitForGpu(directCommandQueue);

        screenBuffer.resource.tryDrop("screen-quad");
        rootSignature.tryDrop("root-signature");
        pipelineState.tryDrop("pipeline-state");
        directCommands.tryDrop("direct-commands");

        fence.tryDrop("fence");
        CloseHandle(fenceEvent);
    }



    void Context::createCopyPipeline() {
        copyCommandQueue = device.newCommandQueue(L"copy-command-queue", { commands::kCopy });
        copyCommands = device.newCommandList(L"copy-commands", getAllocator(Allocator::Copy), pipelineState, commands::kCopy);
    }

    void Context::destroyCopyPipeline() {
        for (auto& resource : copyResources) {
            resource.tryDrop("copy-resource");
        }

        copyCommands.tryDrop("copy-commands");
        copyCommandQueue.tryDrop("copy-command-queue");
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
