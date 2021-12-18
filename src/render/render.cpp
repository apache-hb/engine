#include "render.h"

#include "assets/loader.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

namespace engine::render {
    void Context::createDevice(Context::Create& info) {
        create = info;

        auto adapter = create.adapter;
        auto window = create.window;
        auto frameCount = create.frames;

        auto [width, height] = window->getClientSize();
        auto hwnd = window->getHandle();
        aspect = window->getClientAspectRatio();

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
                .NumDescriptors = Slots::Total,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            };

            check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&cbvSrvHeap)), "failed to create CBV descriptor heap");

            cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            cbvSrvHeap.rename(L"d3d12-cbv-srv-heap");
        }

        frames = new Frame[frameCount];
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

            for (UINT i = 0; i < frameCount; i++) {
                auto &frame = frames[i];
                
                check(swapchain->GetBuffer(i, IID_PPV_ARGS(&frame.target)), "failed to get swapchain buffer");
                device->CreateRenderTargetView(frame.target.get(), nullptr, handle);
                
                for (auto alloc = 0; alloc < Allocator::Total; alloc++) {
                    auto &allocator = frame.allocators[alloc];
                    check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "failed to create command allocator");
                    allocator.rename(std::format(L"frame-allocator-{}-{}", i, alloc).c_str());
                }

                handle.Offset(1, rtvDescriptorSize);
                frame.fenceValue = 0;

                frame.target.rename(std::format(L"frame-target-{}", i));
            }
        }
    }

    void Context::destroyDevice() {
        for (UINT i = 0; i < create.frames; i++) {
            auto frame = frames[i];
            frame.target.release();
            for (auto &alloc : frame.allocators) {
                alloc.tryDrop("allocator");
            }
        }
        delete[] frames;

        cbvSrvHeap.tryDrop("cbv-srv-heap");
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
        queue.tryDrop("queue");
        device.tryDrop("device");
    }

    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    constexpr D3D12_STATIC_SAMPLER_DESC samplers[] = { 
        {
            .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
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
        }
    };

    void Context::createAssets() {
        auto version = rootVersion();
        {
            CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
            CD3DX12_ROOT_PARAMETER1 parameters[1];

            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
            ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

            parameters[0].InitAsDescriptorTable(std::size(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
            desc.Init_1_1(std::size(parameters), parameters, std::size(samplers), samplers, rootSignatureFlags);
        
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
            auto vertexShader = compileShader(L"resources\\shader.hlsl", "VSMain", "vs_5_0");
            auto pixelShader = compileShader(L"resources\\shader.hlsl", "PSMain", "ps_5_0");
        
            D3D12_INPUT_ELEMENT_DESC inputs[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
                .pRootSignature = rootSignature.get(),
                .VS = CD3DX12_SHADER_BYTECODE(vertexShader.get()),
                .PS = CD3DX12_SHADER_BYTECODE(pixelShader.get()),
                .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
                .SampleMask = UINT_MAX,
                .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
                .InputLayout = { inputs, std::size(inputs) },
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
                .DSVFormat = DXGI_FORMAT_D32_FLOAT,
                .SampleDesc = { 1 }
            };

            check(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState)), "failed to create graphics pipeline state");
        
            pipelineState.rename(L"d3d12-pipeline-state");
        }

        auto alloc = getAllocator(Allocator::Context).get();

        check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc, pipelineState.get(), IID_PPV_ARGS(&commandList)), "failed to create command list");
        
        commandList.rename(L"d3d12-command-list");

        const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        model = loader::obj("resources\\utah-teapot.obj");

        {
            const UINT vertexBufferSize = model.vertices.size() * sizeof(loader::Vertex);
            auto verts = model.vertices.data();

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
                .StrideInBytes = sizeof(loader::Vertex)
            };

            vertexBuffer.rename(L"d3d12-vertex-buffer");
        }

        {
            const UINT indexBufferSize = model.indices.size() * sizeof(DWORD);
            auto indicies = model.indices.data();

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

        {
            UINT constBufferSize = sizeof(constBufferData);
            const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(constBufferSize);

            /// create resource to hold constant buffer data
            HRESULT hr = device->CreateCommittedResource(
                &upload, D3D12_HEAP_FLAG_NONE,
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&constBuffer)
            );
            check(hr, "failed to create constant buffer");
            
            /// tell the device we want to make this resource a constant buffer
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
                .BufferLocation = constBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = constBufferSize
            };
            device->CreateConstantBufferView(&desc, getCbvSrvCpuHandle(Slots::Buffer));

            /// map the constant buffer into visible memory
            CD3DX12_RANGE readRange(0, 0);
            check(constBuffer->Map(0, &readRange, &constBufferPtr), "failed to map constant buffer");
        }

        Com<ID3D12Resource> uploadResource;

        {
            const auto width = 256;
            const auto height = 256;

            const auto def = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            const auto up = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto data = generateTexture(width, height);
            const auto format = DXGI_FORMAT_R8G8B8A8_UNORM;

            D3D12_RESOURCE_DESC desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .Width = width,
                .Height = height,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = format,
                .SampleDesc = { 1, 0 },
                .Flags = D3D12_RESOURCE_FLAG_NONE
            };

            HRESULT hrDefault = device->CreateCommittedResource(
                &def, D3D12_HEAP_FLAG_NONE,
                &desc, D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr, IID_PPV_ARGS(&texture)
            );
            check(hrDefault, "failed to create texture");

            const UINT64 textureSize = GetRequiredIntermediateSize(texture.get(), 0, 1);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(textureSize);

            HRESULT hrUpload = device->CreateCommittedResource(
                &up, D3D12_HEAP_FLAG_NONE,
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&uploadResource)
            );
            check(hrUpload, "failed to create upload buffer");

            D3D12_SUBRESOURCE_DATA texData = {
                .pData = data.data(),
                .RowPitch = width * 4,
                .SlicePitch = width * height * 4
            };
            UpdateSubresources(commandList.get(), texture.get(), uploadResource.get(), 0, 0, 1, &texData);
            const auto toResource = CD3DX12_RESOURCE_BARRIER::Transition(texture.get(), 
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
            );
            commandList->ResourceBarrier(1, &toResource);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
                .Format = format,
                .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Texture2D = { .MipLevels = 1 }
            };
            device->CreateShaderResourceView(texture.get(), &srvDesc, getCbvSrvCpuHandle(Slots::Texture));
        
            texture.rename(L"d3d12-texture");
            uploadResource.rename(L"d3d12-upload-resource");
        }

        check(commandList->Close(), "failed to close command list");

        ImGui_ImplWin32_Init(create.window->getHandle());
        ImGui_ImplDX12_Init(device.get(), create.frames,
            DXGI_FORMAT_R8G8B8A8_UNORM, cbvSrvHeap.get(),
            getCbvSrvCpuHandle(Slots::ImGui),
            getCbvSrvGpuHandle(Slots::ImGui)
        );

        flushQueue();

        check(device->CreateFence(frames[frameIndex].fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "failed to create fence");
        frames[frameIndex].fenceValue = 1;
        if ((fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr) {
            throw win32::Error("failed to create fence event");
        }

        waitForGPU();

        uploadResource.tryDrop("upload-resource");
    }

    void Context::destroyAssets() {
        waitForGPU();

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();

        texture.tryDrop("texture");
        constBuffer.tryDrop("const-buffer");
        indexBuffer.tryDrop("index-buffer");
        vertexBuffer.tryDrop("vertex-buffer");
        rootSignature.tryDrop("root-signature");
        pipelineState.tryDrop("pipeline-state");
        commandList.tryDrop("command-list");

        CloseHandle(fenceEvent);
        fence.tryDrop("fence");
    }

    void Context::tick(const input::Camera& camera) {
        auto model = XMMatrixScaling(1.f, 1.f, 1.f);
        XMStoreFloat4x4(&constBufferData.model, model);

        camera.store(&constBufferData.view, &constBufferData.projection, aspect);
    }

    void Context::populate() {
        /// imgui execution
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        log::render->tick();

        ImGui::Render();

        /// common resources
        ID3D12DescriptorHeap *heaps[] = { cbvSrvHeap.get() };
        
        auto *contextList = commandList.get();

        auto draw = ImGui::GetDrawData();
        auto target = getTarget().get();
        const auto toTarget = CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        const auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

        /// main command list execution
        auto alloc = getAllocator(Allocator::Context);
        check(alloc->Reset(), "failed to reset command allocator");
        check(contextList->Reset(alloc.get(), pipelineState.get()), "failed to reset command list");

        contextList->SetGraphicsRootSignature(rootSignature.get());
        contextList->SetDescriptorHeaps(std::size(heaps), heaps);
        contextList->SetGraphicsRootDescriptorTable(0, getCbvSrvGpuHandle(0));
        
        contextList->RSSetViewports(1, &scene.viewport);
        contextList->RSSetScissorRects(1, &scene.scissor);

        contextList->ResourceBarrier(1, &toTarget);

        contextList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        const float clear[] = { 0.f, 0.2f, 0.4f, 1.f };
        contextList->ClearRenderTargetView(rtvHandle, clear, 0, nullptr);
        contextList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        contextList->IASetVertexBuffers(0, 1, &vertexBufferView);
        contextList->IASetIndexBuffer(&indexBufferView);
        contextList->DrawIndexedInstanced(model.indices.size(), 1, 0, 0, 0);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), contextList);

        contextList->ResourceBarrier(1, &toPresent);

        check(contextList->Close(), "failed to close command list");
    }

    void Context::present() {
        memcpy(constBufferPtr, &constBufferData, sizeof(constBufferData));
        populate();

        flushQueue();

        check(swapchain->Present(0, factory.presentFlags()), "failed to present swapchain");

        nextFrame();
    }

    void Context::flushQueue() {
        ID3D12CommandList* lists[] = { commandList.get() };
        queue->ExecuteCommandLists(std::size(lists), lists);
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
