#include "render.h"

#include "assets/loader.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include <array>

namespace engine::render {
    struct ScreenVertex {
        XMFLOAT4 position;
        XMFLOAT2 texcoord;
    };

    constexpr auto screenQuad = std::to_array<ScreenVertex>({
        { { -1.f, -1.f, 0.f, 1.f }, { 0.f, 1.f } },
        { { -1.f,  1.f, 0.f, 1.f }, { 0.f, 0.f } },
        { {  1.f, -1.f, 0.f, 1.f }, { 1.f, 1.f } },
        { {  1.f,  1.f, 0.f, 1.f }, { 1.f, 0.f } }
    });

    constexpr auto swapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr auto depthFormat = DXGI_FORMAT_D32_FLOAT;
    constexpr auto swapSampleCount = 1;
    constexpr auto swapSampleQuality = 0;
    constexpr float clearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    const auto clearValue = CD3DX12_CLEAR_VALUE(swapFormat, clearColour);

    /// shader libraries

    constexpr auto sceneInput = std::to_array({
        shaderInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, offsetof(loader::Vertex, position)),
        shaderInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, offsetof(loader::Vertex, normal)),
        shaderInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, offsetof(loader::Vertex, texcoord))
    });

    constexpr ShaderLibrary::Create sceneCreate = {
        .path = L"resources\\scene-shader.hlsl",
        .vsMain = "vsMain",
        .psMain = "psMain",
        .layout = { sceneInput }
    };

    constexpr auto postInput = std::to_array({
        shaderInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, offsetof(ScreenVertex, position)),
        shaderInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, offsetof(ScreenVertex, texcoord))
    });

    constexpr ShaderLibrary::Create postCreate = {
        .path = L"resources\\post-shader.hlsl",
        .vsMain = "vsMain",
        .psMain = "psMain",
        .layout = { postInput }
    };

    /// buffer property constants

    constexpr auto uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    constexpr auto defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    constexpr auto sceneWidth = 1920 * 2;
    constexpr auto sceneHeight = 1080 * 2;

    void Context::updateViews() {
        float widthRatio = float(internalWidth) / float(displayWidth);
        float heightRatio = float(internalHeight) / float(displayHeight);

        float x = 1.f;
        float y = 1.f;

        if (widthRatio < heightRatio) {
            x = widthRatio / heightRatio;
        } else {
            y = heightRatio / widthRatio;
        }

        /// correct
        /// lines up with fullscreen sample

        auto left   = displayWidth * (1.f - x) / 2.f;
        auto top    = displayHeight * (1.f - y) / 2.f;
        auto width  = displayWidth * x;
        auto height = displayHeight * y;

        /// update post viewport
        postView.viewport.TopLeftX  = FLOAT(left);
        postView.viewport.TopLeftY  = FLOAT(top);
        postView.viewport.Width     = FLOAT(width);
        postView.viewport.Height    = FLOAT(height);

        /// update post scissor
        postView.scissor.left       = LONG(left);
        postView.scissor.top        = LONG(top);
        postView.scissor.right      = LONG(left + width);
        postView.scissor.bottom     = LONG(top + height);

        /// update scene viewport
        sceneView.viewport.TopLeftX = 0;
        sceneView.viewport.TopLeftY = 0;
        sceneView.viewport.Width    = FLOAT(internalWidth);
        sceneView.viewport.Height   = FLOAT(internalHeight);

        /// update scene scissor
        sceneView.scissor.left      = 0;
        sceneView.scissor.top       = 0;
        sceneView.scissor.right     = LONG(internalWidth);
        sceneView.scissor.bottom    = LONG(internalHeight);

        log::render->info("resolution info");
        log::render->info("  display: {}x{}", displayWidth, displayHeight);
        log::render->info("  internal: {}x{}", internalWidth, internalHeight);
        log::render->info("  scene view:");
        log::render->info("    viewport: {}x{} {}x{}", 
            sceneView.viewport.TopLeftX, sceneView.viewport.TopLeftY, 
            sceneView.viewport.Width, sceneView.viewport.Height
        );
        log::render->info("    scissor: {}x{} {}x{}", 
            sceneView.scissor.left, sceneView.scissor.top, 
            sceneView.scissor.right, sceneView.scissor.bottom
        );
        log::render->info("  post view:");
        log::render->info("    viewport: {}x{} {}x{}", 
            postView.viewport.TopLeftX, postView.viewport.TopLeftY, 
            postView.viewport.Width, postView.viewport.Height
        );
        log::render->info("    scissor: {}x{} {}x{}",
            postView.scissor.left, postView.scissor.top,
            postView.scissor.right, postView.scissor.bottom
        );
    }

    void Context::createDevice(Context::Create& info) {
        create = info;
        scene = loader::objScene("resources\\utah-teapot.obj");

        auto adapter = create.adapter;
        auto window = create.window;
        auto frameCount = create.frames;

        auto hwnd = window->getHandle();

        std::tie(displayWidth, displayHeight) = window->getClientSize();
        displayAspect = window->getClientAspectRatio();

        internalWidth = sceneWidth;
        internalHeight = sceneHeight;
        internalAspect = float(internalWidth) / float(internalHeight);

        device = adapter.newDevice<ID3D12Device4>(D3D_FEATURE_LEVEL_11_0, L"render-device");

        attachInfoQueue();
        updateViews();

        directQueue = device.newCommandQueue(L"direct-queue", { D3D12_COMMAND_LIST_TYPE_DIRECT });
        copyQueue = device.newCommandQueue(L"copy-queue", { D3D12_COMMAND_LIST_TYPE_COPY });

        /// correct
        /// lines up with fullscreen sample
        {
            /// create swap chain
            DXGI_SWAP_CHAIN_DESC1 desc = {
                .Width = UINT(displayWidth),
                .Height = UINT(displayHeight),
                .Format = swapFormat,
                .SampleDesc = { .Count = swapSampleCount, .Quality = swapSampleQuality },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = frameCount,
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
                .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
            };

            auto swapchain1 = factory.createSwapChain(directQueue, hwnd, desc);

            if (!(swapchain = swapchain1.as<IDXGISwapChain3>()).valid()) {
                throw engine::Error("failed to create swapchain");
            }

            swapchain.release();

            frameIndex = swapchain->GetCurrentBackBufferIndex();
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                .NumDescriptors = 1
            };

            check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvHeap)), "failed to create DSV descriptor heap");
        
            dsvHeap.rename(L"d3d12-dsv-heap");
        }

        rtvHeap = device.newHeap(L"d3d12-rtv-heap", {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = requiredRtvHeapEntries()
        });

        cbvSrvHeap = device.newHeap(L"d3d12-cbv-srv-heap", {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = requiredCbvSrvHeapEntries(),
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        });

        /// create the intermediate render target
        {
            /// correct
            /// lines up with fullscreen sample
            const auto targetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                swapFormat, internalWidth, internalHeight,
                1u, 1u,
                swapSampleCount, swapSampleQuality,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
            );

            HRESULT hr = device->CreateCommittedResource(
                &defaultProps, D3D12_HEAP_FLAG_NONE,
                &targetDesc, D3D12_RESOURCE_STATE_RENDER_TARGET,
                &clearValue, IID_PPV_ARGS(&sceneTarget)
            );
            check(hr, "failed to create intermediate target");

            device->CreateRenderTargetView(sceneTarget.get(), nullptr, getIntermediateHandle());
            device->CreateShaderResourceView(sceneTarget.get(), nullptr, cbvSrvHeap.cpuHandle(Resource::Intermediate));
            sceneTarget.rename(L"intermediate-target");
        }

        frames = new Frame[frameCount];
        {
            for (UINT i = 0; i < frameCount; i++) {
                auto &frame = frames[i];
                
                check(swapchain->GetBuffer(i, IID_PPV_ARGS(&frame.target)), "failed to get swapchain buffer");
                device->CreateRenderTargetView(frame.target.get(), nullptr, getTargetHandle(i));
                
                for (auto alloc = 0; alloc < Allocator::Total; alloc++) {
                    auto &allocator = frame.allocators[alloc];
                    auto as = Allocator::Index(alloc);

                    const auto type = Allocator::getType(as);
                    const auto name = Allocator::getName(as);

                    check(device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)), "failed to create command allocator");
                    allocator.rename(std::format(L"frame-allocator-{}-{}", i, name).c_str());
                }

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

        sceneTarget.tryDrop("scene-target");
        dsvHeap.tryDrop("dsv-heap");
        cbvSrvHeap.tryDrop("cbv-srv-heap");
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
        directQueue.tryDrop("direct-queue");
        copyQueue.tryDrop("copy-queue");
        device.tryDrop("device");
    }

    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    void Context::createAssets() {
        sceneShaders = ShaderLibrary(sceneCreate);
        postShaders = ShaderLibrary(postCreate);

        auto version = rootVersion();

        // create our scene root signature
        // consists of a constant buffer with camera data 
        // an srv for the model texture
        // and a sampler
        {
            D3D12_STATIC_SAMPLER_DESC textureSamplers[] = {{
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
            CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
            CD3DX12_ROOT_PARAMETER1 parameters[2];

            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // constant buffer data
            ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // texture data

            parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX); // constant buffer
            parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL); // texture

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
            desc.Init_1_1(UINT(std::size(parameters)), parameters, UINT(std::size(textureSamplers)), textureSamplers, rootSignatureFlags);
        
            Com<ID3DBlob> signature;
            Com<ID3DBlob> error;

            HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error);
            if (FAILED(hr)) {
                throw render::Error(hr, (char*)error->GetBufferPointer());
            }

            sceneRootSignature = device.newRootSignature(L"scene-root-signature", signature);
        }

        // create our post root signature
        {
            D3D12_STATIC_SAMPLER_DESC postSamplers[] = {{
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
            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            CD3DX12_ROOT_PARAMETER1 parameters[1];

            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // texture data
            parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL); // texture

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
            desc.Init_1_1(UINT(std::size(parameters)), parameters, UINT(std::size(postSamplers)), postSamplers, rootSignatureFlags);

            Com<ID3DBlob> signature;
            Com<ID3DBlob> error;

            HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error);
            if (FAILED(hr)) {
                throw render::Error(hr, (char*)error->GetBufferPointer());
            }

            postRootSignature = device.newRootSignature(L"post-root-signature", signature);
        }

        scenePipelineState = device.newPipelineState(L"scene-pipeline-state", sceneShaders, sceneRootSignature.get());
        postPipelineState = device.newPipelineState(L"post-pipeline-state", postShaders, postRootSignature.get());

        /// create all the needed command lists

        auto copyAlloc = getAllocator(Allocator::Copy).get();
        auto sceneAlloc = getAllocator(Allocator::Scene).get();
        auto postAlloc = getAllocator(Allocator::Post).get();

        copyCommandList = device.newCommandList(L"copy-command-list", D3D12_COMMAND_LIST_TYPE_COPY, copyAlloc);
        sceneCommandList = device.newCommandList(L"scene-command-list", D3D12_COMMAND_LIST_TYPE_DIRECT, sceneAlloc, scenePipelineState.get());
        postCommandList = device.newCommandList(L"post-command-list", D3D12_COMMAND_LIST_TYPE_DIRECT, postAlloc);

        check(sceneCommandList->Close(), "failed to close scene command list");
        check(postCommandList->Close(), "failed to close post command list");

        /// upload all our needed geometry

        std::tie(vertexBuffer, vertexBufferView) = uploadVertexBuffer(std::span(scene.vertices), L"model-vertex-data");
        std::tie(indexBuffer, indexBufferView) = uploadIndexBuffer(std::span(scene.indices), DXGI_FORMAT_R32_UINT, L"model-index-data");

        // upload our screen quad
        std::tie(screenBuffer, screenBufferView) = uploadVertexBuffer(std::span(screenQuad.data(), screenQuad.size()), L"screen-quad");

        {
            D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
                .Format = depthFormat,
                .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D
            };

            D3D12_CLEAR_VALUE clear = {
                .Format = depthFormat,
                .DepthStencil = { 1.0f, 0 }
            };

            const auto tex2d = CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, internalWidth, internalHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            HRESULT hr = device->CreateCommittedResource(
                &defaultProps, D3D12_HEAP_FLAG_NONE,
                &tex2d, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clear, IID_PPV_ARGS(&depthStencil)
            );
            check(hr, "failed to create depth stencil");
            depthStencil.rename(L"d3d12-depth-stencil");

            device->CreateDepthStencilView(depthStencil.get(), &desc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
        }

        {
            UINT constBufferSize = sizeof(constBufferData);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(constBufferSize);

            /// create resource to hold constant buffer data
            HRESULT hr = device->CreateCommittedResource(
                &uploadProps, D3D12_HEAP_FLAG_NONE,
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&constBuffer)
            );
            check(hr, "failed to create constant buffer");
            
            /// tell the device we want to make this resource a constant buffer
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
                .BufferLocation = constBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = constBufferSize
            };
            device->CreateConstantBufferView(&desc, cbvSrvHeap.cpuHandle(Resource::Camera));

            /// map the constant buffer into visible memory
            CD3DX12_RANGE readRange(0, 0);
            check(constBuffer->Map(0, &readRange, &constBufferPtr), "failed to map constant buffer");
        }

        /// upload all our textures to the gpu
        textures.resize(scene.textures.size());
        for (size_t i = 0; i < scene.textures.size(); i++) {
            const auto& tex = scene.textures[i];
            textures[i] = uploadTexture(tex, getTextureSlotCpuHandle(i), std::format(L"texture-{}", i));
        }

        ImGui_ImplWin32_Init(create.window->getHandle());
        ImGui_ImplDX12_Init(device.get(), create.frames,
            DXGI_FORMAT_R8G8B8A8_UNORM, cbvSrvHeap.get(),
            cbvSrvHeap.cpuHandle(Resource::ImGui),
            cbvSrvHeap.gpuHandle(Resource::ImGui)
        );

        check(device->CreateFence(frames[frameIndex].fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "failed to create fence");
        frames[frameIndex].fenceValue = 1;
        if ((fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr) {
            throw win32::Error("failed to create fence event");
        }

        flushCopyQueue();
    }

    void Context::destroyAssets() {
        waitForGPU(directQueue);

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();

        for (auto& texture : textures) {
            texture.tryDrop("texture");
        }

        screenBuffer.tryDrop("screen-buffer");

        depthStencil.tryDrop("depth-stencil");
        constBuffer.tryDrop("const-buffer");
        indexBuffer.tryDrop("index-buffer");
        vertexBuffer.tryDrop("vertex-buffer");

        sceneRootSignature.tryDrop("scene-root-signature");
        scenePipelineState.tryDrop("scene-pipeline-state");

        postRootSignature.tryDrop("post-root-signature");
        postPipelineState.tryDrop("post-pipeline-state");

        sceneCommandList.tryDrop("scene-command-list");
        postCommandList.tryDrop("post-command-list");
        copyCommandList.tryDrop("copy-command-list");

        CloseHandle(fenceEvent);
        fence.tryDrop("fence");
    }

    void Context::tick(const input::Camera& camera) {
        XMStoreFloat4x4(&constBufferData.model, XMMatrixScaling(0.1f, 0.1f, 0.1f));
        camera.store(&constBufferData.view, &constBufferData.projection, internalAspect);
    }

    void Context::populate() {
        /// imgui execution
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        log::render->tick();

        ImGui::Render();

        /// common scene resources
        auto draw = ImGui::GetDrawData();
        auto target = getTarget().get();
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());

        /// scene resources
        ID3D12DescriptorHeap *heaps[] = { cbvSrvHeap.get() };
        auto sceneAlloc = getAllocator(Allocator::Scene);
        auto *sceneList = sceneCommandList.get();

        /// post resources
        auto postAlloc = getAllocator(Allocator::Post);
        auto *postList = postCommandList.get();

        /// common setup

        check(sceneAlloc->Reset(), "failed to reset scene command allocator");
        check(postAlloc->Reset(), "failed to reset post command allocator");

        check(sceneList->Reset(sceneAlloc.get(), scenePipelineState.get()), "failed to reset scene command list");
        check(postList->Reset(postAlloc.get(), postPipelineState.get()), "failed to reset post command list");

        /// scene command list
        {
            sceneList->SetGraphicsRootSignature(sceneRootSignature.get());
            sceneList->SetDescriptorHeaps(UINT(std::size(heaps)), heaps);
            sceneList->SetGraphicsRootDescriptorTable(0, cbvSrvHeap.gpuHandle(Resource::Camera));

            sceneList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            sceneList->RSSetViewports(1, &sceneView.viewport);
            sceneList->RSSetScissorRects(1, &sceneView.scissor);

            /// render to our intermediate target
            auto rtvHandle = getIntermediateHandle();
            sceneList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

            sceneList->ClearRenderTargetView(rtvHandle, clearColour, 0, nullptr);
            sceneList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
            
            sceneList->IASetVertexBuffers(0, 1, &vertexBufferView);
            sceneList->IASetIndexBuffer(&indexBufferView);

            for (size_t i = 0; i < scene.objects.size(); i++) {
                const auto& obj = scene.objects[i];
                const auto start = obj.offset;
                const auto count = obj.length;

                sceneList->SetGraphicsRootDescriptorTable(1, getTextureSlotGpuHandle(obj.texture));
                sceneList->DrawIndexedInstanced(UINT(count), 1, UINT(start), 0, 0);
            }
        }

        /// post command list

        {
            postList->SetGraphicsRootSignature(postRootSignature.get());

            postList->SetDescriptorHeaps(UINT(std::size(heaps)), heaps);

            D3D12_RESOURCE_BARRIER inTransitions[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
                CD3DX12_RESOURCE_BARRIER::Transition(sceneTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            };

            postList->ResourceBarrier(UINT(std::size(inTransitions)), inTransitions);

            postList->SetGraphicsRootDescriptorTable(0, cbvSrvHeap.gpuHandle(Resource::Intermediate));
            postList->RSSetScissorRects(1, &postView.scissor);
            postList->RSSetViewports(1, &postView.viewport);

            /// render to the back buffer
            auto rtvHandle = getTargetHandle(frameIndex);
            postList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

            float borderColour[] = { 0.f, 0.f, 0.f, 1.f };

            /// draw fullscreen quad with texture on it
            postList->ClearRenderTargetView(rtvHandle, borderColour, 0, nullptr);
            postList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            postList->IASetVertexBuffers(0, 1, &screenBufferView);
            postList->DrawInstanced(4, 1, 0, 0);

            ImGui_ImplDX12_RenderDrawData(draw, postList);

            D3D12_RESOURCE_BARRIER outTransitions[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
                CD3DX12_RESOURCE_BARRIER::Transition(sceneTarget.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
            };

            postList->ResourceBarrier(UINT(std::size(outTransitions)), outTransitions);
        }

        /// common cleanup
        check(sceneList->Close(), "failed to close scene command list");
        check(postList->Close(), "failed to close post command list");
    }

    void Context::present() {
        memcpy(constBufferPtr, &constBufferData, sizeof(constBufferData));
        populate();

        flushQueue();

        check(swapchain->Present(0, factory.presentFlags()), "failed to present swapchain");

        nextFrame(directQueue);
    }

    void Context::flushQueue() {
        ID3D12CommandList* lists[] = { sceneCommandList.get(), postCommandList.get() };
        directQueue->ExecuteCommandLists(UINT(std::size(lists)), lists);
    }

    void Context::flushCopyQueue() {
        check(copyCommandList->Close(), "failed to close copy command list");

        ID3D12CommandList* lists[] = { copyCommandList.get() };
        copyQueue->ExecuteCommandLists(UINT(std::size(lists)), lists);

        waitForGPU(copyQueue);

        for (auto &resource : copyResources) {
            resource.tryDrop("copy-resource");
        }
    }

    void Context::waitForGPU(Com<ID3D12CommandQueue> queue) {
        UINT64 &value = frames[frameIndex].fenceValue;
        check(queue->Signal(fence.get(), value), "failed to signal fence");

        check(fence->SetEventOnCompletion(value, fenceEvent), "failed to set fence event");
        WaitForSingleObject(fenceEvent, INFINITE);

        value += 1;
    }

    void Context::nextFrame(Com<ID3D12CommandQueue> queue) {
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

    auto callback = [](UNUSED auto category, auto severity, auto id, auto msg, auto ctx) {
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
