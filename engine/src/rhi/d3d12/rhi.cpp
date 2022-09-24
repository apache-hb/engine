#include "engine/rhi/rhi.h"

#include "engine/base/panic.h"

#include <comdef.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include "dx/d3dx12.h"

using namespace engine;

namespace {
    IDXGIFactory4 *gFactory;
    ID3D12Debug *gDxDebug;
    IDXGIDebug *gDebug;

    constexpr auto kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    void drop(IUnknown *ptr, std::string_view name) {
        ULONG refs = ptr->Release();
        ASSERTF(refs == 0, "failed to drop {}, {} live references", name, refs);
    }

    constexpr D3D12_COMMAND_LIST_TYPE getCommandListType(rhi::CommandList::Type type) {
        switch (type) {
        case rhi::CommandList::eCopy: return D3D12_COMMAND_LIST_TYPE_COPY;
        case rhi::CommandList::eDirect: return D3D12_COMMAND_LIST_TYPE_DIRECT;
        default: return D3D12_COMMAND_LIST_TYPE();
        }
    }

    constexpr const D3D12_HEAP_PROPERTIES *getHeapProps(rhi::Buffer::State type) {
        switch (type) {
        case rhi::Buffer::State::eUpload: return &kUploadProps;
        default: return &kDefaultProps;
        }
    }

    constexpr D3D12_RESOURCE_STATES getResourceState(rhi::Buffer::State type) {
        switch (type) {
        case rhi::Buffer::State::eUpload: return D3D12_RESOURCE_STATE_GENERIC_READ;
        case rhi::Buffer::State::eIndex: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case rhi::Buffer::State::eVertex: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case rhi::Buffer::State::eCopyDst: return D3D12_RESOURCE_STATE_COPY_DEST;
        case rhi::Buffer::State::eRenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case rhi::Buffer::State::ePresent: return D3D12_RESOURCE_STATE_PRESENT;
        default: return D3D12_RESOURCE_STATES();
        }
    }

    constexpr DXGI_FORMAT getElementFormat(rhi::Format format) {
        switch (format) {
        case rhi::Format::uint32: return DXGI_FORMAT_R32_UINT;
        
        case rhi::Format::float32: return DXGI_FORMAT_R32_FLOAT;
        case rhi::Format::float32x2: return DXGI_FORMAT_R32G32_FLOAT;
        case rhi::Format::float32x3: return DXGI_FORMAT_R32G32B32_FLOAT;
        case rhi::Format::float32x4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }

    constexpr size_t getElementSize(rhi::Format format) {
        switch (format) {
        case rhi::Format::uint32: return sizeof(uint32_t);
        
        case rhi::Format::float32: return sizeof(float);
        case rhi::Format::float32x2: return sizeof(math::float2);
        case rhi::Format::float32x3: return sizeof(math::float3);
        case rhi::Format::float32x4: return sizeof(math::float4);
        
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }

    constexpr D3D12_DESCRIPTOR_HEAP_TYPE getDescriptorHeapType(rhi::Object::Type type) {
        if (type & (rhi::Object::eTexture)) {
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        }

        if (type & rhi::Object::eRenderTarget) {
            return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        }

        return D3D12_DESCRIPTOR_HEAP_TYPE();
    }

    constexpr D3D12_SHADER_VISIBILITY getShaderVisibility(rhi::ShaderVisibility visibility) {
        switch (visibility) {
        case rhi::ShaderVisibility::ePixel: return D3D12_SHADER_VISIBILITY_PIXEL;
        case rhi::ShaderVisibility::eVertex: return D3D12_SHADER_VISIBILITY_VERTEX;

        default: return D3D12_SHADER_VISIBILITY();
        }
    }

    constexpr D3D12_DESCRIPTOR_RANGE_TYPE getRangeType(rhi::Object::Type type) {
        switch (type) {
        case rhi::Object::eTexture: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        default: return D3D12_DESCRIPTOR_RANGE_TYPE();
        }
    }

    std::string hrErrorString(HRESULT hr) {
        _com_error err(hr);
        return err.ErrorMessage();
    }
}

#define DX_CHECK(expr) do { HRESULT hr = (expr); ASSERTF(SUCCEEDED(hr), #expr " => {}", hrErrorString(hr)); } while (0)

struct DxAllocator final : rhi::Allocator {
    DxAllocator(ID3D12CommandAllocator *allocator)
        : allocator(allocator)
    { }

    ~DxAllocator() override {
        drop(allocator, "allocator");
    }

    void reset() {
        DX_CHECK(allocator->Reset());
    }

    ID3D12CommandAllocator *get() { return allocator; }
private:
    ID3D12CommandAllocator *allocator;
};

struct DxFence final : rhi::Fence {
    DxFence(ID3D12Fence *fence, HANDLE event)
        : fence(fence)
        , event(event)
    { }

    ~DxFence() override {
        drop(fence, "fence");
        CloseHandle(event);
    }

    void waitUntil(size_t signal) override {
        if (fence->GetCompletedValue() < signal) {
            DX_CHECK(fence->SetEventOnCompletion(signal, event));
            WaitForSingleObject(event, INFINITE);
        }
    }

    ID3D12Fence *get() { return fence; }

private:
    ID3D12Fence *fence;
    HANDLE event;
};

struct DxDescriptorSet final : rhi::DescriptorSet {
    DxDescriptorSet(ID3D12DescriptorHeap *heap, UINT stride)
        : heap(heap)
        , stride(stride)
    { }

    ~DxDescriptorSet() override {
        drop(heap, "descriptor-heap");
    }

    virtual rhi::CpuHandle cpuHandle(size_t offset) override {
        const auto kCpuHeapStart = heap->GetCPUDescriptorHandleForHeapStart();
        return rhi::CpuHandle(kCpuHeapStart.ptr + (offset * stride));
    }

    virtual rhi::GpuHandle gpuHandle(size_t offset) override {
        const auto kGpuHeapStart = heap->GetGPUDescriptorHandleForHeapStart();
        return rhi::GpuHandle(kGpuHeapStart.ptr + (offset * stride));
    }

    ID3D12DescriptorHeap *get() { return heap; }

private:
    ID3D12DescriptorHeap *heap;
    UINT stride;
};

struct DxBuffer : rhi::Buffer {
    DxBuffer(ID3D12Resource *resource)
        : resource(resource)
    { }

    ~DxBuffer() override {
        drop(resource, "buffer");
    }

    void write(const void *src, size_t size) override {
        void *ptr;
        CD3DX12_RANGE range(0, 0);

        DX_CHECK(resource->Map(0, &range, &ptr));
        memcpy(ptr, src, size);
        resource->Unmap(0, &range);
    }
    
    rhi::GpuHandle gpuAddress() override {
        return rhi::GpuHandle(resource->GetGPUVirtualAddress());
    }

    ID3D12Resource *get() { return resource; }
private:
    ID3D12Resource *resource;
};

/// treat render targets specially because they have different release semantics
struct DxRenderTarget final : DxBuffer {
    using DxBuffer::DxBuffer;

    ~DxRenderTarget() override {
        get()->Release();
    }
};

struct DxSwapChain final : rhi::SwapChain {
    DxSwapChain(IDXGISwapChain3 *swapchain)
        : swapchain(swapchain)
    { }

    ~DxSwapChain() override {
        drop(swapchain, "swapchain");
    }

    void present() override {
        DX_CHECK(swapchain->Present(0, 0));
    }

    rhi::Buffer *getBuffer(size_t index) override {
        ID3D12Resource *resource;
        DX_CHECK(swapchain->GetBuffer(UINT(index), IID_PPV_ARGS(&resource)));

        return new DxRenderTarget(resource);
    }

    size_t currentBackBuffer() override {
        return swapchain->GetCurrentBackBufferIndex();
    }

private:
    IDXGISwapChain3 *swapchain;
};

struct DxPipelineState final : rhi::PipelineState {
    DxPipelineState(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline)
        : signature(signature)
        , pipeline(pipeline) 
    { }

    ~DxPipelineState() override {
        drop(pipeline, "pipeline");
        drop(signature, "signature");
    }

    ID3D12RootSignature *getSignature() { return signature; }
    ID3D12PipelineState *getPipelineState() { return pipeline; }
private:
    ID3D12RootSignature *signature;
    ID3D12PipelineState *pipeline;
};

struct DxCommandList : rhi::CommandList {
    DxCommandList(ID3D12GraphicsCommandList *commands)
        : commands(commands)
    { }

    ~DxCommandList() override {
        drop(commands, "command-list");
    }

    void beginRecording(rhi::Allocator *allocator) override {
        auto *alloc = static_cast<DxAllocator*>(allocator);
        alloc->reset();

        DX_CHECK(commands->Reset(alloc->get(), nullptr));
    }
    
    void endRecording() override {
        DX_CHECK(commands->Close());
    }

    void setRenderTarget(rhi::CpuHandle target, const math::float4 &colour) override {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv { size_t(target) };
        const float kClear[] = { colour.x, colour.y, colour.z, colour.w };

        commands->OMSetRenderTargets(1, &rtv, false, nullptr);
        commands->ClearRenderTargetView(rtv, kClear, 0, nullptr);
    }

    void setViewport(const rhi::Viewport &view) override {
        const D3D12_VIEWPORT kViewport = {
            .TopLeftX = 0.f,
            .TopLeftY = 0.f,
            .Width = view.width,
            .Height = view.height,
            .MinDepth = D3D12_MIN_DEPTH,
            .MaxDepth = D3D12_MAX_DEPTH
        };

        const D3D12_RECT kScissor = {
            .left = 0,
            .top = 0,
            .right = LONG(view.width),
            .bottom = LONG(view.height)
        };

        commands->RSSetViewports(1, &kViewport);
        commands->RSSetScissorRects(1, &kScissor);
    }

    void copyBuffer(rhi::Buffer *dst, rhi::Buffer *src, size_t size) override {
        auto *input = static_cast<DxBuffer*>(src);
        auto *output = static_cast<DxBuffer*>(dst);

        commands->CopyBufferRegion(output->get(), 0, input->get(), 0, size);
    }

    void transition(rhi::Buffer *buffer, rhi::Buffer::State before, rhi::Buffer::State after) override {
        auto *resource = static_cast<DxBuffer*>(buffer);
        
        transitionBarrier(resource->get(), getResourceState(before), getResourceState(after));
    }

    void bindDescriptors(std::span<rhi::DescriptorSet*> sets) override {
        auto heaps = std::unique_ptr<ID3D12DescriptorHeap*[]>(new ID3D12DescriptorHeap*[sets.size()]);
        for (size_t i = 0; i < sets.size(); i++) {
            auto *set = static_cast<DxDescriptorSet*>(sets[i]);
            heaps[i] = set->get();
        }

        commands->SetDescriptorHeaps(UINT(sets.size()), heaps.get());
    }

    void setPipeline(rhi::PipelineState *pipeline) override {
        auto *pso = static_cast<DxPipelineState*>(pipeline);
        commands->SetGraphicsRootSignature(pso->getSignature());
        commands->SetPipelineState(pso->getPipelineState());
    }

    void drawMesh(const rhi::IndexBufferView &indexView, const rhi::VertexBufferView &vertexView) override {
        const D3D12_VERTEX_BUFFER_VIEW kVertexBufferView = {
            .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(vertexView.buffer->gpuAddress()),
            .SizeInBytes = UINT(vertexView.size * vertexView.stride),
            .StrideInBytes = UINT(vertexView.stride)
        };
        
        const D3D12_INDEX_BUFFER_VIEW kIndexBufferView = {
            .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(indexView.buffer->gpuAddress()),
            .SizeInBytes = UINT(indexView.size * getElementSize(indexView.format)),
            .Format = getElementFormat(indexView.format)
        };

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commands->IASetVertexBuffers(0, 1, &kVertexBufferView);
        commands->IASetIndexBuffer(&kIndexBufferView);
        commands->DrawIndexedInstanced(UINT(indexView.size), 1, 0, 0, 0);
    }

    ID3D12GraphicsCommandList *get() { return commands; }

private:
    void transitionBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
        const auto kBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);

        commands->ResourceBarrier(1, &kBarrier);
    }

    ID3D12GraphicsCommandList *commands;
};

struct DxCommandQueue final : rhi::CommandQueue {
    DxCommandQueue(ID3D12CommandQueue *queue)
        : queue(queue)
    { }

    ~DxCommandQueue() override {
        drop(queue, "command queue");
    }

    rhi::SwapChain *newSwapChain(Window *window, size_t buffers) override {
        auto [width, height] = window->size();
        HWND handle = window->handle();

        const DXGI_SWAP_CHAIN_DESC1 kDesc = {
            .Width = static_cast<UINT>(width),
            .Height = static_cast<UINT>(height),
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = {
                .Count = 1
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = static_cast<UINT>(buffers),
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        };

        IDXGISwapChain1 *swapchain;
        DX_CHECK(gFactory->CreateSwapChainForHwnd(
            queue,
            handle,
            &kDesc,
            nullptr,
            nullptr,
            &swapchain
        ));

        DX_CHECK(gFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));
    
        IDXGISwapChain3 *swapchain3;
        DX_CHECK(swapchain->QueryInterface(IID_PPV_ARGS(&swapchain3)));
        ASSERT(swapchain->Release() == 1);

        return new DxSwapChain(swapchain3);
    }

    void signal(rhi::Fence *fence, size_t value) override {
        auto *it = static_cast<DxFence*>(fence);
        DX_CHECK(queue->Signal(it->get(), value));
    }

    void execute(std::span<rhi::CommandList*> lists) override {
        auto data = std::unique_ptr<ID3D12CommandList*[]>(new ID3D12CommandList*[lists.size()]);
        for (size_t i = 0; i < lists.size(); i++) {
            auto *list = static_cast<DxCommandList*>(lists[i]);
            data[i] = list->get();
        }

        queue->ExecuteCommandLists(UINT(lists.size()), data.get());
    }

private:
    ID3D12CommandQueue *queue;
};

struct DxDevice final : rhi::Device {
    DxDevice(ID3D12Device *device, D3D_ROOT_SIGNATURE_VERSION version)
        : device(device)
        , kHighestVersion(version)
    { }

    ~DxDevice() override {
        drop(device, "device");
    }

    rhi::Fence *newFence() override {
        HANDLE event = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(event != nullptr);

        ID3D12Fence *fence;
        DX_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        return new DxFence(fence, event);
    }

    rhi::CommandQueue *newQueue(rhi::CommandList::Type type) override {
        const D3D12_COMMAND_QUEUE_DESC kDesc = {
            .Type = getCommandListType(type)
        };

        ID3D12CommandQueue *queue;
        DX_CHECK(device->CreateCommandQueue(&kDesc, IID_PPV_ARGS(&queue)));

        return new DxCommandQueue(queue);
    }

    rhi::CommandList *newCommandList(rhi::Allocator *allocator, rhi::CommandList::Type type) override {
        auto *alloc = static_cast<DxAllocator*>(allocator);

        ID3D12GraphicsCommandList *commands;
        DX_CHECK(device->CreateCommandList(0, getCommandListType(type), alloc->get(), nullptr, IID_PPV_ARGS(&commands)));
        DX_CHECK(commands->Close());

        return new DxCommandList(commands);
    }
    
    rhi::Allocator *newAllocator(rhi::CommandList::Type type) override {
        ID3D12CommandAllocator *allocator;
        DX_CHECK(device->CreateCommandAllocator(getCommandListType(type), IID_PPV_ARGS(&allocator)));

        return new DxAllocator(allocator);
    }

    rhi::DescriptorSet *newDescriptorSet(size_t count, rhi::Object::Type type, bool shaderVisible) override {
        const auto kHeapType = getDescriptorHeapType(type);
        const D3D12_DESCRIPTOR_HEAP_DESC kRtvHeapDesc {
            .Type = kHeapType,
            .NumDescriptors = UINT(count),
            .Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        };

        ID3D12DescriptorHeap *rtvHeap;
        DX_CHECK(device->CreateDescriptorHeap(&kRtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        UINT stride = device->GetDescriptorHandleIncrementSize(kHeapType);

        return new DxDescriptorSet(rtvHeap, stride);
    }

    void createRenderTargetView(rhi::Buffer *target, rhi::CpuHandle rtvHandle) override {
        auto *dxTarget = static_cast<DxBuffer*>(target);
        D3D12_CPU_DESCRIPTOR_HANDLE handle { size_t(rtvHandle) };
        device->CreateRenderTargetView(dxTarget->get(), nullptr, handle);
    }

    void createShaderResourceView(rhi::Buffer *buffer, rhi::CpuHandle srvHandle) override {
        auto *it = static_cast<DxBuffer*>(buffer);
        D3D12_CPU_DESCRIPTOR_HANDLE handle { size_t(srvHandle) };
        device->CreateShaderResourceView(it->get(), nullptr, handle);
    }

    rhi::Buffer *newBuffer(size_t size, rhi::Buffer::State state) override {
        const auto kBufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);

        ID3D12Resource *resource;
        DX_CHECK(device->CreateCommittedResource(
            getHeapProps(state), 
            D3D12_HEAP_FLAG_NONE, 
            &kBufferSize, 
            getResourceState(state), 
            nullptr, 
            IID_PPV_ARGS(&resource)
        ));
        
        return new DxBuffer(resource);
    }

    rhi::PipelineState *newPipelineState(const rhi::PipelineBinding& bindings) override {
        const auto vs = createShader(bindings.vs);
        const auto ps = createShader(bindings.ps);

        auto input = createInputLayout(bindings.input);

        ID3D12RootSignature *root = createRootSignature(bindings.samplers, bindings.bindings);
        ID3D12PipelineState *pipeline = createPipelineState(root, vs, ps, input);
        return new DxPipelineState(root, pipeline);
    }

private:
    static std::vector<D3D12_INPUT_ELEMENT_DESC> createInputLayout(std::span<rhi::InputElement> layout) {
        std::vector<D3D12_INPUT_ELEMENT_DESC> input{layout.size()};
    
        for (size_t i = 0; i < layout.size(); i++) {
            const auto &item = layout[i];
            D3D12_INPUT_ELEMENT_DESC desc { item.name.c_str(), 0, getElementFormat(item.format), 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            input[i] = desc;
        }

        return input;
    }

    static constexpr D3D12_SHADER_BYTECODE createShader(std::span<std::byte> bytecode) {
        return D3D12_SHADER_BYTECODE {bytecode.data(), bytecode.size()};
    }

    static constexpr D3D12_STATIC_SAMPLER_DESC createSampler(rhi::Sampler sampler) {
        return D3D12_STATIC_SAMPLER_DESC {
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
            .ShaderVisibility = getShaderVisibility(sampler.visibility)
        };
    }

    static std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> createSamplers(std::span<rhi::Sampler> samplers) {
        if (samplers.size() == 0) {
            return nullptr;
        }

        auto it = std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]>(new D3D12_STATIC_SAMPLER_DESC[samplers.size()]);

        for (size_t i = 0; i < samplers.size(); i++) {
            it[i] = createSampler(samplers[i]);
        }

        return it;
    }

    static constexpr D3D12_DESCRIPTOR_RANGE1 createRange(rhi::Binding binding) {
        return D3D12_DESCRIPTOR_RANGE1 {
            .RangeType = getRangeType(binding.type),
            .NumDescriptors = 1,
            .BaseShaderRegister = UINT(binding.base),
            .RegisterSpace = 0,
            .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC
        };
    }

    static std::vector<D3D12_DESCRIPTOR_RANGE1> createBindingRange(std::span<rhi::Binding> bindings) {
        if (bindings.size() == 0) {
            return {};
        }

        std::vector<D3D12_DESCRIPTOR_RANGE1> ranges{bindings.size()};
        for (size_t i = 0; i < bindings.size(); i++) {
            ranges[i] = createRange(bindings[i]);
        }

        return ranges;
    }

    ID3D12RootSignature *createRootSignature(std::span<rhi::Sampler> samplers, std::span<rhi::Binding> bindings) {
        constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        auto samplerData = createSamplers(samplers);

        auto ranges = createBindingRange(bindings);

        CD3DX12_ROOT_PARAMETER1 param;
        param.InitAsDescriptorTable(UINT(ranges.size()), ranges.data());

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, &param, UINT(samplers.size()), samplerData.get(), kFlags);

        ID3DBlob *signature;
        ID3DBlob *error;

        ID3D12RootSignature *root;

        DX_CHECK(D3DX12SerializeVersionedRootSignature(&desc, kHighestVersion, &signature, &error));
        DX_CHECK(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root)));
    
        drop(signature, "signature");
        
        if (error != nullptr) {
            drop(error, "error");
        }

        return root;
    }

    ID3D12PipelineState *createPipelineState(ID3D12RootSignature *root, D3D12_SHADER_BYTECODE vertex, D3D12_SHADER_BYTECODE pixel, std::span<D3D12_INPUT_ELEMENT_DESC> input) {
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = root,
            .VS = vertex,
            .PS = pixel,
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = {
                .DepthEnable = false,
                .StencilEnable = false
            },
            .InputLayout = {
                .pInputElementDescs = input.data(),
                .NumElements = UINT(input.size())
            },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .SampleDesc = {
                .Count = 1
            }
        };

        ID3D12PipelineState *pipeline;
        DX_CHECK(device->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&pipeline)));
        return pipeline;
    }

    ID3D12Device *device;
    const D3D_ROOT_SIGNATURE_VERSION kHighestVersion;
};

rhi::Device *rhi::getDevice() {
    DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&gDebug)));

    UINT factoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&gDxDebug)))) {
        gDxDebug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    DX_CHECK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&gFactory)));

    IDXGIAdapter1 *adapter;
    for (UINT i = 0; SUCCEEDED(gFactory->EnumAdapters1(i, &adapter)); i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        break;
    }

    ID3D12Device *device;
    DX_CHECK(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    return new DxDevice(device, featureData.HighestVersion);
}
