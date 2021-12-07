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
            renderTargets = new d3d12::Resource[frameCount];
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

            for (UINT i = 0; i < frameCount; i++) {
                if (HRESULT hr = swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])); FAILED(hr)) {
                    render->fatal("failed to get buffer\n{}", to_string(hr));
                    return hr;
                }

                device->CreateRenderTargetView(renderTargets[i].get(), nullptr, handle);

                handle.Offset(1, rtvDescriptorSize);
            }
        }

        if (HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)); FAILED(hr)) {
            render->fatal("failed to create command allocator\n{}", to_string(hr));
            return hr;
        }

        return S_OK;
    }

    void Context::destroyCore() {
        commandAllocator.tryDrop("command-allocator");
        
        UINT remaining = 0;
        for (UINT i = 0; i < frameCount; i++) {
            remaining = renderTargets[i].release(); // render targets all share a reference counter
        }
        delete[] renderTargets;
        
        if (remaining != 0) {
            render->fatal("failed to release all render targets, {} references remaining", remaining);
        }

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
        
        {
            CD3DX12_ROOT_SIGNATURE_DESC desc;
            desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            d3d::Blob signature;
            d3d::Blob error;

            if (HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error); FAILED(hr)) {
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
                { { 0.25f, 0.25f * ratio, 0.f }, { 1.f, 1.f, 1.f, 1.f } }
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

        if (HRESULT hr = createCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, pipelineState, &commandList); FAILED(hr)) {
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
        fenceValue = 1;

        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            render->fatal("failed to create fence event\n{}", to_string(hr));
            return hr;
        }

        ImGui::CreateContext();
        ImGui::GetIO();
        ImGui::StyleColorsDark();

        if (!ImGui_ImplWin32_Init(window->getHandle())) {
            render->fatal("failed to initialize win32 imgui\n");
            return E_FAIL;
        }

        if (!ImGui_ImplDX12_Init(
            device.get(), 
            frameCount,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            srvHeap.get(),
            srvHeap->GetCPUDescriptorHandleForHeapStart(),
            srvHeap->GetGPUDescriptorHandleForHeapStart()
        )) {
            render->fatal("failed to initialize d3d12 imgui\n");
            return E_FAIL;
        }

        return S_OK;
    }

    void Context::destroyAssets() {  
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        vertexBuffer.tryDrop("vertex-buffer");
        pipelineState.tryDrop("pipeline-state");
        rootSignature.tryDrop("root-signature");
        commandList.tryDrop("command-list");

        CloseHandle(fenceEvent);
        fence.tryDrop("fence");
    }

    HRESULT Context::present() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        if (HRESULT hr = populate(); FAILED(hr)) {
            return hr;
        }
        
        ID3D12CommandList *commands[] = { commandList.get() };
        commandQueue->ExecuteCommandLists(_countof(commands), commands);

        if (HRESULT hr = swapchain->Present(0, 0); FAILED(hr)) {
            return hr;
        }

        if (HRESULT hr = waitForFrame(); FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT Context::populate() {
        if (HRESULT hr = commandAllocator->Reset(); FAILED(hr)) {
            render->fatal("failed to reset command allocator\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = commandList->Reset(commandAllocator.get(), pipelineState.get()); FAILED(hr)) {
            render->fatal("failed to reset command list\n{}", to_string(hr));
            return hr;
        }

        commandList->SetGraphicsRootSignature(rootSignature.get());
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);

        const auto toTarget = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        const auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        commandList->ResourceBarrier(1, &toTarget);

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

        const float clear[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(handle, clear, 0, nullptr);
        commandList->OMSetRenderTargets(1, &handle, false, nullptr);
        commandList->SetDescriptorHeaps(1, &srvHeap);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.get());        
        
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->DrawInstanced(4, 1, 0, 0);

        commandList->ResourceBarrier(1, &toPresent);

        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            render->fatal("failed to close command list\n{}", to_string(hr));
            return hr;
        }

        return S_OK;
    }

    HRESULT Context::waitForFrame() {
        UINT64 value = fenceValue;
        if (HRESULT hr = commandQueue->Signal(fence.get(), value); FAILED(hr)) {
            render->fatal("failed to signal fence\n{}", to_string(hr));
            return hr;
        }
        fenceValue += 1;

        if (fence->GetCompletedValue() < value) {
            if (HRESULT hr = fence->SetEventOnCompletion(value, fenceEvent); FAILED(hr)) {
                render->fatal("failed to set fence event\n{}", to_string(hr));
                return hr;
            }
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        frameIndex = swapchain->GetCurrentBackBufferIndex();

        return S_OK;
    }

    bool Context::retry() {
        // retry 3 times total
        for (UINT i = 0; i < 3; i++) {
            destroyAssets();
            destroyCore();
            if (SUCCEEDED(createCore()) && SUCCEEDED(createAssets())) {
                return true;
            }
        }

        render->fatal("failed to recreate render context\n");
        return false;
    }

    Context createContext(Factory factory, system::Window *window, size_t adapter, UINT frameCount) {
        IMGUI_CHECKVERSION();

        return Context(factory, adapter, window, frameCount);
    }
}
