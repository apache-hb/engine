#pragma once

#include "engine/base/window.h"
#include "engine/base/util.h"

#include <span>
#include <string_view>

#include "dx/d3dx12.h"

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

    using Viewport = math::Resolution<float>;

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

    enum struct ShaderVisibility {
        ePixel = D3D12_SHADER_VISIBILITY_PIXEL,
        eVertex = D3D12_SHADER_VISIBILITY_VERTEX
    };

    enum struct BindingMutability {
        eAlwaysStatic = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
        eStaticAtExecute = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
    };
    
    struct Sampler {
        ShaderVisibility visibility;
    };

    struct Binding {
        size_t base;
        Object::Type type;
        BindingMutability mutability;
    };

    struct PipelineBinding {
        std::span<Sampler> samplers;
        std::span<Binding> bindings;

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

    struct PipelineState {
        virtual ~PipelineState() = default;
    };

    struct CommandList {
        enum struct Type {
            eDirect = D3D12_COMMAND_LIST_TYPE_DIRECT,
            eCopy = D3D12_COMMAND_LIST_TYPE_COPY
        };

        virtual void beginRecording(Allocator &allocator) = 0;
        virtual void endRecording() = 0;

        virtual void setRenderTarget(CpuHandle target, const math::float4 &colour) = 0;
        virtual void setViewport(const Viewport &view) = 0;
        virtual void setPipeline(PipelineState *pipeline) = 0;

        // TODO: this is a bit wonky
        virtual void bindDescriptors(std::span<ID3D12DescriptorHeap*> sets) = 0;
        virtual void bindTable(size_t index, GpuHandle handle) = 0;

        virtual void copyBuffer(Buffer &dst, Buffer &src, size_t size) = 0;

        virtual void drawMesh(const IndexBufferView &indexView, const VertexBufferView &vertexView) = 0;

        virtual void transition(Buffer &buffer, Buffer::State before, Buffer::State after) = 0;

        virtual void imguiRender() = 0;

        virtual ~CommandList() = default;
    };
    
    struct SwapChain {
        virtual void present() = 0;
        virtual Buffer getBuffer(size_t index) = 0;

        virtual size_t currentBackBuffer() = 0;

        virtual ~SwapChain() = default;
    };

    struct CommandQueue {
        virtual SwapChain *newSwapChain(Window *window, size_t buffers) = 0;
        virtual void signal(Fence &fence, size_t value) = 0;
        virtual void execute(std::span<CommandList*> lists) = 0;

        virtual ~CommandQueue() = default;
    };

    struct Device {
        virtual Fence newFence() = 0;
        virtual CommandQueue *newQueue(CommandList::Type type) = 0;
        virtual CommandList *newCommandList(Allocator &allocator, CommandList::Type type) = 0;
        virtual Allocator newAllocator(CommandList::Type type) = 0;

        virtual DescriptorSet newDescriptorSet(size_t count, DescriptorSet::Type type, bool shaderVisible) = 0;

        virtual void createRenderTargetView(Buffer &buffer, CpuHandle handle) = 0;
        virtual void createConstBufferView(Buffer &buffer, size_t size, CpuHandle handle) = 0;

        virtual Buffer newBuffer(size_t size, DescriptorSet::Visibility visibility, Buffer::State state) = 0;
        virtual Buffer newTexture(math::Resolution<size_t> size, DescriptorSet::Visibility visibility, Buffer::State state) = 0;

        virtual PipelineState *newPipelineState(const PipelineBinding& bindings) = 0;

        virtual void imguiInit(size_t frames, DescriptorSet &heap, CpuHandle cpuHandle, GpuHandle gpuHandle) = 0;
        virtual void imguiNewFrame() = 0;

        virtual ~Device() = default;
    };

    Device *getDevice();
}
