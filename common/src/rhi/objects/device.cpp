#include "engine/rhi/rhi.h"
#include "objects/common.h"

#include "imgui_impl_dx12.h"

using namespace simcoe;
using namespace simcoe::rhi;

namespace {
    constexpr auto kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    constexpr auto kDepthFormat = DXGI_FORMAT_D32_FLOAT;

    constexpr const D3D12_HEAP_PROPERTIES *getHeapProps(rhi::DescriptorSet::Visibility visibility) {
        switch (visibility) {
        case rhi::DescriptorSet::Visibility::eDeviceOnly: return &kDefaultProps;
        case rhi::DescriptorSet::Visibility::eHostVisible: return &kUploadProps;
        default: return nullptr;
        }
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> createInputLayout(std::span<const rhi::InputElement> layout) {
        std::vector<D3D12_INPUT_ELEMENT_DESC> input{layout.size()};

        for (size_t i = 0; i < layout.size(); i++) {
            const auto &item = layout[i];
            auto align = item.offset == SIZE_MAX ? D3D12_APPEND_ALIGNED_ELEMENT : UINT(item.offset);
            D3D12_INPUT_ELEMENT_DESC desc { 
                .SemanticName = item.name, 
                .SemanticIndex = 0, 
                .Format = getElementFormat(item.format), 
                .InputSlot = 0, 
                .AlignedByteOffset = align, 
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 
                .InstanceDataStepRate = 0 
            };
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
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            .MinLOD = 0.0f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = UINT(index),
            .RegisterSpace = 0,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY(sampler.visibility)
        };
    }

    std::vector<D3D12_STATIC_SAMPLER_DESC> createSamplers(std::span<const rhi::Sampler> samplers) {
        if (samplers.size() == 0) {
            return {};
        }

        std::vector<D3D12_STATIC_SAMPLER_DESC> it(samplers.size());

        for (size_t i = 0; i < samplers.size(); i++) {
            it[i] = createSampler(samplers[i], i);
        }

        return it;
    }

    constexpr D3D12_DESCRIPTOR_RANGE1 createRange(rhi::BindingRange binding) {
        return D3D12_DESCRIPTOR_RANGE1 {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE(binding.type),
            .NumDescriptors = (binding.count == SIZE_MAX) ? UINT_MAX : UINT(binding.count),
            .BaseShaderRegister = UINT(binding.base),
            .RegisterSpace = UINT(binding.space),
            .Flags = D3D12_DESCRIPTOR_RANGE_FLAGS(binding.mutability),
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        };
    }

    struct BindingBuilder {
        using DxRanges = UniquePtr<D3D12_DESCRIPTOR_RANGE1[]>;
        using DxParams = UniquePtr<D3D12_ROOT_PARAMETER1[]>;

        D3D12_DESCRIPTOR_RANGE1 *createBindingRange(std::span<const rhi::BindingRange> bindings) {
            DxRanges ranges { bindings.size() };
            for (size_t i = 0; i < bindings.size(); i++) {
                ranges[i] = createRange(bindings[i]);
            }

            return descriptors.emplace_back(std::move(ranges)).get();
        }

        D3D12_ROOT_PARAMETER1 createRootBinding(const rhi::Binding& binding) {
            const auto kVisibility = D3D12_SHADER_VISIBILITY(binding.visibility);
            CD3DX12_ROOT_PARAMETER1 param;
            switch (binding.kind) {
            case Binding::eTable: {
                auto ranges = createBindingRange(binding.ranges);
                param.InitAsDescriptorTable(UINT(binding.ranges.size()), ranges, kVisibility);
                break;
            }
            case Binding::eConst:
                param.InitAsConstants(UINT(binding.root.count), UINT(binding.root.base), 0, kVisibility);
                break;
            
            default:
                ASSERT(false);
                break;
            }

            return param;
        }

        DxParams createBindingParams(std::span<const Binding> bindings) {
            DxParams params { bindings.size() };
            for (size_t i = 0; i < bindings.size(); i++) {
                params[i] = createRootBinding(bindings[i]);
            }

            return params;
        }

    private:
        std::vector<DxRanges> descriptors;
    };

    rhi::UniqueObject<ID3DBlob> serializeRootSignature(D3D_ROOT_SIGNATURE_VERSION version, std::span<const rhi::Sampler> samplers, std::span<const rhi::Binding> bindings) {
        // TODO: make this configurable
        constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        auto samplerData = createSamplers(samplers);

        BindingBuilder builder;

        auto params = builder.createBindingParams(bindings);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(UINT(bindings.size()), params.get(), UINT(samplerData.size()), samplerData.data(), kFlags);

        rhi::UniqueObject<ID3DBlob> signature;
        rhi::UniqueObject<ID3DBlob> error;

        HRESULT err = D3DX12SerializeVersionedRootSignature(&desc, version, &signature, &error);
        ASSERTF(SUCCEEDED(err), "failed to serialize root signature: {}", std::string((char*)error->GetBufferPointer(), error->GetBufferSize()));
        return signature;
    }

    D3D_ROOT_SIGNATURE_VERSION getHighestVersion(ID3D12Device *device) {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }
        return featureData.HighestVersion;
    }
}

Device::Device(ID3D12Device *device)
    : Super(device)
    , kHighestVersion(getHighestVersion(device))
{ }

Device::~Device() {
    ImGui_ImplDX12_Shutdown();
}

rhi::Fence Device::newFence() {
    ID3D12Fence *fence;
    DX_CHECK(get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return rhi::Fence(fence);
}

rhi::CommandQueue Device::newQueue(rhi::CommandList::Type type) {
    const D3D12_COMMAND_QUEUE_DESC kDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE(type)
    };

    ID3D12CommandQueue *queue;
    DX_CHECK(get()->CreateCommandQueue(&kDesc, IID_PPV_ARGS(&queue)));

    return rhi::CommandQueue(queue);
}

rhi::CommandList Device::newCommandList(rhi::Allocator &allocator, rhi::CommandList::Type type) {
    ID3D12GraphicsCommandList *commands;
    DX_CHECK(get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE(type), allocator.get(), nullptr, IID_PPV_ARGS(&commands)));
    DX_CHECK(commands->Close());

    return rhi::CommandList(commands);
}

rhi::Allocator Device::newAllocator(rhi::CommandList::Type type) {
    ID3D12CommandAllocator *allocator;
    DX_CHECK(get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE(type), IID_PPV_ARGS(&allocator)));

    return rhi::Allocator(allocator);
}

rhi::DescriptorSet Device::newDescriptorSet(size_t count, rhi::DescriptorSet::Type type, bool shaderVisible) {
    const auto kHeapType = D3D12_DESCRIPTOR_HEAP_TYPE(type);
    const D3D12_DESCRIPTOR_HEAP_DESC kRtvHeapDesc {
        .Type = kHeapType,
        .NumDescriptors = UINT(count),
        .Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    ID3D12DescriptorHeap *rtvHeap;
    DX_CHECK(get()->CreateDescriptorHeap(&kRtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
    UINT stride = get()->GetDescriptorHandleIncrementSize(kHeapType);

    return rhi::DescriptorSet(rtvHeap, stride);
}

void Device::createRenderTargetView(rhi::Buffer &target, rhi::CpuHandle rtvHandle) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kHandle { size_t(rtvHandle) };
    get()->CreateRenderTargetView(target.get(), nullptr, kHandle);
}

void Device::createDepthStencilView(rhi::Buffer &target, rhi::CpuHandle dsvHandle) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kHandle { size_t(dsvHandle) };
    get()->CreateDepthStencilView(target.get(), nullptr, kHandle);
}

void Device::createConstBufferView(rhi::Buffer &buffer, size_t size, rhi::CpuHandle srvHandle) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kHandle { size_t(srvHandle) };
    const D3D12_CONSTANT_BUFFER_VIEW_DESC kDesc {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(buffer.gpuAddress()),
        .SizeInBytes = UINT(size)
    };
    get()->CreateConstantBufferView(&kDesc, kHandle);
}

void Device::createTextureBufferView(Buffer &buffer, CpuHandle handle) {
    const D3D12_SHADER_RESOURCE_VIEW_DESC kDesc {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MipLevels = 1
        }
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE kHandle { size_t(handle) };
    get()->CreateShaderResourceView(buffer.get(), &kDesc, kHandle);
}

rhi::Buffer Device::newBuffer(size_t size, rhi::DescriptorSet::Visibility visibility, rhi::Buffer::State state) {
    ASSERT(size > 0);

    const auto kBufferSize = CD3DX12_RESOURCE_DESC::Buffer(size);

    ID3D12Resource *resource;
    DX_CHECK(get()->CreateCommittedResource(
        getHeapProps(visibility),
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kBufferSize,
        D3D12_RESOURCE_STATES(state),
        nullptr,
        IID_PPV_ARGS(&resource)
    ));

    return rhi::Buffer(resource);
}

rhi::TextureCreate Device::newTexture(math::Resolution<size_t> size, rhi::DescriptorSet::Visibility visibility, rhi::Buffer::State state, math::float4 clear) {
    ASSERT(size.width > 0 && size.height > 0);

    const D3D12_CLEAR_VALUE kClear { 
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .Color = { clear.x, clear.y, clear.z, clear.w }
    };
    const auto kState = D3D12_RESOURCE_STATES(state);
    const D3D12_RESOURCE_DESC kTextureDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
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
        .Flags = (kState & D3D12_RESOURCE_STATE_RENDER_TARGET) ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE
    };

    ID3D12Resource *resource;
    DX_CHECK(get()->CreateCommittedResource(
        getHeapProps(visibility),
        D3D12_HEAP_FLAG_NONE,
        &kTextureDesc,
        kState,
        (kState & D3D12_RESOURCE_STATE_RENDER_TARGET) ? &kClear : nullptr,
        IID_PPV_ARGS(&resource)
    ));

    // get the size needed for the destination buffer
    UINT64 requiredSize = 0;
    get()->GetCopyableFootprints(&kTextureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &requiredSize);

    return { rhi::Buffer(resource), requiredSize };
}

rhi::Buffer Device::newDepthStencil(TextureSize size, CpuHandle handle) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kHandle { size_t(handle) };

    const D3D12_DEPTH_STENCIL_VIEW_DESC kDesc {
        .Format = kDepthFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE
    };

    const D3D12_CLEAR_VALUE kClear {
        .Format = kDepthFormat,
        .DepthStencil = {
            .Depth = 1.f,
            .Stencil = 0
        }
    };

    const CD3DX12_RESOURCE_DESC kDepthTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ kDepthFormat,
        /* width = */ UINT(size.width),
        /* height = */ UINT(size.height),
        /* arraySize = */ 1,
        /* mipLevels = */ 0,
        /* sampleCount = */ 1,
        /* sampleQuality = */ 0,
        /* flags = */ D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    ID3D12Resource *resource;

    DX_CHECK(get()->CreateCommittedResource(
        &kDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &kDepthTextureDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &kClear,
        IID_PPV_ARGS(&resource)
    ));

    get()->CreateDepthStencilView(resource, &kDesc, kHandle);

    return resource;
}

rhi::PipelineState Device::newPipelineState(const rhi::PipelineBinding& bindings) {
    const auto vs = createShader(bindings.vs);
    const auto ps = createShader(bindings.ps);

    auto input = createInputLayout(bindings.input);

    auto signature = serializeRootSignature(kHighestVersion, bindings.samplers, bindings.bindings);

    ID3D12RootSignature *root = createRootSignature(signature.get());
    ID3D12PipelineState *pipeline = createPipelineState(root, vs, ps, input, bindings.depth);
    return rhi::PipelineState(root, pipeline);
}

ID3D12RootSignature *Device::createRootSignature(ID3DBlob *signature) {
    ID3D12RootSignature *root;

    DX_CHECK(get()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root)));

    return root;
}

ID3D12PipelineState *Device::createPipelineState(ID3D12RootSignature *root, D3D12_SHADER_BYTECODE vertex, D3D12_SHADER_BYTECODE pixel, std::span<D3D12_INPUT_ELEMENT_DESC> input, bool depth) {
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
        .pRootSignature = root,
        .VS = vertex,
        .PS = pixel,
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = depth ? CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT) : CD3DX12_DEPTH_STENCIL_DESC(),
        .InputLayout = {
            .pInputElementDescs = input.data(),
            .NumElements = UINT(input.size())
        },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat = depth ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1
        }
    };

    ID3D12PipelineState *pipeline;
    DX_CHECK(get()->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&pipeline)));
    return pipeline;
}


void Device::imguiInit(size_t frames, rhi::DescriptorSet &heap, size_t offset) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kCpuHandle { size_t(heap.cpuHandle(offset)) };
    const D3D12_GPU_DESCRIPTOR_HANDLE kGpuHandle { size_t(heap.gpuHandle(offset)) };

    ImGui_ImplDX12_Init(get(), int(frames), DXGI_FORMAT_R8G8B8A8_UNORM, heap.get(), kCpuHandle, kGpuHandle);
}

void Device::imgui() {
    ImGui_ImplDX12_NewFrame();
}
