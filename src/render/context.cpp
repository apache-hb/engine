#include "context.h"

#include "d3dx12.h"

#include <d3dcompiler.h>
#include "vertex.h"

extern "C" { 
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
    __declspec(dllexport) extern const auto* D3D12SDKPath = u8".\\D3D12\\"; 
}

namespace render {
    namespace {
        dxgi::Adapter1 getAdapter(dxgi::Factory4 factory, UINT idx) {
            dxgi::Adapter1 result;
            if (factory->EnumAdapters1(idx, &result) == DXGI_ERROR_NOT_FOUND) {
                return nullptr;
            }
            return result;
        }

        void infoQueueCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR msg, void *ctx) {
            fprintf(stderr, "d3d12: %s\n", msg);
        }

        HRESULT compileFromFile(LPCWSTR path, const char *entry, const char *target, ID3DBlob **blob) {
            return D3DCompileFromFile(path, nullptr, nullptr, entry, target, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, blob, nullptr);
        }

        HRESULT createUploadBuffer(d3d12::Device1 device, UINT size, ID3D12Resource **resource) {
            const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(size);

            return device->CreateCommittedResource(
                &upload, D3D12_HEAP_FLAG_NONE, 
                &buffer, D3D12_RESOURCE_STATE_GENERIC_READ, 
                nullptr, 
                IID_PPV_ARGS(resource)
            );
        }
    }

    ///
    /// create instance data
    ///

    HRESULT Context::create(const Instance &instance, size_t index) { 
        factory = instance.factory;
        adapter = instance.adapters[index];

        return createDevice();
    }

    HRESULT Context::createDevice() {
        /// create the logical device
        if (HRESULT hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3D12CreateDevice = 0x%x\n", hr);
            return hr;
        }

        /// if we are on a recent enough version of windows
        /// enable the info queue for better debug reporting
        if ((infoQueue = device.as<d3d12::InfoQueue1>(L"d3d12-info-queue")).valid()) {
            DWORD cookie = 0;
            if (HRESULT hr = infoQueue->RegisterMessageCallback(infoQueueCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie); FAILED(hr)) {
                fprintf(stderr, "d3d12: RegisterMessageCallback = 0x%x\n", hr);
            }
        }

        /// create our root command queue
        D3D12_COMMAND_QUEUE_DESC desc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT };
        if (HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateCommandQueue = 0x%x\n", hr);
            return hr;
        }

        return S_OK;
    }

    void Context::destroy() {
        commandQueue.tryDrop("command-queue");
        infoQueue.tryRelease();
        device.tryDrop("device");
    }

    ///
    /// attach to a window
    ///

    HRESULT Context::attach(WindowHandle *handle, UINT buffers) {
        window = handle;
        frames = buffers;

        if (HRESULT hr = createSwapChain(); FAILED(hr)) {
            return hr;
        }
        
        if (HRESULT hr = createAssets(); FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT Context::createSwapChain() {
        /// create and attach a swapchain to our window
        auto [width, height] = window->getClientSize();

        viewport = d3d12::Viewport(FLOAT(width), FLOAT(height));
        scissor = d3d12::Scissor(width, height);
        DXGI_SWAP_CHAIN_DESC1 desc = {
            .Width = UINT(width),
            .Height = UINT(height),
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { .Count = 1 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = frames,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
        };

        dxgi::SwapChain1 swap;
        HRESULT hr = factory->CreateSwapChainForHwnd(
            /* pDevice = */ commandQueue.get(),
            /* hWnd = */ window->getHandle(),
            /* pDesc = */ &desc,
            /* pFullscreenDesc = */ nullptr,
            /* pRestrictToOutput = */ nullptr,
            /* ppSwapChain = */ swap.addr()
        );

        if (FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateSwapChainForHwnd = 0x%x\n", hr);
            return hr;
        }

        /// prevent ALT+ENTER from changing the fullscreen state
        /// we dont currently support this behaviour
        if (HRESULT hr = factory->MakeWindowAssociation(window->getHandle(), DXGI_MWA_NO_ALT_ENTER); FAILED(hr)) {
            fprintf(stderr, "d3d12: MakeWindowAssociation = 0x%x\n", hr);
            return hr;
        }

        /// we need a swapchain3 but currently have a swapchain1
        /// attempt to upcast to a swapchain3
        if ((swapChain = swap.as<dxgi::SwapChain3>(L"d3d12-swapchain")).valid()) {
            swap.release();
        } else {
            return E_NOINTERFACE;
        }

        /// get our current render target index
        frameIndex = swapChain->GetCurrentBackBufferIndex();
        
        /// create our descriptor heap that contains our frame render targets
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = frames,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        };
        
        if (HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.addr())); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateDescriptorHeap = 0x%x\n", hr);
            return hr;
        }

        /// allocate data for our frame render targets
        renderTargets = new d3d12::Resource[frames];
        commandAllocators = new d3d12::CommandAllocator[frames];

        rtvHeap.descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        /// iterate over our frame render targets
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < frames; i++) {
            /// get our current back buffer
            d3d12::Resource *target = renderTargets + i;
            
            /// create a render target for this backbuffer
            if (HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(target->addr())); FAILED(hr)) {
                fprintf(stderr, "d3d12: GetBuffer(%u) = 0x%x\n", i, hr);
                return hr;
            }

            device->CreateRenderTargetView(target->get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvHeap.descriptorSize);

            /// then create a command allocator for this frame
            if (HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocators->addr())); FAILED(hr)) {
                fprintf(stderr, "d3d12: CreateCommandAllocator(%u) = 0x%x\n", i, hr);
                return hr;
            }

            target->release();
        }

        if (HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&bundleAllocator)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateCommandAllocator = 0x%x\n", hr);
            return hr;
        }

        return S_OK;
    }

    void Context::detach() {
        bundleAllocator.tryDrop("bundle-allocator");

        for (UINT i = 0; i < frames; i++) {
            commandAllocators[i].drop("command-allocator");
            renderTargets[i].drop("render-target");
        }

        rtvHeap.tryDrop("rtv-heap");
        swapChain.tryDrop("swapchain");

        delete[] commandAllocators;
        delete[] renderTargets;
    }

    ///
    /// create assets
    ///

    HRESULT Context::createAssets() {
        /// create our shader pipeline
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        d3d::Blob signature;
        d3d::Blob error;

        /// first create a root signature for our pipeline
        /// TODO: what does this do exactly? check d3d12 docs
        if (HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3D12SerializeRootSignature 0x%x %s\n", hr, (char*)error->GetBufferPointer());
            return hr;
        }

        if (HRESULT hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateRootSignature 0x%x\n", hr);
            return hr;
        }

        d3d::Blob vertexShader;
        d3d::Blob pixelShader;

        /// compile our pixel and vertex shader
        if (HRESULT hr = compileFromFile(L"shaders/shader.hlsl", "VSMain", "vs_5_0", &vertexShader); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3DCompileFromFile 0x%x\n", hr);
            return hr;
        }

        if (HRESULT hr = compileFromFile(L"shaders/shader.hlsl", "PSMain", "ps_5_0", &pixelShader); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3DCompileFromFile 0x%x\n", hr);
            return hr;
        }

        /// then bind our shader inputs
        D3D12_INPUT_ELEMENT_DESC inputs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
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

        /// create our graphics pipeline encapsulating our shaders,
        /// input layout, and formats
        if (HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateGraphicsPipelineState 0x%x\n", hr);
            return hr;
        }

        /// create a command list to record commands into
        if (HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[frameIndex].get(), pipelineState.get(), IID_PPV_ARGS(&commandList)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT) 0x%x\n", hr);
            return hr;
        }

        /// make sure its closed
        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            fprintf(stderr, "d3d12: Close 0x%x\n", hr);
            return hr;
        }

        /// create our triangle
        auto [width, height] = window->getClientSize();
        auto aspect = float(width) / float(height);

        /// scaled properly to the window aspect ratio
        Vertex triangle[] = {
            { { 0.f, 0.25f * aspect, 0.f }, { 1.f, 0.f, 0.f, 1.f } },
            { { 0.25f, -0.25f * aspect, 0.f }, { 0.f, 1.f, 0.f, 1.f } },
            { { -0.25f, -0.25f * aspect, 0.f }, { 0.f, 0.f, 1.f, 1.f } }
        };
        UINT vertexBufferSize = sizeof(triangle);

        /// create our upload buffer
        if (HRESULT hr = createUploadBuffer(device, vertexBufferSize, &vertexBuffer); FAILED(hr)) {
            fprintf(stderr, "d3d12: createUploadBuffer 0x%x\n", hr);
            return hr;
        }

        /// map it into our visible memory
        void *vertexDataPtr = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        if (HRESULT hr = vertexBuffer->Map(0, &readRange, &vertexDataPtr); FAILED(hr)) {
            fprintf(stderr, "d3d12: Map 0x%x\n", hr);
            return hr;
        }
        /// upload the data and unmap it
        memcpy(vertexDataPtr, triangle, vertexBufferSize);
        vertexBuffer->Unmap(0, nullptr);

        vertexBufferView = {
            .BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = vertexBufferSize,
            .StrideInBytes = sizeof(Vertex)
        };

        /// create our bundle
        if (HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, bundleAllocator.get(), pipelineState.get(), IID_PPV_ARGS(&bundle)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateCommandList(D3D12_COMMAND_LIST_TYPE_BUNDLE) 0x%x\n", hr);
            return hr;
        }

        /// record the draw commands
        bundle.record([this](auto self) {
            self->SetGraphicsRootSignature(rootSignature.get());
            self->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            self->IASetVertexBuffers(0, 1, &vertexBufferView);
            self->DrawInstanced(3, 1, 0, 0);
            return S_OK;
        });

        fenceValues = new UINT64[frames];

        if (HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateFence 0x%x\n", hr);
            return hr;
        }

        fenceValues[frameIndex] = 1;

        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            fprintf(stderr, "d3d12: CreateEvent 0x%x\n", GetLastError());
            return E_FAIL;
        }

        if (HRESULT hr = waitForGpu(); FAILED(hr)) {
            fprintf(stderr, "d3d12: waitForGpu 0x%x\n", hr);
            return hr;
        }

        return S_OK;
    }

    void Context::unload() {
        if (HRESULT hr = waitForGpu(); FAILED(hr)) {
            fprintf(stderr, "d3d12: waitForGpu 0x%x\n", hr);
        }

        delete[] fenceValues;

        CloseHandle(fenceEvent);
        fence.tryDrop("fence");
        bundle.tryDrop("bundle-list");
        vertexBuffer.tryDrop("vertex-buffer");
        commandList.tryDrop("command-list");
        pipelineState.tryDrop("pipeline-state");
        rootSignature.tryDrop("root-signature");
    }

    void Context::present() {
        if (HRESULT hr = commandAllocators[frameIndex]->Reset(); FAILED(hr)) {
            fprintf(stderr, "d3d12: commandAllocators[%u]->Reset 0x%x\n", frameIndex, hr);
            return recover(hr);
        }

        if (HRESULT hr = commandList->Reset(commandAllocators[frameIndex].get(), pipelineState.get()); FAILED(hr)) {
            fprintf(stderr, "d3d12: commandList->Reset 0x%x\n", hr);
            return recover(hr);
        }

        commandList->SetGraphicsRootSignature(rootSignature.get());
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);

        commandList.transition(
            renderTargets[frameIndex],
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvHeap.descriptorSize);
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    
        const float clear[] = { 0.f, 0.2f, 0.4f, 1.f };
        commandList->ClearRenderTargetView(rtvHandle, clear, 0, nullptr);

        commandList->ExecuteBundle(bundle.get());

        commandList.transition(
            renderTargets[frameIndex],
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            fprintf(stderr, "d3d12: commandList->Close 0x%x\n", hr);
            return recover(hr);
        }

        ID3D12CommandList *lists[] = { commandList.get() };
        commandQueue->ExecuteCommandLists(1, lists);

        if (HRESULT hr = swapChain->Present(1, 0); FAILED(hr)) {
            fprintf(stderr, "d3d12: Present 0x%x\n", hr);
            return recover(hr);
        }

        if (HRESULT hr = nextFrame(); FAILED(hr)) {
            fprintf(stderr, "d3d12: nextFrame 0x%x\n", hr);
            return recover(hr);
        }
    }

    ///
    /// synchronising
    ///

    HRESULT Context::waitForGpu() {
        if (HRESULT hr = commandQueue->Signal(fence.get(), fenceValues[frameIndex]); FAILED(hr)) {
            fprintf(stderr, "d3d12: Signal 0x%x\n", hr);
            return hr;
        }

        if (HRESULT hr = fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent); FAILED(hr)) {
            fprintf(stderr, "d3d12: SetEventOnCompletion 0x%x\n", hr);
            return hr;
        }

        WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

        fenceValues[frameIndex]++;

        return S_OK;
    }

    HRESULT Context::nextFrame() {
        auto currentValue = fenceValues[frameIndex];
        if (HRESULT hr = commandQueue->Signal(fence.get(), currentValue); FAILED(hr)) {
            fprintf(stderr, "d3d12: Signal 0x%x\n", hr);
            return hr;
        }

        frameIndex = swapChain->GetCurrentBackBufferIndex();

        if (fence->GetCompletedValue() < fenceValues[frameIndex]) {
            if (HRESULT hr = fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent); FAILED(hr)) {
                fprintf(stderr, "d3d12: SetEventOnCompletion 0x%x\n", hr);
                return hr;
            }

            WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
        }

        fenceValues[frameIndex] = currentValue + 1;

        return S_OK;
    }

    ///
    /// error recovery
    ///

    void Context::recover(HRESULT hr) {
        fprintf(stderr, "d3d12: recovering 0x%x\n", hr);

        /// destroy all possible poisoned resources
        unload();
        detach();
        destroy();

        /// create fresh resources
        if (HRESULT hr = createDevice(); FAILED(hr)) {
            fprintf(stderr, "d3d12: recover device = 0x%x\n", hr);
            return;
        }

        if (HRESULT hr = createSwapChain(); FAILED(hr)) {
            fprintf(stderr, "d3d12: recover swapchain = 0x%x\n", hr);
            return;
        }

        if (HRESULT hr = createAssets(); FAILED(hr)) {
            fprintf(stderr, "d3d12: recover assets = 0x%x\n", hr);
            return;
        }
    }


    HRESULT Instance::create() {
        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug)); SUCCEEDED(hr)) {
            debug->EnableDebugLayer();
        } else {
            fprintf(stderr, "d3d12: D3D12GetDebugInstance = 0x%x\n", hr);
            return hr;
        }

        if (HRESULT hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)); SUCCEEDED(hr)) {
            dxgi::Adapter1 adapter;
            for (UINT i = 0; (adapter = getAdapter(factory, i)).valid(); i++) {
                adapters.push_back(adapter);
            }
        } else {
            fprintf(stderr, "d3d12: CreateDXGIFactory2 = 0x%x\n", hr);
            return hr;
        }

        return S_OK;
    }

    void Instance::destroy() {
        for (auto &adapter : adapters) { 
            adapter.tryDrop("adapter");
        }
        factory.tryDrop("factory");
        debug.tryDrop("debug");
    }
}   
