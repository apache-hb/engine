#pragma once

#include "util.h"
#include "system/system.h"
#include "assets/loader.h"
#include "input/camera.h"

namespace engine::render {
    namespace Slots {
        enum Index : int {
            Buffer = 0,
            Texture = 1,
            ImGui = 2,
            Total
        };
    }

    namespace Allocator {
        enum Index : int {
            Context = 0,
            Total
        };
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
        };
        
        Context(Factory factory)
            : factory(factory)
        { }

        ~Context() {
            destroyAssets();
            destroyDevice();
        }

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

        void populate();
        void flushQueue();
        void waitForGPU();
        void nextFrame();

        Factory factory;
        Create create;

        struct View {
            d3d12::Scissor scissor{0, 0};
            d3d12::Viewport viewport{0.f, 0.f};
        };

        struct Frame {
            Com<ID3D12Resource> target;
            Com<ID3D12CommandAllocator> allocators[Allocator::Total];
            UINT64 fenceValue;
        };

        View scene;

        /// pipeline objects
        Com<ID3D12Device> device;
        Com<ID3D12CommandQueue> queue;
        Com<IDXGISwapChain3> swapchain;
        float aspect;

        Com<ID3D12RootSignature> rootSignature;
        Com<ID3D12PipelineState> pipelineState;

        Com<ID3D12DescriptorHeap> cbvSrvHeap;
        UINT cbvSrvDescriptorSize;

        auto getCbvSrvGpuHandle(UINT index) {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), index, cbvSrvDescriptorSize);
        }

        auto getCbvSrvCpuHandle(UINT index) {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), index, cbvSrvDescriptorSize);
        }

        Com<ID3D12DescriptorHeap> rtvHeap;
        UINT rtvDescriptorSize;

        Com<ID3D12GraphicsCommandList> commandList;

        /// resources
        Com<ID3D12Resource> vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        Com<ID3D12Resource> indexBuffer;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        Com<ID3D12Resource> constBuffer;
        void *constBufferPtr;

        Com<ID3D12Resource> texture;

        loader::Model model;

        /// frame data
        Frame *frames;

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
