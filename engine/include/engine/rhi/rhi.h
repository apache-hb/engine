#pragma once

#include "engine/base/window.h"
#include "engine/base/util.h"

#include <span>
#include <string_view>

#include "dx/d3dx12.h"
#include <dxgi1_6.h>

namespace engine::rhi {
    template<typename T>
    concept IsComObject = std::is_base_of_v<IUnknown, T>;
    
    template<IsComObject T>
    struct ComDeleter {
        void operator()(T *ptr) {
            ptr->Release();
        }
    };

    template<IsComObject T>
    using UniqueComPtr = UniquePtr<T, ComDeleter<T>>;

    struct HandleDeleter {
        void operator()(HANDLE handle) {
            CloseHandle(handle);
        }
    };

    using UniqueHandle = UniqueResource<HANDLE, HandleDeleter>;

    enum struct CpuHandle : std::size_t {};
    enum struct GpuHandle : std::size_t {};

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
        std::string name;
        Format format;
    };

    enum ShaderVisibility {
        ePixel = D3D12_SHADER_VISIBILITY_PIXEL,
        eVertex = D3D12_SHADER_VISIBILITY_VERTEX,
        eTotal = D3D12_SHADER_VISIBILITY_ALL
    };

    enum struct BindingMutability {
        eAlwaysStatic = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
        eStaticAtExecute = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
    };
    
    struct Sampler {
        ShaderVisibility visibility;
    };

    struct BindingRange {
        size_t base;
        Object::Type type;
        BindingMutability mutability;
    };

    struct BindingTable {
        ShaderVisibility visibility;
        std::span<BindingRange> ranges;
    };

    struct PipelineBinding {
        std::span<Sampler> samplers;
        std::span<BindingTable> tables;

        std::span<InputElement> input;

        std::span<std::byte> ps;
        std::span<std::byte> vs;
    };

    struct Allocator final : UniqueComPtr<ID3D12CommandAllocator> {
        using Super = UniqueComPtr<ID3D12CommandAllocator>;
        using Super::Super;

        void reset();
    };

    struct Buffer final : UniqueComPtr<ID3D12Resource> {
        using Super = UniqueComPtr<ID3D12Resource>;
        using Super::Super;

        enum State {
            eUpload = D3D12_RESOURCE_STATE_GENERIC_READ,
            eCopyDst = D3D12_RESOURCE_STATE_COPY_DEST,

            eVertex = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            eConstBuffer = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            ePixelShaderResource = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,

            eIndex = D3D12_RESOURCE_STATE_INDEX_BUFFER,

            eRenderTarget = D3D12_RESOURCE_STATE_RENDER_TARGET,
            ePresent = D3D12_RESOURCE_STATE_PRESENT
        };

        void write(const void *src, size_t size);
        
        void *map();
        void unmap();

        GpuHandle gpuAddress();
    };

    struct IndexBufferView {
        GpuHandle buffer;
        size_t size;
        Format format;
    };

    struct VertexBufferView {
        GpuHandle buffer;
        size_t size;
        size_t stride;
    };

    struct Fence final : UniqueComPtr<ID3D12Fence> {
        using Super = UniqueComPtr<ID3D12Fence>;
        using Super::Super;

        Fence(ID3D12Fence *fence = nullptr);

        void waitUntil(size_t signal);

    private:
        UniqueHandle event;
    };
    
    struct DescriptorSet final : UniqueComPtr<ID3D12DescriptorHeap> {
        using Super = UniqueComPtr<ID3D12DescriptorHeap>;
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

        UniqueComPtr<ID3D12RootSignature> signature;
        UniqueComPtr<ID3D12PipelineState> pipeline;
    };

    struct CommandList final : UniqueComPtr<ID3D12GraphicsCommandList> {
        using Super = UniqueComPtr<ID3D12GraphicsCommandList>;
        using Super::Super;

        enum struct Type {
            eDirect = D3D12_COMMAND_LIST_TYPE_DIRECT,
            eCopy = D3D12_COMMAND_LIST_TYPE_COPY
        };

        void beginRecording(Allocator &allocator);
        void endRecording();

        void setRenderTarget(CpuHandle target, const math::float4 &colour);
        void setViewAndScissor(const View &view);
        void setPipeline(PipelineState &pipeline);

        // TODO: this is a bit wonky
        void bindDescriptors(std::span<ID3D12DescriptorHeap*> sets);
        void bindTable(size_t index, GpuHandle handle);

        void copyBuffer(Buffer &dst, Buffer &src, size_t size);
        void copyTexture(Buffer &dst, Buffer &src, const void *ptr, TextureSize size);

        void drawMesh(const IndexBufferView &indexView, const VertexBufferView &vertexView);

        void transition(Buffer &buffer, Buffer::State before, Buffer::State after);

        void imguiRender();

    private:
        void transitionBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    };
    
    struct SwapChain final : UniqueComPtr<IDXGISwapChain3> {
        using Super = UniqueComPtr<IDXGISwapChain3>;
        using Super::Super;

        SwapChain(IDXGISwapChain3 *swapchain = nullptr, bool tearing = false);

        void present();
        Buffer getBuffer(size_t index);

        size_t currentBackBuffer();

    private:
        UINT flags;
    };

    struct CommandQueue final : UniqueComPtr<ID3D12CommandQueue> {
        using Super = UniqueComPtr<ID3D12CommandQueue>;
        using Super::Super;

        SwapChain newSwapChain(Window *window, size_t buffers);
        void signal(Fence &fence, size_t value);
        void execute(std::span<ID3D12CommandList*> lists);
    };

    struct TextureCreate {
        Buffer buffer;
        size_t uploadSize;
    };

    struct Device final : UniqueComPtr<ID3D12Device> {
        using Super = UniqueComPtr<ID3D12Device>;
        using Super::Super;

        Device(ID3D12Device *device);
        ~Device();

        Fence newFence();
        CommandQueue newQueue(CommandList::Type type);
        CommandList newCommandList(Allocator &allocator, CommandList::Type type);
        Allocator newAllocator(CommandList::Type type);

        DescriptorSet newDescriptorSet(size_t count, DescriptorSet::Type type, bool shaderVisible);

        void createRenderTargetView(Buffer &buffer, CpuHandle handle);
        void createConstBufferView(Buffer &buffer, size_t size, CpuHandle handle);
        void createTextureBufferView(Buffer &buffer, CpuHandle handle);

        Buffer newBuffer(size_t size, DescriptorSet::Visibility visibility, Buffer::State state);
        TextureCreate newTexture(TextureSize size, DescriptorSet::Visibility visibility, Buffer::State state, math::float4 clear = math::float4::of(0.f));

        PipelineState newPipelineState(const PipelineBinding& bindings);

        void imguiInit(size_t frames, DescriptorSet &heap, CpuHandle cpuHandle, GpuHandle gpuHandle);
        void imguiNewFrame();

    private:
        ID3D12RootSignature *createRootSignature(ID3DBlob *signature);

        ID3D12PipelineState *createPipelineState(ID3D12RootSignature *root, D3D12_SHADER_BYTECODE vertex, D3D12_SHADER_BYTECODE pixel, std::span<D3D12_INPUT_ELEMENT_DESC> input);

        D3D_ROOT_SIGNATURE_VERSION kHighestVersion;
    };

    Device getDevice();
}
