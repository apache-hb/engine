#include "render.h"

namespace engine::render {
    void Context::createDevice(Context::Create& info) {
        create = info;

        auto adapter = create.adapter;
        auto window = create.window;
        auto frameCount = create.frames;

        auto [width, height] = window->getClientSize();
        auto hwnd = window->getHandle();

        check(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)), "failed to create device");
    
        device.rename(L"d3d12-device");

        attachInfoQueue();

        {
            D3D12_COMMAND_QUEUE_DESC desc = {
                .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
                .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
            };

            check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)), "failed to create command queue");

            queue.rename(L"d3d12-queue");
        }

        {
            DXGI_SWAP_CHAIN_DESC1 desc = {
                .Width = UINT(width),
                .Height = UINT(height),
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = { .Count = 1 },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = frameCount,
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
                .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
            };

            auto swapchain1 = factory.createSwapChain(queue, hwnd, desc);

            if (!(swapchain = swapchain1.as<IDXGISwapChain3>()).valid()) {
                throw engine::Error("failed to create swapchain");
            }

            swapchain.release();

            frameIndex = swapchain->GetCurrentBackBufferIndex();
        }

        scene.scissor = d3d12::Scissor(width, height);
        scene.viewport = d3d12::Viewport(FLOAT(width), FLOAT(height));
    
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                .NumDescriptors = frameCount
            };
            
            check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeap)), "failed to create RTV descriptor heap");

            rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            rtvHeap.rename(L"d3d12-rtv-heap");
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 1,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            };

            check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&cbvHeap)), "failed to create CBV descriptor heap");

            cbvHeap.rename(L"d3d12-cbv-heap");
        }

        frames = new Frame[frameCount];
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

            for (UINT i = 0; i < frameCount; i++) {
                auto &frame = frames[i];
                
                check(swapchain->GetBuffer(i, IID_PPV_ARGS(&frame.target)), "failed to get swapchain buffer");
                device->CreateRenderTargetView(frame.target.get(), nullptr, handle);
                
                check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.allocator)), "failed to create command allocator");

                handle.Offset(1, rtvDescriptorSize);
                frame.fenceValue = 0;

                frame.allocator.rename(std::format(L"frame-allocator-{}", i));
                frame.target.rename(std::format(L"frame-target-{}", i));
            }
        }
    }

    void Context::destroyDevice() {
        for (UINT i = 0; i < create.frames; i++) {
            auto frame = frames[i];
            frame.target.release();
            frame.allocator.tryDrop("allocator");
        }
        delete[] frames;

        cbvHeap.tryDrop("cbv-heap");
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
        queue.tryDrop("queue");
        device.tryDrop("device");
    }

    void Context::createAssets() {
        auto version = rootVersion();
        {
            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
            desc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        
            Com<ID3DBlob> signature;
            Com<ID3DBlob> error;

            HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error);
            if (FAILED(hr)) {
                throw render::Error(hr, (char*)error->GetBufferPointer());
            }

            check(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)), "failed to create root signature");
        
            rootSignature.rename(L"d3d12-root-signature");
        }

        {
            auto vertexShader = compileShader(L"shaders\\shader.hlsl", "VSMain", "vs_5_0");
            auto pixelShader = compileShader(L"shaders\\shader.hlsl", "PSMain", "ps_5_0");
        
            D3D12_INPUT_ELEMENT_DESC inputs[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
                .pRootSignature = rootSignature.get(),
                .VS = CD3DX12_SHADER_BYTECODE(vertexShader.get()),
                .PS = CD3DX12_SHADER_BYTECODE(pixelShader.get()),
                .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
                .SampleMask = UINT_MAX,
                .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
                .InputLayout = { inputs, _countof(inputs) },
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
                .DSVFormat = DXGI_FORMAT_D32_FLOAT,
                .SampleDesc = { 1 }
            };

            check(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState)), "failed to create graphics pipeline state");
        
            pipelineState.rename(L"d3d12-pipeline-state");
        }

        check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getAllocator().get(), pipelineState.get(), IID_PPV_ARGS(&commandList)), "failed to create command list");
        check(commandList->Close(), "failed to close command list");

        commandList.rename(L"d3d12-command-list");

        const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        {
            auto aspect = create.window->getClientAspectRatio();
            Vertex verts[] = {
                { { -0.25f, 0.25f * aspect, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // top left
                { { 0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }, // bottom right
                { { -0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }, // bottom left
                { { 0.25f, 0.25f * aspect, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } } // top right
            };

            const UINT vertexBufferSize = sizeof(verts);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

            HRESULT hr = device->CreateCommittedResource(
                &upload, D3D12_HEAP_FLAG_NONE,
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&vertexBuffer)
            );
            check(hr, "failed to create vertex buffer");

            void *vertexBufferPtr;
            CD3DX12_RANGE readRange(0, 0);
            check(vertexBuffer->Map(0, &readRange, &vertexBufferPtr), "failed to map vertex buffer");
            memcpy(vertexBufferPtr, verts, vertexBufferSize);
            vertexBuffer->Unmap(0, nullptr);

            vertexBufferView = {
                .BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = vertexBufferSize,
                .StrideInBytes = sizeof(Vertex)
            };

            vertexBuffer.rename(L"d3d12-vertex-buffer");
        }

        {
            DWORD indicies[] = {
                0, 1, 2, // first triangle
                0, 3, 1 // second triangle
            };

            UINT indexBufferSize = sizeof(indicies);
            const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

            HRESULT hr = device->CreateCommittedResource(
                &upload, D3D12_HEAP_FLAG_NONE,
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&indexBuffer)
            );
            check(hr, "failed to create index buffer");

            void *indexBufferPtr;
            CD3DX12_RANGE readRange(0, 0);
            check(indexBuffer->Map(0, &readRange, &indexBufferPtr), "failed to map index buffer");
            memcpy(indexBufferPtr, indicies, indexBufferSize);
            indexBuffer->Unmap(0, nullptr);

            indexBufferView = {
                .BufferLocation = indexBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = indexBufferSize,
                .Format = DXGI_FORMAT_R32_UINT
            };
        }

        check(device->CreateFence(frames[frameIndex].fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "failed to create fence");
        frames[frameIndex].fenceValue = 1;
        if ((fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr) {
            throw win32::Error("failed to create fence event");
        }

        waitForGPU();
    }

    void Context::destroyAssets() {
        waitForGPU();

        indexBuffer.tryDrop("index-buffer");
        vertexBuffer.tryDrop("vertex-buffer");
        rootSignature.tryDrop("root-signature");
        pipelineState.tryDrop("pipeline-state");
        commandList.tryDrop("command-list");

        CloseHandle(fenceEvent);
        fence.tryDrop("fence");
    }

    void Context::populate() {
        auto alloc = getAllocator();
        check(alloc->Reset(), "failed to reset command allocator");
        check(commandList->Reset(alloc.get(), pipelineState.get()), "failed to reset command list");
    
        auto target = frames[frameIndex].target.get();
        const auto toTarget = CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        const auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

        commandList->SetGraphicsRootSignature(rootSignature.get());
        commandList->RSSetViewports(1, &scene.viewport);
        commandList->RSSetScissorRects(1, &scene.scissor);

        commandList->ResourceBarrier(1, &toTarget);

        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        const float clear[] = { 0.f, 0.2f, 0.4f, 1.f };
        commandList->ClearRenderTargetView(rtvHandle, clear, 0, nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->IASetIndexBuffer(&indexBufferView);
        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

        commandList->ResourceBarrier(1, &toPresent);

        check(commandList->Close(), "failed to close command list");
    }

    void Context::present() {
        populate();

        ID3D12CommandList* lists[] = { commandList.get() };
        queue->ExecuteCommandLists(1, lists);

        check(swapchain->Present(0, factory.presentFlags()), "failed to present swapchain");

        nextFrame();
    }

    void Context::waitForGPU() {
        UINT64 &value = frames[frameIndex].fenceValue;
        check(queue->Signal(fence.get(), value), "failed to signal fence");

        check(fence->SetEventOnCompletion(value, fenceEvent), "failed to set fence event");
        WaitForSingleObject(fenceEvent, INFINITE);

        value += 1;
    }

    void Context::nextFrame() {
        UINT64 value = frames[frameIndex].fenceValue;
        check(queue->Signal(fence.get(), value), "failed to signal fence");
        
        frameIndex = swapchain->GetCurrentBackBufferIndex();

        auto &current = frames[frameIndex].fenceValue;

        if (fence->GetCompletedValue() < current) {
            check(fence->SetEventOnCompletion(current, fenceEvent), "failed to set fence event");
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        current = value + 1;
    }

    auto callback = [](auto category, auto severity, auto id, auto msg, auto ctx) {
        if (id == D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE) { return; }

        auto *log = reinterpret_cast<log::Channel*>(ctx);

        switch (severity) {
        case D3D12_MESSAGE_SEVERITY_MESSAGE:
        case D3D12_MESSAGE_SEVERITY_INFO:
            log->info("{}", msg);
            break;
        case D3D12_MESSAGE_SEVERITY_WARNING:
            log->warn("{}", msg);
            break;
        case D3D12_MESSAGE_SEVERITY_ERROR:
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
            log->fatal("{}", msg);
            break;

        default:
            log->fatal("{}", msg);
            break;
        }
    };

    void Context::attachInfoQueue() {
        auto infoQueue = device.as<ID3D12InfoQueue1>();
        if (!infoQueue.valid()) { 
            log::render->warn("failed to find info queue interface");
            return;
        }

        DWORD cookie = 0;
        auto flags = D3D12_MESSAGE_CALLBACK_FLAG_NONE;
        check(infoQueue->RegisterMessageCallback(callback, flags, log::render, &cookie), "failed to register message callback");
    
        log::render->info("registered message callback with cookie {}", cookie);
        infoQueue.release();
    }

    D3D_ROOT_SIGNATURE_VERSION Context::rootVersion() {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE features = { .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1 };

        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &features, sizeof(features)))) {
            features.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        return features.HighestVersion;
    }
}
