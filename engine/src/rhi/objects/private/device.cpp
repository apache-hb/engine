#include "objects/device.h"

#include "objects/fence.h"
#include "objects/allocator.h"
#include "objects/buffer.h"
#include "objects/commands.h"
#include "objects/queue.h"
#include "objects/descriptors.h"
#include "objects/pipeline.h"

#include "imgui_impl_dx12.h"

using namespace engine;

namespace {
    constexpr auto kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    constexpr auto kDepthFormat = DXGI_FORMAT_D32_FLOAT;

    constexpr D3D12_DEPTH_STENCIL_VIEW_DESC kDsvDesc {
        .Format = kDepthFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D
    };

    constexpr D3D12_COMMAND_LIST_TYPE getCommandListType(rhi::CommandList::Type type) {
        switch (type) {
        case rhi::CommandList::eCopy: return D3D12_COMMAND_LIST_TYPE_COPY;
        case rhi::CommandList::eDirect: return D3D12_COMMAND_LIST_TYPE_DIRECT;
        default: return D3D12_COMMAND_LIST_TYPE();
        }
    }

    constexpr const D3D12_HEAP_PROPERTIES *getHeapProps(rhi::DescriptorSet::Visibility visibility) {
        switch (visibility) {
        case rhi::DescriptorSet::Visibility::eDeviceOnly: return &kDefaultProps;
        case rhi::DescriptorSet::Visibility::eHostVisible: return &kUploadProps;
        default: return nullptr;
        }
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
        case rhi::Object::eConstBuffer: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

        default: return D3D12_DESCRIPTOR_RANGE_TYPE();
        }
    }

    constexpr D3D12_DESCRIPTOR_RANGE_FLAGS getDescriptorFlags(rhi::BindingMutability mut) {
        switch (mut) {
        case rhi::BindingMutability::eAlwaysStatic: return D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
        case rhi::BindingMutability::eStaticAtExecute: return D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        
        default: return D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        }
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> createInputLayout(std::span<rhi::InputElement> layout) {
        std::vector<D3D12_INPUT_ELEMENT_DESC> input{layout.size()};

        for (size_t i = 0; i < layout.size(); i++) {
            const auto &item = layout[i];
            D3D12_INPUT_ELEMENT_DESC desc { item.name.c_str(), 0, getElementFormat(item.format), 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            input[i] = desc;
        }

        return input;
    }

    constexpr D3D12_SHADER_BYTECODE createShader(std::span<std::byte> bytecode) {
        return D3D12_SHADER_BYTECODE {bytecode.data(), bytecode.size()};
    }

    constexpr D3D12_STATIC_SAMPLER_DESC createSampler(rhi::Sampler sampler, size_t index) {
        return D3D12_STATIC_SAMPLER_DESC {
            .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            .MinLOD = 0.0f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = UINT(index),
            .RegisterSpace = 0,
            .ShaderVisibility = getShaderVisibility(sampler.visibility)
        };
    }

    std::vector<D3D12_STATIC_SAMPLER_DESC> createSamplers(std::span<rhi::Sampler> samplers) {
        if (samplers.size() == 0) {
            return {};
        }

        std::vector<D3D12_STATIC_SAMPLER_DESC> it(samplers.size());

        for (size_t i = 0; i < samplers.size(); i++) {
            it[i] = createSampler(samplers[i], i);
        }

        return it;
    }

    constexpr D3D12_DESCRIPTOR_RANGE1 createRange(rhi::Binding binding) {
        return D3D12_DESCRIPTOR_RANGE1 {
            .RangeType = getRangeType(binding.type),
            .NumDescriptors = 1,
            .BaseShaderRegister = UINT(binding.base),
            .RegisterSpace = 0,
            .Flags = getDescriptorFlags(binding.mutability),
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        };
    }

    std::vector<D3D12_DESCRIPTOR_RANGE1> createBindingRange(std::span<rhi::Binding> bindings) {
        if (bindings.size() == 0) {
            return {};
        }

        std::vector<D3D12_DESCRIPTOR_RANGE1> ranges;
        for (size_t i = 0; i < bindings.size(); i++) {
            ranges.push_back(createRange(bindings[i]));
        }

        return ranges;
    }

    UniqueComPtr<ID3DBlob> serializeRootSignature(D3D_ROOT_SIGNATURE_VERSION version, std::span<rhi::Sampler> samplers, std::span<rhi::Binding> bindings) {
        // TODO: make this configurable
        constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        auto samplerData = createSamplers(samplers);

        auto ranges = createBindingRange(bindings);

        CD3DX12_ROOT_PARAMETER1 param;
        param.InitAsDescriptorTable(UINT(ranges.size()), ranges.data());

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, &param, UINT(samplerData.size()), samplerData.data(), kFlags);

        UniqueComPtr<ID3DBlob> signature;
        UniqueComPtr<ID3DBlob> error;

        HRESULT err = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error);
        ASSERTF(SUCCEEDED(err), "failed to serialize root signature: {}", std::string((char*)error->GetBufferPointer(), error->GetBufferSize()));
        return signature;
    }
}

DxDevice::DxDevice(ID3D12Device *device, D3D_ROOT_SIGNATURE_VERSION version)
    : device(device)
    , kHighestVersion(version)
{ }

DxDevice::~DxDevice() {
    ImGui_ImplDX12_Shutdown();
}

rhi::Fence *DxDevice::newFence() {
    HANDLE event = CreateEvent(nullptr, false, false, nullptr);
    ASSERT(event != nullptr);

    ID3D12Fence *fence;
    DX_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return new DxFence(fence, event);
}

rhi::CommandQueue *DxDevice::newQueue(rhi::CommandList::Type type) {
    const D3D12_COMMAND_QUEUE_DESC kDesc = {
        .Type = getCommandListType(type)
    };

    ID3D12CommandQueue *queue;
    DX_CHECK(device->CreateCommandQueue(&kDesc, IID_PPV_ARGS(&queue)));

    return new DxCommandQueue(queue);
}

rhi::CommandList *DxDevice::newCommandList(rhi::Allocator *allocator, rhi::CommandList::Type type) {
    auto *alloc = static_cast<DxAllocator*>(allocator);

    ID3D12GraphicsCommandList *commands;
    DX_CHECK(device->CreateCommandList(0, getCommandListType(type), alloc->get(), nullptr, IID_PPV_ARGS(&commands)));
    DX_CHECK(commands->Close());

    return new DxCommandList(commands);
}

rhi::Allocator *DxDevice::newAllocator(rhi::CommandList::Type type) {
    ID3D12CommandAllocator *allocator;
    DX_CHECK(device->CreateCommandAllocator(getCommandListType(type), IID_PPV_ARGS(&allocator)));

    return new DxAllocator(allocator);
}

rhi::DescriptorSet *DxDevice::newDescriptorSet(size_t count, rhi::DescriptorSet::Type type, bool shaderVisible) {
    const auto kHeapType = D3D12_DESCRIPTOR_HEAP_TYPE(type);
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

void DxDevice::createRenderTargetView(rhi::Buffer *target, rhi::CpuHandle rtvHandle) {
    auto *dxTarget = static_cast<DxBuffer*>(target);
    D3D12_CPU_DESCRIPTOR_HANDLE handle { size_t(rtvHandle) };
    device->CreateRenderTargetView(dxTarget->get(), nullptr, handle);
}

void DxDevice::createConstBufferView(rhi::Buffer *buffer, size_t size, rhi::CpuHandle srvHandle) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kHandle { size_t(srvHandle) };
    const D3D12_CONSTANT_BUFFER_VIEW_DESC kDesc {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(buffer->gpuAddress()),
        .SizeInBytes = UINT(size)
    };
    device->CreateConstantBufferView(&kDesc, kHandle);
}

void DxDevice::createDepthStencilView(rhi::Buffer *buffer, rhi::CpuHandle handle) {
    auto *stencil = static_cast<DxBuffer*>(buffer);
    const D3D12_CPU_DESCRIPTOR_HANDLE kHandle { size_t(handle) };
    device->CreateDepthStencilView(stencil->get(), &kDsvDesc, kHandle);
}

rhi::Buffer *DxDevice::newBuffer(size_t size, rhi::DescriptorSet::Visibility visibility, rhi::Buffer::State state) {
    const auto kBufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);

    ID3D12Resource *resource;
    DX_CHECK(device->CreateCommittedResource(
        getHeapProps(visibility), 
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kBufferSize,
        D3D12_RESOURCE_STATES(state), 
        nullptr, 
        IID_PPV_ARGS(&resource)
    ));
    
    return new DxBuffer(resource);
}

rhi::Buffer *DxDevice::newTexture(math::Resolution<size_t> size, rhi::DescriptorSet::Visibility visibility, rhi::Buffer::State state) {
    const D3D12_RESOURCE_DESC kTextureDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D,
        .Alignment = 0,
        .Width = UINT(size.width),
        .Height = UINT(size.height),
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    ID3D12Resource *resource;
    DX_CHECK(device->CreateCommittedResource(
        getHeapProps(visibility), 
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kTextureDesc, 
        D3D12_RESOURCE_STATES(state), 
        nullptr, 
        IID_PPV_ARGS(&resource)
    ));

    return new DxBuffer(resource);
}

rhi::PipelineState *DxDevice::newPipelineState(const rhi::PipelineBinding& bindings) {
    const auto vs = createShader(bindings.vs);
    const auto ps = createShader(bindings.ps);

    auto input = createInputLayout(bindings.input);

    auto signature = serializeRootSignature(kHighestVersion, bindings.samplers, bindings.bindings);

    ID3D12RootSignature *root = createRootSignature(signature.get());
    ID3D12PipelineState *pipeline = createPipelineState(root, vs, ps, input);
    return new DxPipelineState(root, pipeline);
}

ID3D12RootSignature *DxDevice::createRootSignature(ID3DBlob *signature) {
    ID3D12RootSignature *root;

    DX_CHECK(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root)));

    return root;
}

ID3D12PipelineState *DxDevice::createPipelineState(ID3D12RootSignature *root, D3D12_SHADER_BYTECODE vertex, D3D12_SHADER_BYTECODE pixel, std::span<D3D12_INPUT_ELEMENT_DESC> input) {
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


void DxDevice::imguiInit(size_t frames, rhi::DescriptorSet *heap, rhi::CpuHandle cpuHandle, rhi::GpuHandle gpuHandle) {
    auto *it = static_cast<DxDescriptorSet*>(heap);
    const D3D12_CPU_DESCRIPTOR_HANDLE kCpuHandle { size_t(cpuHandle) };
    const D3D12_GPU_DESCRIPTOR_HANDLE kGpuHandle { size_t(gpuHandle) };

    ImGui_ImplDX12_Init(device.get(), int(frames), DXGI_FORMAT_R8G8B8A8_UNORM, it->get(), kCpuHandle, kGpuHandle);
}

void DxDevice::imguiNewFrame() {
    ImGui_ImplDX12_NewFrame();
}
