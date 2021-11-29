#include "dx12.h"

#include "d3dx12.h"
#include <d3dcompiler.h>
#include <dxgidebug.h>

#include <format>

namespace {
    void ensure(HRESULT hr, const char *expr) {
        if (FAILED(hr)) {
            fprintf(stderr, "panic %s: 0x%x\n", expr, hr);
            ExitProcess(99);
        }
    }
}

#define ENSURE(expr) ensure(expr, #expr)

namespace render {
    void Factory::create(bool debugging) {
        UINT flags = 0;
        if (debugging) {
            ENSURE(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
            debug->EnableDebugLayer();
            flags = DXGI_CREATE_FACTORY_DEBUG;
        }

        ENSURE(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));

        dxgi::Adapter1 adapter;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            adapters.push_back(adapter);
        }
    }

    void Factory::destroy() {
        for (auto adapter : adapters) {
            adapter.drop();
        }
        factory.drop();
        debug.drop();
    }

    void Context::create(Factory *factory, size_t index) {
        this->factory = factory;
        this->index = index;

        auto adapter = getAdapter();
        ENSURE(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
    
        auto callback = [](D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR msg, void *ctx) {
            printf("d3d12: %hs\n", msg);
        };

        auto msg = device.as<d3d12::InfoQueue1>();
        if (msg.valid()) {
            DWORD cookie = 0;
            ENSURE(msg->RegisterMessageCallback(callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie));
            msg.release();
        }

        D3D12_COMMAND_QUEUE_DESC desc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
        };

        ENSURE(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
    }

    void Context::destroy() {
        commandQueue.drop();
        device.drop();
    }

    void Context::attach(WindowHandle *handle, UINT frames) {
        this->handle = handle;
        this->frames = frames;

        auto [width, height] = handle->getClientSize();

        viewport = Viewport(FLOAT(width), FLOAT(height));
        scissor = Scissor(UINT(width), UINT(height));

        DXGI_SWAP_CHAIN_DESC1 desc = {
            .Width = UINT(width),
            .Height = UINT(height),
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { .Count = 1 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = frames,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
        };

        auto factory = getFactory();
        auto queue = commandQueue.get();
        auto window = handle->getHandle();

        dxgi::SwapChain1 swap;
        ENSURE(factory->CreateSwapChainForHwnd(
            /* pDevice = */ queue, 
            /* hWnd = */ window, 
            /* pDesc = */ &desc,
            /* pFullscreenDesc = */ nullptr, 
            /* pRestrictToOutput = */ nullptr, 
            /* ppSwapChain = */ swap.addr()
        ));

        ENSURE(factory->MakeWindowAssociation(
            /* hWnd = */ handle->getHandle(), 
            /* dwFlags = */ DXGI_MWA_NO_ALT_ENTER
        ));

        swapChain = swap.as<dxgi::SwapChain3>();
        swap.release();
        currentFrame = swapChain->GetCurrentBackBufferIndex();

        D3D12_DESCRIPTOR_HEAP_DESC cbvDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = 1,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        };

        ENSURE(device->CreateDescriptorHeap(&cbvDesc, IID_PPV_ARGS(&cbvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = frames,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        };

        ENSURE(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap)));
        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

        renderTargets = new d3d12::Resource[frames];
        commandAllocators = new d3d12::CommandAllocator[frames];
        fenceValues = new UINT64[frames];

        for (UINT i = 0; i < frames; i++) {
            auto rtv = renderTargets + i;
            ENSURE(swapChain->GetBuffer(i, IID_PPV_ARGS(rtv->addr())));
            device->CreateRenderTargetView(rtv->get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize);
            rtv->release();
            rtv->rename(std::format(TEXT("render target {}/{}"), i + 1, frames).c_str());
            ENSURE(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
        }

        ENSURE(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&bundleAllocator)));
    }

    void Context::detach() {
        bundleAllocator.drop();

        for (UINT i = 0; i < frames; i++) {
            commandAllocators[i].drop();
        }

        for (UINT i = 0; i < frames; i++) {
            renderTargets[i].drop();
        }

        rtvHeap.drop();
        swapChain.drop();

        delete[] commandAllocators;
        delete[] renderTargets;
        delete[] fenceValues;
    }

    void Context::load() {
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        d3d::Blob signature;
        d3d::Blob error;

        ENSURE(D3DX12SerializeVersionedRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, signature.addr(), error.addr()));
        ENSURE(device->CreateRootSignature(0, 
            signature->GetBufferPointer(), 
            signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)
        ));

        d3d::Blob vertexShader;
        d3d::Blob pixelShader;

        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        ENSURE(D3DCompileFromFile(
            L"shaders/shader.hlsl", 
            nullptr, 
            nullptr, 
            "VSMain", 
            "vs_5_0", 
            compileFlags, 
            0, 
            vertexShader.addr(), 
            error.addr()
        ));

        ENSURE(D3DCompileFromFile(
            L"shaders/shader.hlsl", 
            nullptr, 
            nullptr, 
            "PSMain", 
            "ps_5_0", 
            compileFlags, 
            0, 
            pixelShader.addr(), 
            error.addr()
        ));

        D3D12_INPUT_ELEMENT_DESC inputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {
            .pRootSignature = rootSignature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vertexShader.get()),
            .PS = CD3DX12_SHADER_BYTECODE(pixelShader.get()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .InputLayout = { inputElements, _countof(inputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .SampleDesc = { .Count = 1 }
        };
        
        ENSURE(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState)));

        ENSURE(device->CreateCommandList(0, 
            D3D12_COMMAND_LIST_TYPE_DIRECT, 
            commandAllocators[currentFrame].get(), 
            pipelineState.get(), 
            IID_PPV_ARGS(&commandList)
        ));

        ENSURE(commandList->Close());
        
        auto [width, height] = handle->getClientSize();
        auto aspect = float(width) / float(height);

        Vertex triangle[] = {
            { { 0.0f, 0.25f * aspect, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        UINT vertexBufferSize = sizeof(triangle);

        const auto upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ENSURE(device->CreateCommittedResource(
            &upload,
            D3D12_HEAP_FLAG_NONE,
            &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer)
        ));

        vertexBuffer.rename(TEXT("Vertex Buffer"));

        void *mapped = nullptr;
        const auto range = CD3DX12_RANGE(0, 0);
        ENSURE(vertexBuffer->Map(0, &range, &mapped));
        memcpy(mapped, triangle, vertexBufferSize);
        vertexBuffer->Unmap(0, nullptr);

        vertexBufferView = {
            .BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = vertexBufferSize,
            .StrideInBytes = sizeof(Vertex)
        };

        ENSURE(device->CreateCommandList(0, 
            D3D12_COMMAND_LIST_TYPE_BUNDLE, 
            bundleAllocator.get(), 
            pipelineState.get(),
            IID_PPV_ARGS(&bundleList)
        ));
        bundleList->SetGraphicsRootSignature(rootSignature.get());
        bundleList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        bundleList->IASetVertexBuffers(0, 1, &vertexBufferView);
        bundleList->DrawInstanced(3, 1, 0, 0);
        ENSURE(bundleList->Close());

        ENSURE(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceValues[currentFrame] = 1;

        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            ENSURE(HRESULT_FROM_WIN32(GetLastError()));
        }

        gpuWait();
    }

    void Context::unload() {
        gpuWait();

        CloseHandle(fenceEvent);
        fence.drop();
        vertexBuffer.drop();
        commandList.drop();
        pipelineState.drop();
        rootSignature.drop();
    }

    void Context::gpuWait() {
        auto &fenceValue = fenceValues[currentFrame];
        ENSURE(commandQueue->Signal(fence.get(), fenceValue));

        ENSURE(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

        fenceValue += 1;
    }

    void Context::nextFrame() {
        auto fenceValue = fenceValues[currentFrame];
        ENSURE(commandQueue->Signal(fence.get(), fenceValue));

        currentFrame = swapChain->GetCurrentBackBufferIndex();

        if (fence->GetCompletedValue() < fenceValue) {
            ENSURE(fence->SetEventOnCompletion(fenceValue, fenceEvent));
            WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
        }

        fenceValues[currentFrame] = fenceValue + 1;
    }

    void Context::present() {
        gpuWait();

        ENSURE(commandAllocators[currentFrame]->Reset());
        ENSURE(commandList->Reset(commandAllocators[currentFrame].get(), pipelineState.get()));

        commandList->SetGraphicsRootSignature(rootSignature.get());
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);

        auto toTarget = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[currentFrame].get(),
            D3D12_RESOURCE_STATE_PRESENT, 
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        commandList->ResourceBarrier(1, &toTarget);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrame, rtvDescriptorSize);
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        const float clear[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clear, 0, nullptr);

        commandList->ExecuteBundle(bundleList.get());

        auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[currentFrame].get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );
        commandList->ResourceBarrier(1, &toPresent);

        ENSURE(commandList->Close());

        ID3D12CommandList *commandLists[] = { commandList.get() };
        commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        ENSURE(swapChain->Present(1, 0));

        nextFrame();
    }

    dxgi::Adapter1 Context::getAdapter() const {
        return factory->adapters[index];
    }

    dxgi::Factory4 Context::getFactory() const {
        return factory->factory;
    }
}
