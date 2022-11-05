#pragma once

#include "dx/d3d12.h"
#include "engine/window/window.h"
#include "engine/base/util.h"
#include "engine/base/logging.h"
#include "engine/base/container/com/com.h"

#include <span>
#include <string_view>

#include "dx/d3dx12.h"
#include <dxgi1_6.h>

namespace simcoe::rhi {
    template<typename T>
    struct UniqueObject : com::UniqueComPtr<T> {
        using Super = com::UniqueComPtr<T>;
        using Super::Super;

        UniqueObject& rename(std::string_view name) {
            Super::get()->SetName(strings::widen(name).c_str());
            return *this;
        }
    };
    
    struct HandleDeleter {
        void operator()(HANDLE handle) const {
            CloseHandle(handle);
        }
    };

    using UniqueHandle = UniqueResource<HANDLE, INVALID_HANDLE_VALUE, HandleDeleter>;

    enum struct CpuHandle : std::size_t { Invalid = SIZE_MAX };
    enum struct GpuHandle : std::size_t { Invalid = SIZE_MAX };

    static_assert(sizeof(CpuHandle) == sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
    static_assert(sizeof(GpuHandle) == sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));

    using TextureSize = math::Resolution<size_t>;

    struct Scissor : D3D12_RECT {
        using Super = D3D12_RECT;

        Scissor(LONG top, LONG left, LONG width, LONG height)
            : Super({ top, left, width, height })
        { }
    };

    struct Viewport : D3D12_VIEWPORT {
        using Super = D3D12_VIEWPORT;
        Viewport(FLOAT top, FLOAT left, FLOAT width, FLOAT height)
            : Super({ top, left, width, height, 0.f, 1.f })
        { }
    };

    struct View {
        View() = default;
        View(FLOAT top, FLOAT left, FLOAT width, FLOAT height)
            : scissor(LONG(top), LONG(left), LONG(width + left), LONG(height + top))
            , viewport(top, left, width, height)
        { }

        Scissor scissor{0, 0, 0, 0};
        Viewport viewport{0.f, 0.f, 0.f, 0.f};
    };

    enum struct Format {
        uint32,

        float32,
        float32x2,
        float32x3,
        float32x4
    };

    namespace Object {
        enum Type : unsigned {
            eTexture = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            eConstBuffer = D3D12_DESCRIPTOR_RANGE_TYPE_CBV
        };
    }
    
    struct InputElement {
        const char *name;
        Format format;
        size_t offset = SIZE_MAX;
    };

    BITFLAGS(ShaderVisibility, D3D12_SHADER_VISIBILITY, {
        ePixel = D3D12_SHADER_VISIBILITY_PIXEL,
        eVertex = D3D12_SHADER_VISIBILITY_VERTEX,
        eAll = D3D12_SHADER_VISIBILITY_ALL
    })

    enum struct BindingMutability {
        eAlwaysStatic = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
        eBindless = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
        eStaticAtExecute = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
    };
    
    struct Sampler {
        ShaderVisibility visibility;
    };

    struct BindingRange {
        size_t base;
        Object::Type type;
        BindingMutability mutability;
        size_t count = 1;
        size_t space = 0;
    };

    using BindingSpan = std::span<const BindingRange>;

    struct BindingConst {
        size_t base;
        size_t count;
    };

    struct Binding {
        enum Kind { eTable, eConst } kind;
        ShaderVisibility visibility;
        union {
            BindingSpan ranges;
            BindingConst root;
        };
    };

    constexpr Binding bindTable(ShaderVisibility visibility, BindingSpan spans) {
        return {
            .kind = Binding::eTable,
            .visibility = visibility,
            .ranges = spans
        };
    };

    constexpr Binding bindConst(ShaderVisibility visibility, size_t base, size_t count) {
        return {
            .kind = Binding::eConst,
            .visibility = visibility,
            .root = { base, count }
        };
    }

    struct PipelineBinding {
        std::span<const Sampler> samplers;
        std::span<const Binding> bindings;

        std::span<const InputElement> input;

        std::span<std::byte> ps;
        std::span<std::byte> vs;

        bool depth = false;
    };

    struct Allocator final : UniqueObject<ID3D12CommandAllocator> {
        using Super = UniqueObject<ID3D12CommandAllocator>;
        using Super::Super;

        void reset();
    };

    struct Buffer final : UniqueObject<ID3D12Resource> {
        using Super = UniqueObject<ID3D12Resource>;
        using Super::Super;

        enum State {
            eUpload = D3D12_RESOURCE_STATE_GENERIC_READ,
            eCopyDst = D3D12_RESOURCE_STATE_COPY_DEST,

            eVertex = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            eConstBuffer = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            ePixelShaderResource = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,

            eIndex = D3D12_RESOURCE_STATE_INDEX_BUFFER,

            eRenderTarget = D3D12_RESOURCE_STATE_RENDER_TARGET,
            ePresent = D3D12_RESOURCE_STATE_PRESENT,

            eInvalid = UINT_MAX
        };

        void write(const void *src, size_t size);
        
        void *map();
        void unmap();

        GpuHandle gpuAddress();
    };

    struct IndexBufferView {
        GpuHandle buffer;
        size_t length;
        Format format;
    };

    using VertexBufferView = D3D12_VERTEX_BUFFER_VIEW;
    using StateTransition = D3D12_RESOURCE_BARRIER;

    constexpr VertexBufferView newVertexBufferView(GpuHandle buffer, size_t length, size_t stride) {
        return {
            .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(buffer),
            .SizeInBytes = UINT(length * stride),
            .StrideInBytes = UINT(stride)
        };
    }

    inline StateTransition newStateTransition(Buffer& buffer, Buffer::State before, Buffer::State after) {
        return {
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Transition = {
                .pResource = buffer.get(),
                .StateBefore = D3D12_RESOURCE_STATES(before),
                .StateAfter = D3D12_RESOURCE_STATES(after),
            }
        };
    }

    struct Fence final : UniqueObject<ID3D12Fence> {
        using Super = UniqueObject<ID3D12Fence>;
        using Super::Super;

        Fence(ID3D12Fence *fence = nullptr);

        void wait(size_t signal);

    private:
        UniqueHandle event;
    };
    
    struct DescriptorSet final : UniqueObject<ID3D12DescriptorHeap> {
        using Super = UniqueObject<ID3D12DescriptorHeap>;
        using Super::Super;

        enum struct Type {
            eConstBuffer = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            eRenderTarget = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            eDepthStencil = D3D12_DESCRIPTOR_HEAP_TYPE_DSV
        };

        enum struct Visibility {
            eHostVisible,
            eDeviceOnly
        };

        DescriptorSet(ID3D12DescriptorHeap *heap = nullptr, UINT stride = UINT_MAX);

        CpuHandle cpuHandle(size_t offset);
        GpuHandle gpuHandle(size_t offset);

    private:
        UINT stride;
    };

    struct PipelineState final {
        PipelineState(ID3D12RootSignature *signature = nullptr, ID3D12PipelineState *pipeline = nullptr);

        void rename(std::string_view name) {
            signature.rename(std::format("{}.signature", name));
            pipeline.rename(std::format("{}.pipeline", name));
        }

        UniqueObject<ID3D12RootSignature> signature;
        UniqueObject<ID3D12PipelineState> pipeline;
    };

    struct CommandList final : UniqueObject<ID3D12GraphicsCommandList> {
        using Super = UniqueObject<ID3D12GraphicsCommandList>;
        using Super::Super;

        enum struct Type {
            eDirect = D3D12_COMMAND_LIST_TYPE_DIRECT,
            eCopy = D3D12_COMMAND_LIST_TYPE_COPY
        };

        void beginRecording(Allocator &allocator);
        void endRecording();

        void setRenderTarget(CpuHandle rtv, CpuHandle dsv, const math::float4 &colour);
        void clearRenderTarget(CpuHandle rtv, const math::float4 &colour);

        void setViewAndScissor(const View &view);
        void setPipeline(PipelineState &pipeline);

        // TODO: this is a bit wonky
        void bindDescriptors(std::span<ID3D12DescriptorHeap*> sets);
        void bindTable(size_t index, GpuHandle handle);
        void bindConst(size_t index, size_t offset, uint32_t value);

        void copyBuffer(Buffer &dst, Buffer &src, size_t size);
        void copyTexture(Buffer &dst, Buffer &src, const void *ptr, TextureSize size);

        void setVertexBuffers(std::span<const VertexBufferView> buffers);

        void drawMesh(const IndexBufferView &indexView);

        void transition(std::span<const StateTransition> barriers);

        void imguiFrame();
    };
    
    struct SwapChain final : UniqueObject<IDXGISwapChain3> {
        using Super = UniqueObject<IDXGISwapChain3>;
        using Super::Super;

        SwapChain(IDXGISwapChain3 *swapchain = nullptr, bool tearing = false);

        void present();
        Buffer getBuffer(size_t index);
        void resize(TextureSize resolution, size_t frames);

        size_t currentBackBuffer();

    private:
        UINT flags;
        UINT presentFlags;
    };

    struct CommandQueue final : UniqueObject<ID3D12CommandQueue> {
        using Super = UniqueObject<ID3D12CommandQueue>;
        using Super::Super;

        SwapChain newSwapChain(Window *window, size_t buffers);

        void signal(Fence &fence, size_t value);
        void execute(std::span<ID3D12CommandList*> lists);
    };

    struct TextureCreate {
        Buffer buffer;
        size_t uploadSize;
    };

    struct TextureDesc {
        D3D12_RESOURCE_DESC resourceDesc;
        D3D12_RESOURCE_STATES state;
    };

    constexpr TextureDesc newTextureDesc(const TextureSize& size, Buffer::State state) {
        ASSERT(size.width > 0 && size.height > 0);

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

        return { kTextureDesc, kState };
    }

    struct Device final : UniqueObject<ID3D12Device> {
        using Super = UniqueObject<ID3D12Device>;
        using Super::Super;
        
        Fence newFence();
        CommandQueue newQueue(CommandList::Type type);
        CommandList newCommandList(Allocator &allocator, CommandList::Type type);
        Allocator newAllocator(CommandList::Type type);

        DescriptorSet newDescriptorSet(size_t count, DescriptorSet::Type type, bool shaderVisible);

        void createRenderTargetView(Buffer &buffer, CpuHandle handle);
        void createDepthStencilView(Buffer &target, CpuHandle dsvHandle);
        void createConstBufferView(Buffer &buffer, size_t size, CpuHandle handle);
        void createTextureBufferView(Buffer &buffer, CpuHandle handle);

        Buffer newBuffer(size_t size, DescriptorSet::Visibility visibility, Buffer::State state);
        Buffer newDepthStencil(TextureSize size, CpuHandle handle);

        rhi::Buffer newTexture(const TextureDesc& desc, DescriptorSet::Visibility visibility, math::float4 clear = math::float4::of(0.f));
        TextureCreate newTexture(TextureSize size, DescriptorSet::Visibility visibility, Buffer::State state, math::float4 clear = math::float4::of(0.f));

        PipelineState newPipelineState(const PipelineBinding& bindings);

        void imguiInit(size_t frames, DescriptorSet &heap, size_t offset);
        void imguiShutdown();
        void imguiFrame();

    private:
        ID3D12RootSignature *createRootSignature(ID3DBlob *signature);

        ID3D12PipelineState *createPipelineState(ID3D12RootSignature *root, D3D12_SHADER_BYTECODE vertex, D3D12_SHADER_BYTECODE pixel, std::span<D3D12_INPUT_ELEMENT_DESC> input, bool depth);
    };

    Device getDevice();
}
