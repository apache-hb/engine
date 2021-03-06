#pragma once

#include <span>

#include "util.h"
#include "objects/library.h"
#include "objects/device.h"
#include "objects/heap.h"
#include "factory.h"
#include "system/system.h"
#include "assets/loader.h"
#include "input/camera.h"
#include "scene/scene.h"

namespace engine::render {
    namespace SceneResources {
        enum Index : int {
            Camera, /// camera projection data
            Total
        };
    }

    namespace PostResources {
        enum Index : int {
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

    /**
     * @brief encapsulates all rendering state
     * 
     * contains one of each queue type and manages their allocators.
     * is responsible for uploading data to the gpu, managing lifetimes.
     * handles all post processing including resolution scaling and imgui
     */
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

        struct Image {
            size_t width;
            size_t height;
            size_t component;
            
            std::vector<unsigned char> data;
        };


        /// resource uploading
        Resource uploadTexture(std::wstring_view name, const Image& texture);


        /// lifetime management interface
        void createDevice(Create& info);
        void destroyDevice();

        void createAssets();
        void destroyAssets();


        /// presentation interface
        void present();
        void tick(const input::Camera& camera);

        ConstBuffer constBufferData;

    private:
        void attachInfoQueue();

        void updateViews();

        void populate();

        void flushQueue();
        void flushCopyQueue();

        void waitForGPU(Object<ID3D12CommandQueue> queue);
        void nextFrame(Object<ID3D12CommandQueue> queue);

        struct Frame {
            Resource target;
            Object<ID3D12CommandAllocator> allocators[Allocator::Total];
            UINT64 fenceValue;
        };

        Factory* factory;
        Create create;

        std::vector<Resource> buffers;
        std::vector<Resource> textures;

        View sceneView;
        View postView;

        /// basic pipeline objects
        Device<ID3D12Device4> device;
        Com<IDXGISwapChain3> swapchain;

        // queues
        Object<ID3D12CommandQueue> directQueue;
        Object<ID3D12CommandQueue> copyQueue;
        Object<ID3D12CommandQueue> computeQueue;

        float internalAspect;
        LONG internalWidth;
        LONG internalHeight;

        float displayAspect;
        LONG displayWidth;
        LONG displayHeight;

        DescriptorHeap rtvHeap;

        /// resource copying state
        Object<ID3D12GraphicsCommandList> copyCommandList;        
        std::vector<Resource> copyResources;

        /// frame data
        Frame *frames;

        /// post specific resources
        CommandBundle postCommandBundle;
        Object<ID3D12GraphicsCommandList> postCommandList;

        ShaderLibrary postShaders;
        Object<ID3D12RootSignature> postRootSignature;
        Object<ID3D12PipelineState> postPipelineState;

        DescriptorHeap postCbvHeap;
        VertexBuffer screenBuffer;

        /// scene specific resources
        CommandBundle sceneCommandBundle;
        Object<ID3D12GraphicsCommandList> sceneCommandList;

        ShaderLibrary sceneShaders;
        Object<ID3D12RootSignature> sceneRootSignature;
        Object<ID3D12PipelineState> scenePipelineState;

        /// all our textures currently go into this heap
        DescriptorHeap sceneCbvSrvHeap;
        Object<ID3D12DescriptorHeap> dsvHeap;

        // our intermediate render target
        // the scene draws to this rather than to the swapchain
        Resource depthStencil;
        Resource sceneTarget;

        // camera data
        Resource constBuffer;
        void *constBufferPtr;

        auto &getAllocator(Allocator::Index type, size_t index = SIZE_MAX) noexcept {
            if (index == SIZE_MAX) { index = frameIndex; }
            return frames[index].allocators[type];
        } 

        auto &getTarget(size_t index = SIZE_MAX) noexcept {
            if (index == SIZE_MAX) { index = frameIndex; }
            return frames[index].target;
        }

        ///
        /// slot accessing functions
        ///
        
        UINT requiredPostCbvHeapEntries() const {
            return PostResources::Total;
        }

        UINT requiredSceneCbvSrvHeapEntries() const {
            return UINT(textures.size() + SceneResources::Total);
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
            return sceneCbvSrvHeap.gpuHandle(index + SceneResources::Total);
        }

        auto getTextureSlotCpuHandle(size_t index) {
            return sceneCbvSrvHeap.cpuHandle(index + SceneResources::Total);
        }



        ///
        /// resource uploading queuing functions
        ///

        Resource uploadBuffer(const void *data, size_t size, std::wstring_view name = L"resource");

        template<typename T>
        Resource uploadSpan(const std::span<T>& data, std::wstring_view name = L"resource") {
            return uploadBuffer(data.data(), data.size() * sizeof(T), name);
        }

        template<typename T>
        VertexBuffer uploadVertexBuffer(const std::span<T>& data, std::wstring_view name = L"vertex-resource") {
            auto resource = uploadSpan(data, name);
            return VertexBuffer {
                .resource = resource, 
                .view = {
                    .BufferLocation = resource->GetGPUVirtualAddress(),
                    .SizeInBytes = UINT(data.size() * sizeof(T)),
                    .StrideInBytes = sizeof(T)
                }
            };
        }

        template<typename T>
        IndexBuffer uploadIndexBuffer(const std::span<T>& data, DXGI_FORMAT format, std::wstring_view name = L"index-resource") {
            auto resource = uploadSpan(data, name);

            return IndexBuffer(resource, {
                .BufferLocation = resource->GetGPUVirtualAddress(),
                .SizeInBytes = UINT(data.size() * sizeof(T)),
                .Format = format
            });
        }

        /// sync objects
        Object<ID3D12Fence> fence;
        HANDLE fenceEvent;
        UINT frameIndex;
    };
}
