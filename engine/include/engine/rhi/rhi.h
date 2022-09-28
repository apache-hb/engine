#pragma once

#include "engine/base/window.h"

#include <span>
#include <string_view>

#include "dx/d3d12.h"

namespace engine::rhi {
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
            eRenderTarget = (1 << 0),
            eTexture = (1 << 1),
            eConstBuffer = (1 << 2),
            eDepthStencil = (1 << 3)
        };
    }

    struct Allocator {
        virtual ~Allocator() = default;
    };
    
    struct InputElement {
        std::string name;
        Format format;
    };

    enum struct ShaderVisibility {
        ePixel,
        eVertex
    };

    enum struct BindingMutability {
        eAlwaysStatic,
        eStaticAtExecute,
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

    struct Buffer {
        enum struct State {
            eUpload,
            eCopyDst,

            eVertex,
            eIndex,
            eConstBuffer,

            eRenderTarget,
            ePresent
        };

        virtual void write(const void *src, size_t size) = 0;
        
        virtual void *map() = 0;
        virtual void unmap() = 0;

        virtual GpuHandle gpuAddress() = 0;

        virtual ~Buffer() = default;
    };

    struct IndexBufferView {
        Buffer *buffer;
        size_t size;
        Format format;
    };

    struct VertexBufferView {
        Buffer *buffer;
        size_t size;
        size_t stride;
    };

    struct Fence {
        virtual void waitUntil(size_t signal) = 0;

        virtual ~Fence() = default;
    };
    
    struct DescriptorSet {
        enum struct Type {
            eConstBuffer = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            eRenderTarget = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            eDepthStencil = D3D12_DESCRIPTOR_HEAP_TYPE_DSV
        };

        enum struct Visibility {
            eHostVisible,
            eDeviceOnly
        };

        virtual CpuHandle cpuHandle(size_t offset) = 0;
        virtual GpuHandle gpuHandle(size_t offset) = 0;

        virtual ~DescriptorSet() = default;
    };

    struct PipelineState {
        virtual ~PipelineState() = default;
    };

    struct CommandList {
        enum Type {
            eDirect,
            eCopy
        };

        virtual void beginRecording(Allocator *allocator) = 0;
        virtual void endRecording() = 0;

        virtual void setRenderTarget(CpuHandle target, const math::float4 &colour) = 0;
        virtual void setViewport(const Viewport &view) = 0;
        virtual void setPipeline(PipelineState *pipeline) = 0;

        virtual void bindDescriptors(std::span<DescriptorSet*> sets) = 0;
        virtual void bindTable(size_t index, GpuHandle handle) = 0;

        virtual void copyBuffer(Buffer *dst, Buffer *src, size_t size) = 0;

        virtual void drawMesh(const IndexBufferView &indexView, const VertexBufferView &vertexView) = 0;

        virtual void transition(Buffer *buffer, Buffer::State before, Buffer::State after) = 0;

        virtual void imguiRender() = 0;

        virtual ~CommandList() = default;
    };
    
    struct SwapChain {
        virtual void present() = 0;
        virtual Buffer *getBuffer(size_t index) = 0;

        virtual size_t currentBackBuffer() = 0;

        virtual ~SwapChain() = default;
    };

    struct CommandQueue {
        virtual SwapChain *newSwapChain(Window *window, size_t buffers) = 0;
        virtual void signal(Fence *fence, size_t value) = 0;
        virtual void execute(std::span<CommandList*> lists) = 0;

        virtual ~CommandQueue() = default;
    };

    struct Device {
        virtual Fence *newFence() = 0;
        virtual CommandQueue *newQueue(CommandList::Type type) = 0;
        virtual CommandList *newCommandList(rhi::Allocator *allocator, CommandList::Type type) = 0;
        virtual Allocator *newAllocator(rhi::CommandList::Type type) = 0;

        virtual DescriptorSet *newDescriptorSet(size_t count, DescriptorSet::Type type, bool shaderVisible) = 0;

        virtual void createRenderTargetView(Buffer *buffer, CpuHandle handle) = 0;
        virtual void createConstBufferView(Buffer *buffer, size_t size, CpuHandle handle) = 0;
        virtual void createDepthStencilView(Buffer *buffer, CpuHandle handle) = 0;

        virtual Buffer *newBuffer(size_t size, DescriptorSet::Visibility visibility, rhi::Buffer::State state) = 0;

        virtual PipelineState *newPipelineState(const PipelineBinding& bindings) = 0;

        virtual void imguiInit(size_t frames, DescriptorSet *heap, CpuHandle cpuHandle, GpuHandle gpuHandle) = 0;
        virtual void imguiNewFrame() = 0;

        virtual ~Device() = default;
    };

    Device *getDevice();
}
