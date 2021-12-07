#include "render.h"

namespace engine::render {
    HRESULT Context::createCore() {
        auto size = window->getClientSize();
        if (!size) { return E_FAIL; }

        auto [width, height] = size.value();
        auto handle = window->getHandle();

        auto adapter = getAdapter();
        if (HRESULT hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)); FAILED(hr)) {
            render->fatal("failed to create device\n{}", to_string(hr));
            return hr;
        }

        {
            auto info = device.as<d3d12::InfoQueue1>();
            if (info.valid()) {
                auto it = info.value();

                auto callback = [](auto category, auto severity, auto id, auto msg, auto ctx) {
                    if (id == D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE) { return; }

                    auto *log = reinterpret_cast<logging::Channel*>(ctx);
                    log->info("{}", msg);
                };

                DWORD cookie = 0;
                if (HRESULT hr = it->RegisterMessageCallback(callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, render, &cookie); FAILED(hr)) {
                    render->fatal("failed to register message callback\n{}", to_string(hr));
                    return hr;
                }
                render->info("registered message callback with cookie {}", cookie);
                it.release();
            }
        }

        {
            D3D12_COMMAND_QUEUE_DESC desc = {
                .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
            };
            if (HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)); FAILED(hr)) {
                render->fatal("failed to create command queue\n{}", to_string(hr));
                return hr;
            }
        }

        {
            dxgi::SwapChain1 swapchain1;
            DXGI_SWAP_CHAIN_DESC1 desc = {
                .Width = UINT(width),
                .Height = UINT(height),
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = { .Count = 1 },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = frameCount,
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
            };

            if (HRESULT hr = factory->CreateSwapChainForHwnd(commandQueue.get(), handle, &desc, nullptr, nullptr, &swapchain1); FAILED(hr)) {
                render->fatal("failed to create swap chain\n{}", to_string(hr));
                return hr;
            }

            if (HRESULT hr = factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER); FAILED(hr)) {
                render->fatal("failed to make window association\n{}", to_string(hr));
                return hr;
            }

            if (auto swap = swapchain1.as<dxgi::SwapChain3>(); !swap) {
                render->fatal("failed to create swap chain\n{}", to_string(swap.error()));
                return swap.error();
            } else {
                swapchain = swap.value();
                swapchain.release(); /// make sure only one reference is held
            }

            frameIndex = swapchain->GetCurrentBackBufferIndex();
        }

        scissor = d3d12::Scissor(LONG(width), LONG(height));
        viewport = d3d12::Viewport(FLOAT(width), FLOAT(height));

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                .NumDescriptors = frameCount
            };

            if (HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeap)); FAILED(hr)) {
                render->fatal("failed to create rtv heap\n{}", to_string(hr));
                return hr;
            }

            rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 1,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            };

            if (HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)); FAILED(hr)) {
                render->fatal("failed to create srv heap\n{}", to_string(hr));
                return hr;
            }
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 1,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            };

            if (HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&cbvHeap)); FAILED(hr)) {
                render->fatal("failed to create cbv heap\n{}", to_string(hr));
                return hr;
            }
        }

        {
            frameData = new FrameData[frameCount];
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

            for (UINT i = 0; i < frameCount; i++) {
                if (HRESULT hr = swapchain->GetBuffer(i, IID_PPV_ARGS(&frameData[i].renderTarget)); FAILED(hr)) {
                    render->fatal("failed to get buffer\n{}", to_string(hr));
                    return hr;
                }

                device->CreateRenderTargetView(frameData[i].renderTarget.get(), nullptr, handle);

                handle.Offset(1, rtvDescriptorSize);

                if (HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameData[i].commandAllocator)); FAILED(hr)) {
                    render->fatal("failed to create frame allocator {}\n{}", i, to_string(hr));
                    return hr;
                }
            }
        }

        return S_OK;
    }

    void Context::destroyCore() {
        UINT remaining = 0;
        for (UINT i = 0; i < frameCount; i++) {
            auto data = frameData[i];
            data.commandAllocator.tryDrop("frame-allocator");
            remaining = data.renderTarget.release(); // render targets all share a reference counter
        }
        delete[] frameData;
        
        if (remaining != 0) {
            render->fatal("failed to release all render targets, {} references remaining", remaining);
        }

        cbvHeap.tryDrop("cbv-heap");
        srvHeap.tryDrop("srv-heap");
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
        commandQueue.tryDrop("command-queue");
        device.tryDrop("device");
    }

    HRESULT Context::createCommandList(
        D3D12_COMMAND_LIST_TYPE type,
        d3d12::CommandAllocator allocator,
        d3d12::PipelineState state,
        ID3D12GraphicsCommandList **list) {
        HRESULT hr = device->CreateCommandList(0, type, allocator.get(), state.get(), IID_PPV_ARGS(list)); 
        
        if (FAILED(hr)) {
            render->fatal("failed to create command list\n{}", to_string(hr));
        }

        return hr;
    }

    HRESULT Context::createAssets() {
        auto version = [&] {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE features = {
                .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
            };

            if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &features, sizeof(features)))) {
                features.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            return features.HighestVersion;
        }();

        {
            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            CD3DX12_ROOT_PARAMETER1 parameters[1];

            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
            parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

            auto flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                         D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                         D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                         D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                         D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
            desc.Init_1_1(_countof(parameters), parameters, 0, nullptr, flags);

            d3d::Blob signature;
            d3d::Blob error;

            if (HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error); FAILED(hr)) {
                render->fatal("failed to serialize root signature\n{}\n{}", to_string(hr), (char*)error->GetBufferPointer());
                return hr;
            }

            if (HRESULT hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)); FAILED(hr)) {
                render->fatal("failed to create root signature\n{}", to_string(hr));
                return hr;
            }
        }

        {
            d3d::Blob vertexShader;
            d3d::Blob pixelShader;

            if (HRESULT hr = d3d::compileFromFile(L"shaders/shader.hlsl", "VSMain", "vs_5_0", &vertexShader); FAILED(hr)) {
                return hr;
            }

            if (HRESULT hr = d3d::compileFromFile(L"shaders/shader.hlsl", "PSMain", "ps_5_0", &pixelShader); FAILED(hr)) {
                return hr;
            }

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
                .SampleDesc = { 1 }
            };

            if (HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState)); FAILED(hr)) {
                render->fatal("failed to create pipeline state\n{}", to_string(hr));
                return hr;
            }
        }

        {
            auto aspect = window->getClientAspectRatio();
            if (!aspect) { return HRESULT_FROM_WIN32(aspect.error()); }
            auto ratio = aspect.value();
            Vertex verts[] = {
                { { 0.f, 0.25f * ratio, 0.f }, { 1.f, 0.f, 0.f, 1.f } },
                { { 0.25f, -0.25f * ratio, 0.f }, { 0.f, 1.f, 0.f, 1.f } },
                { { -0.25f, -0.25f * ratio, 0.f }, { 0.f, 0.f, 1.f, 1.f } },
            };

            const UINT vertexBufferSize = sizeof(verts);

            const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

            if (HRESULT hr = device->CreateCommittedResource(
                &upload, D3D12_HEAP_FLAG_NONE,
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&vertexBuffer)); FAILED(hr)) {
                render->fatal("failed to create vertex buffer\n{}", to_string(hr));
                return hr;
            }

            void *vertexData;
            CD3DX12_RANGE readRange(0, 0);
            if (HRESULT hr = vertexBuffer->Map(0, &readRange, &vertexData); FAILED(hr)) {
                render->fatal("failed to map vertex buffer\n{}", to_string(hr));
                return hr;
            }
            memcpy(vertexData, verts, vertexBufferSize);
            vertexBuffer->Unmap(0, nullptr);

            vertexBufferView = {
                .BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = vertexBufferSize,
                .StrideInBytes = sizeof(Vertex)
            };
        }
        
        {
            const UINT constBufferSize = sizeof(ConstBuffer);
            const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(constBufferSize);

            if (HRESULT hr = device->CreateCommittedResource(
                &upload, D3D12_HEAP_FLAG_NONE,
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&constBuffer)); FAILED(hr)) {
                render->fatal("failed to create const buffer\n{}", to_string(hr));
                return hr;
            }

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
                .BufferLocation = constBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = constBufferSize
            };

            device->CreateConstantBufferView(&desc, cbvHeap->GetCPUDescriptorHandleForHeapStart());

            CD3DX12_RANGE readRange(0, 0);
            if (HRESULT hr = constBuffer->Map(0, &readRange, &constBufferPtr); FAILED(hr)) {
                render->fatal("failed to map const buffer\n{}", to_string(hr));
                return hr;
            }

            memcpy(constBufferPtr, &constBufferData, sizeof(ConstBuffer));
        }

        if (HRESULT hr = createCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, frameData[frameIndex].commandAllocator.get(), pipelineState, &commandList); FAILED(hr)) {
            render->fatal("failed to create command list\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            render->fatal("failed to close command list\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)); FAILED(hr)) {
            render->fatal("failed to create fence\n{}", to_string(hr));
            return hr;
        }
        frameData[frameIndex].fenceValue = 1;

        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            render->fatal("failed to create fence event\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = waitForGpu(); FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    void Context::destroyAssets() {  
        waitForGpu();

        constBuffer.tryDrop("const-buffer");
        vertexBuffer.tryDrop("vertex-buffer");
        pipelineState.tryDrop("pipeline-state");
        rootSignature.tryDrop("root-signature");
        commandList.tryDrop("command-list");

        CloseHandle(fenceEvent);
        fence.tryDrop("fence");
    }

    HRESULT Context::present() {
        memcpy(constBufferPtr, &constBufferData, sizeof(ConstBuffer));

        if (HRESULT hr = populate(); FAILED(hr)) {
            return hr;
        }
        
        ID3D12CommandList *commands[] = { commandList.get() };
        commandQueue->ExecuteCommandLists(_countof(commands), commands);

        if (HRESULT hr = swapchain->Present(1, 0); FAILED(hr)) {
            return hr;
        }

        if (HRESULT hr = nextFrame(); FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT Context::populate() {
        auto allocator = frameData[frameIndex].commandAllocator;
        if (HRESULT hr = allocator->Reset(); FAILED(hr)) {
            render->fatal("failed to reset command allocator\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = commandList->Reset(allocator.get(), pipelineState.get()); FAILED(hr)) {
            render->fatal("failed to reset command list\n{}", to_string(hr));
            return hr;
        }

        commandList->SetGraphicsRootSignature(rootSignature.get());
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);

        auto renderTarget = frameData[frameIndex].renderTarget.get();

        const auto toTarget = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTarget,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        const auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTarget,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        commandList->ResourceBarrier(1, &toTarget);

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

        const float clear[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(handle, clear, 0, nullptr);
        commandList->OMSetRenderTargets(1, &handle, false, nullptr);
        commandList->SetDescriptorHeaps(1, &cbvHeap);
        commandList->SetGraphicsRootDescriptorTable(0, cbvHeap->GetGPUDescriptorHandleForHeapStart());

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->DrawInstanced(3, 1, 0, 0);

        commandList->ResourceBarrier(1, &toPresent);

        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            render->fatal("failed to close command list\n{}", to_string(hr));
            return hr;
        }

        return S_OK;
    }

    HRESULT Context::nextFrame() {
        auto& value = frameData[frameIndex].fenceValue;
        if (HRESULT hr = commandQueue->Signal(fence.get(), value); FAILED(hr)) {
            render->fatal("failed to signal fence\n{}", to_string(hr));
            return hr;
        }

        frameIndex = swapchain->GetCurrentBackBufferIndex();

        auto current = frameData[frameIndex].fenceValue;

        if (fence->GetCompletedValue() < current) {
            if (HRESULT hr = fence->SetEventOnCompletion(value, fenceEvent); FAILED(hr)) {
                render->fatal("failed to set fence event\n{}", to_string(hr));
                return hr;
            }
            WaitForSingleObjectEx(fenceEvent, INFINITE, false);
        }

        value = current + 1;

        return S_OK;
    }

    HRESULT Context::waitForGpu() {
        UINT64 &value = frameData[frameIndex].fenceValue;
        if (HRESULT hr = commandQueue->Signal(fence.get(), value); FAILED(hr)) {
            render->fatal("failed to signal fence\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = fence->SetEventOnCompletion(value, fenceEvent); FAILED(hr)) {
            render->fatal("failed to set fence event\n{}", to_string(hr));
            return hr;
        }
        WaitForSingleObject(fenceEvent, INFINITE);

        value += 1;

        return S_OK;
    }

    bool Context::retry(size_t attempts) {
        for (size_t i = 0; i < attempts; i++) {
            destroyAssets();
            destroyCore();
            if (SUCCEEDED(createCore()) && SUCCEEDED(createAssets())) {
                return true;
            }
        }

        render->fatal("failed to recreate render context after {} attempts", attempts);
        return false;
    }

    Context createContext(Factory factory, system::Window *window, size_t adapter, UINT frameCount) {
        return Context(factory, adapter, window, frameCount);
    }
}
