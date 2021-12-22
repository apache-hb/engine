#pragma once

#include <span>

#include "util.h"
#include "library.h"
#include "device.h"
#include "heap.h"
#include "factory.h"
#include "system/system.h"
#include "assets/loader.h"
#include "input/camera.h"

namespace engine::render {
    namespace Resource {
        enum Index : int {
            Camera, /// camera projection data
            Intermediate, /// intermediate scaling render target
            ImGui, /// render target for imgui
            Total
        };
    }

    namespace Allocator {
        enum Index : int {
            Scene,
            Post,
            Copy,
            Total
        };

        constexpr D3D12_COMMAND_LIST_TYPE getType(Index index) {
            switch (index) {
            case Scene: case Post: return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
            default: throw engine::Error("invalid allocator index");
            }
        }

        constexpr std::wstring_view getName(Index index) {
            switch (index) {
            case Scene: return L"scene";
            case Post: return L"post";
            case Copy: return L"copy";
            default: throw engine::Error("invalid allocator index");
            }
        }
    }

    namespace debug {
        void enable();
        void disable();
        void report();
    }

    struct Context {
        struct Create {
            Adapter adapter;
            system::Window *window;
            UINT frames;
            Resolution resolution;
        };
        
        Context(Factory* factory)
            : factory(factory)
        { }

        ~Context() {
            destroyAssets();
            destroyDevice();
        }

        void changeInternalResolution(Resolution resolution);

        void createDevice(Create& info);
        void destroyDevice();

        void createAssets();
        void destroyAssets();

        void present();
        void tick(const input::Camera& camera);

        ConstBuffer constBufferData;

    private:
        void attachInfoQueue();
        D3D_ROOT_SIGNATURE_VERSION rootVersion();

        void updateViews();

        void populate();

        void flushQueue();
        void flushCopyQueue();

        void waitForGPU(Com<ID3D12CommandQueue> queue);
        void nextFrame(Com<ID3D12CommandQueue> queue);

        Factory* factory;
        Create create;

        struct Frame {
            Com<ID3D12Resource> target;
            Com<ID3D12CommandAllocator> allocators[Allocator::Total];
            UINT64 fenceValue;
        };

        Com<ID3D12Resource> uploadBuffer(const void *data, size_t size, std::wstring_view name = L"resource");
        Com<ID3D12Resource> uploadTexture(const loader::Texture& texture, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::wstring_view name = L"texture");

        template<typename T>
        Com<ID3D12Resource> uploadSpan(const std::span<T>& data, std::wstring_view name = L"resource") {
            return uploadBuffer(data.data(), data.size() * sizeof(T), name);
        }

        template<typename T>
        std::tuple<Com<ID3D12Resource>, D3D12_VERTEX_BUFFER_VIEW> 
        uploadVertexBuffer(const std::span<T>& data, std::wstring_view name = L"vertex-resource") {
            auto resource = uploadSpan(data, name);
            D3D12_VERTEX_BUFFER_VIEW view = {
                .BufferLocation = resource->GetGPUVirtualAddress(),
                .SizeInBytes = UINT(data.size() * sizeof(T)),
                .StrideInBytes = sizeof(T)
            };

            return std::make_tuple(resource, view);
        }

        template<typename T>
        std::tuple<Com<ID3D12Resource>, D3D12_INDEX_BUFFER_VIEW> 
        uploadIndexBuffer(const std::span<T>& data, DXGI_FORMAT format, std::wstring_view name = L"index-resource") {
            auto resource = uploadSpan(data, name);
            D3D12_INDEX_BUFFER_VIEW view = {
                .BufferLocation = resource->GetGPUVirtualAddress(),
                .SizeInBytes = UINT(data.size() * sizeof(T)),
                .Format = format
            };
            
            return std::make_tuple(resource, view);
        }

        View sceneView;
        View postView;

        /// basic pipeline objects
        Device<ID3D12Device4> device;
        Com<ID3D12CommandQueue> directQueue;
        Com<IDXGISwapChain3> swapchain;

        float internalAspect;
        LONG internalWidth;
        LONG internalHeight;

        float displayAspect;
        LONG displayWidth;
        LONG displayHeight;

        Com<ID3D12DescriptorHeap> dsvHeap;
        
        DescriptorHeap rtvHeap;
        DescriptorHeap cbvSrvHeap;

        UINT requiredCbvSrvHeapEntries() const {
            return UINT(scene.textures.size() + Resource::Total);
        }

        /// render target resource layout
        /// 
        /// --------------------------------------------------
        /// intermediate | rtv1 | rtvN

        UINT requiredRtvHeapEntries() const {
            return create.frames + 1; // +1 for intermediate frame
        }
    
        auto getIntermediateHandle() {
            return rtvHeap.cpuHandle(0);
        }

        auto getTargetHandle(size_t frame) {
            return rtvHeap.cpuHandle(frame + 1);
        }

        auto getTextureSlotGpuHandle(size_t index) {
            return cbvSrvHeap.gpuHandle(index + Resource::Total);
        }

        auto getTextureSlotCpuHandle(size_t index) {
            return cbvSrvHeap.cpuHandle(index + Resource::Total);
        }

        /// resource copying state
        Com<ID3D12CommandQueue> copyQueue;
        Com<ID3D12GraphicsCommandList> copyCommandList;        
        std::vector<Com<ID3D12Resource>> copyResources;

        /// resources
        Com<ID3D12Resource> vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        Com<ID3D12Resource> indexBuffer;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        Com<ID3D12Resource> constBuffer;
        void *constBufferPtr;

        Com<ID3D12Resource> screenBuffer;
        D3D12_VERTEX_BUFFER_VIEW screenBufferView;

        Com<ID3D12Resource> depthStencil;

        std::vector<Com<ID3D12Resource>> textures;

        loader::Scene scene;

        /// frame data
        Frame *frames;

        Com<ID3D12GraphicsCommandList> sceneCommandList;
        Com<ID3D12GraphicsCommandList> postCommandList;

        Com<ID3D12RootSignature> sceneRootSignature;
        Com<ID3D12PipelineState> scenePipelineState;

        Com<ID3D12RootSignature> postRootSignature;
        Com<ID3D12PipelineState> postPipelineState;

        ShaderLibrary sceneShaders;
        ShaderLibrary postShaders;

        Com<ID3D12Resource> sceneTarget;

        auto &getAllocator(Allocator::Index type, size_t index = SIZE_MAX) noexcept {
            if (index == SIZE_MAX) { index = frameIndex; }
            return frames[index].allocators[type];
        } 

        auto &getTarget(size_t index = SIZE_MAX) noexcept {
            if (index == SIZE_MAX) { index = frameIndex; }
            return frames[index].target;
        }

        /// sync objects
        Com<ID3D12Fence> fence;
        HANDLE fenceEvent;
        UINT frameIndex;
    };
}
