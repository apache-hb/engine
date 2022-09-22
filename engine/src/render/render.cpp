#include "engine/render/render.h"
#include "dx/d3d12.h"
#include "dx/d3dx12.h"
#include "engine/base/panic.h"

#include "dx/dxgiformat.h"
#include "dx/d3d12sdklayers.h"

#include <array>
#include <cmath>
#include <d3dcompiler.h>
#include <dxgi.h>

using namespace engine;
using namespace math;
using namespace engine::render;

#define CHECK(expr) ASSERT(SUCCEEDED(expr))

static void compileText(ID3DBlob **shader, std::string_view text, const char *entry, const char *version, logging::Channel *channel) {
    constexpr auto kCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    Com<ID3DBlob> error;

    HRESULT hr = (D3DCompile(text.data(), text.size(), nullptr, nullptr, nullptr, entry, version, kCompileFlags, 0, shader, &error));

    if (!SUCCEEDED(hr)) {
        channel->fatal("{}", std::string{(const char*)error->GetBufferPointer(), error->GetBufferSize()});
    }

    CHECK(hr);
}

static bool checkTearingSupport(Com<IDXGIFactory4> &factory) {
    auto it = factory.as<IDXGIFactory5>();
    if (!it.valid()) {
        return false;
    }

    BOOL tearing = FALSE;
    CHECK(it->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(BOOL)));
    return tearing;
}

Context::Context(engine::Window *window, logging::Channel *channel) {
    UINT factoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        channel->info("enabled debug layer");

        debug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    CHECK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

    for (UINT i = 0; SUCCEEDED(factory->EnumAdapters1(i, &adapter)); i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        break;
    }

    tearing = checkTearingSupport(factory);

    CHECK(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
    };

    CHECK(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    auto size = window->size();
    channel->info("size: {}x{}", size.width, size.height);

    auto [width, height] = size;

    view = d3d12::View(width, height);

    DXGI_SWAP_CHAIN_DESC1 swapDesc = {
        .Width = static_cast<UINT>(width),
        .Height = static_cast<UINT>(height),
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = kFrameCount,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
    };

    Com<IDXGISwapChain1> swapChain1;
    CHECK(factory->CreateSwapChainForHwnd(
        commandQueue.get(),
        window->handle(),
        &swapDesc,
        nullptr,
        nullptr,
        &swapChain1
    ));
    CHECK(factory->MakeWindowAssociation(window->handle(), DXGI_MWA_NO_ALT_ENTER));
    ASSERT((swapChain = swapChain1.as<IDXGISwapChain3>()).valid());
    
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = kFrameCount
        };
        CHECK(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = 1,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        };
        CHECK(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));
    }

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ rtvHeap->GetCPUDescriptorHandleForHeapStart() };

        for (UINT i = 0; i < kFrameCount; i++) {
            CHECK(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            device->CreateRenderTargetView(renderTargets[i].get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvSize);
            
            CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
        }
    }

    CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[frameIndex].get(), nullptr, IID_PPV_ARGS(&commandList)));

    CHECK(commandList->Close());


    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 range;
        CD3DX12_ROOT_PARAMETER1 parameter;

        range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        parameter.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_VERTEX);

        constexpr D3D12_ROOT_SIGNATURE_FLAGS kRootFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc;
        rootDesc.Init_1_1(1, &parameter, 0, nullptr, kRootFlags);

        Com<ID3DBlob> signature;
        Com<ID3DBlob> error;
        CHECK(D3DX12SerializeVersionedRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        CHECK(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
    }

    {
        Com<ID3DBlob> vertexShader;
        Com<ID3DBlob> pixelShader;

        constexpr auto kShader = R"(
            cbuffer Data : register(b0) {
                float4 offset;
                float4 padding[15];
            };

            struct PSInput {
                float4 pos : SV_POSITION;
                float4 colour : COLOUR;
            };

            PSInput vsMain(float4 pos : POSITION, float4 colour : COLOUR) {
                PSInput result;
                result.pos = pos + offset;
                result.colour = colour;
                return result;
            }

            float4 psMain(PSInput input) : SV_TARGET {
                return input.colour;
            }
        )";

        compileText(&vertexShader, kShader, "vsMain", "vs_5_0", channel);
        compileText(&pixelShader, kShader, "psMain", "ps_5_0", channel);

        D3D12_INPUT_ELEMENT_DESC inputDesc[] = {
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
            .DepthStencilState = {
                .DepthEnable = false,
                .StencilEnable = false
            },
            .InputLayout = {
                .pInputElementDescs = inputDesc,
                .NumElements = sizeof(inputDesc) / sizeof(D3D12_INPUT_ELEMENT_DESC)
            },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .SampleDesc = {
                .Count = 1
            }
        };
        CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    }

    const auto kUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    {
        d3d12::Resource<> buffer;
        auto aspectRatio = size.aspectRatio<float>();
        const Vertex kTriangle[]{
            { { -0.25f, -0.25f * aspectRatio, 0.f }, { 1.f, 0.f, 0.f, 1.f } },
            { { -0.25f, 0.25f * aspectRatio, 0.f }, { 0.f, 1.f, 0.f, 1.f } },
            { { 0.25f, -0.25f * aspectRatio, 0.f }, { 0.f, 0.f, 1.f, 1.f } },
            { { 0.25f, 0.25f * aspectRatio, 0.f }, { 1.f, 1.f, 0.f, 1.f } }
        };

        const auto kBufferSize = sizeof(kTriangle);
        const auto kBuffer = CD3DX12_RESOURCE_DESC::Buffer(kBufferSize);

        CHECK(device->CreateCommittedResource(&kUpload, D3D12_HEAP_FLAG_NONE, &kBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer)));

        void *vertexData;
        CD3DX12_RANGE range(0, 0);
        CHECK(buffer->Map(0, &range, &vertexData));
        memcpy(vertexData, kTriangle, kBufferSize);
        buffer->Unmap(0, nullptr);

        vertexBuffer = d3d12::VertexBuffer(buffer.get(), kBufferSize, sizeof(Vertex));
    }

    {
        d3d12::Resource<> buffer;
        constexpr auto kIndices = std::to_array<UINT32>({
            0, 1, 2,
            2, 1, 3
        });

        const auto kBufferSize = kIndices.size() * sizeof(UINT32);
        const auto kBuffer = CD3DX12_RESOURCE_DESC::Buffer(kBufferSize);

        CHECK(device->CreateCommittedResource(&kUpload, D3D12_HEAP_FLAG_NONE, &kBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer)));

        void *vertexData;
        CD3DX12_RANGE range(0, 0);
        CHECK(buffer->Map(0, &range, &vertexData));
        memcpy(vertexData, kIndices.data(), kBufferSize);
        buffer->Unmap(0, nullptr);

        indexBuffer = d3d12::IndexBuffer(buffer.get(), kBufferSize, DXGI_FORMAT_R32_UINT);
    }

    {
        constexpr auto kBufferSize = sizeof(ConstBuffer);

        const auto kBuffer = CD3DX12_RESOURCE_DESC::Buffer(kBufferSize);

        CHECK(device->CreateCommittedResource(&kUpload, D3D12_HEAP_FLAG_NONE, &kBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer)));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
            .BufferLocation = constantBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = kBufferSize
        };
        device->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());

        CD3DX12_RANGE range(0, 0);
        CHECK(constantBuffer->Map(0, &range, &bufferPtr));
        memcpy(bufferPtr, &bufferData, sizeof(ConstBuffer));
    }

    {
        CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceValue = 1;

        fenceEvent = CreateEvent(nullptr, false, false, nullptr);
        ASSERTF(fenceEvent != nullptr, "{:x}", HRESULT_FROM_WIN32(GetLastError()));
    }

    waitForFrame();
}

void addTransition(ID3D12GraphicsCommandList *list, ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);

    list->ResourceBarrier(1, &barrier);
}

void Context::begin(float elapsed) {
    bufferData.offset.x = std::sinf(elapsed) / 2.f;
    bufferData.offset.y = std::cosf(elapsed) / 2.f;
    memcpy(bufferPtr, &bufferData, sizeof(ConstBuffer));

    CHECK(commandAllocators[frameIndex]->Reset());

    CHECK(commandList->Reset(commandAllocators[frameIndex].get(), pipelineState.get()));

    commandList->SetGraphicsRootSignature(rootSignature.get());

    ID3D12DescriptorHeap *heaps[] = { cbvHeap.get() };
    commandList->SetDescriptorHeaps(sizeof(heaps) / sizeof(ID3D12DescriptorHeap*), heaps);

    commandList->SetGraphicsRootDescriptorTable(0, cbvHeap->GetGPUDescriptorHandleForHeapStart());
    commandList->RSSetViewports(1, &view.viewport);
    commandList->RSSetScissorRects(1, &view.scissor);

    addTransition(commandList.get(), renderTargets[frameIndex].get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvSize);
    commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    constexpr float kClear[] = { 0.f, 0.2f, 0.4f, 1.f };
    commandList->ClearRenderTargetView(rtvHandle, kClear, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBuffer.view);
    commandList->IASetIndexBuffer(&indexBuffer.view);
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    
    addTransition(commandList.get(), renderTargets[frameIndex].get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    CHECK(commandList->Close());
}

void Context::end() {
    ID3D12CommandList *lists[] = { commandList.get() };
    commandQueue.execute(lists);

    auto flags = tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

    CHECK(swapChain->Present(0, flags));

    waitForFrame();
}

void Context::close() {
    waitForFrame();

    CloseHandle(fenceEvent);
}

void Context::waitForFrame() {
    const auto oldValue = fenceValue;
    CHECK(commandQueue->Signal(fence.get(), oldValue));
    fenceValue += 1;

    if (fence->GetCompletedValue() < oldValue) {
        CHECK(fence->SetEventOnCompletion(oldValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    frameIndex = swapChain->GetCurrentBackBufferIndex();
}
