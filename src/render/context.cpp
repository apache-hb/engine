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
        if (HRESULT hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3D12CreateDevice = 0x%x\n", hr);
            return hr;
        }

        if ((infoQueue = device.as<d3d12::InfoQueue1>(L"d3d12-info-queue")).valid()) {
            DWORD cookie = 0;
            if (HRESULT hr = infoQueue->RegisterMessageCallback(infoQueueCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie); FAILED(hr)) {
                fprintf(stderr, "d3d12: RegisterMessageCallback = 0x%x\n", hr);
            }
        }

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
        auto [width, height] = window->getClientSize();
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

        if (HRESULT hr = factory->MakeWindowAssociation(window->getHandle(), DXGI_MWA_NO_ALT_ENTER); FAILED(hr)) {
            fprintf(stderr, "d3d12: MakeWindowAssociation = 0x%x\n", hr);
            return hr;
        }

        if ((swapChain = swap.as<dxgi::SwapChain3>(L"d3d12-swapchain")).valid()) {
            swap.release();
        } else {
            return E_NOINTERFACE;
        }

        frameIndex = swapChain->GetCurrentBackBufferIndex();
        
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = frames,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        };
        
        if (HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.addr())); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateDescriptorHeap = 0x%x\n", hr);
            return hr;
        }

        renderTargets = new d3d12::Resource[frames];
        commandAllocators = new d3d12::CommandAllocator[frames];

        rtvHeap.descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < frames; i++) {
            d3d12::Resource *target = renderTargets + i;
            if (HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(target->addr())); FAILED(hr)) {
                fprintf(stderr, "d3d12: GetBuffer(%u) = 0x%x\n", i, hr);
                return hr;
            }

            device->CreateRenderTargetView(target->get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvHeap.descriptorSize);

            if (HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocators->addr())); FAILED(hr)) {
                fprintf(stderr, "d3d12: CreateCommandAllocator(%u) = 0x%x\n", i, hr);
                return hr;
            }

            target->release();
        }

        return S_OK;
    }

    void Context::detach() {
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
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        d3d::Blob signature;
        d3d::Blob error;

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

        if (HRESULT hr = compileFromFile(L"shaders/shader.hlsl", "VSMain", "vs_5_0", &vertexShader); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3DCompileFromFile 0x%x\n", hr);
            return hr;
        }

        if (HRESULT hr = compileFromFile(L"shaders/shader.hlsl", "PSMain", "ps_5_0", &pixelShader); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3DCompileFromFile 0x%x\n", hr);
            return hr;
        }

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

        if (HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateGraphicsPipelineState 0x%x\n", hr);
            return hr;
        }

        if (HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[frameIndex].get(), pipelineState.get(), IID_PPV_ARGS(&commandList)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateCommandList 0x%x\n", hr);
            return hr;
        }

        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            fprintf(stderr, "d3d12: Close 0x%x\n", hr);
            return hr;
        }

        return S_OK;
    }

    void Context::unload() {
        commandList.tryDrop("command-list");
        pipelineState.tryDrop("pipeline-state");
        rootSignature.tryDrop("root-signature");
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
