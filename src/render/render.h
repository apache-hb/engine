#pragma once

#include "util.h"
#include "camera.h"
#include "system/system.h"

namespace engine::render {
    namespace debug {
        void enable();
        void disable();
        void report();
    }

    struct Resolution {
        UINT width;
        UINT height;
    };

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

        Camera camera;

    private:
        void attachInfoQueue();
        D3D_ROOT_SIGNATURE_VERSION rootVersion();

        void populate();
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
            Com<ID3D12CommandAllocator> allocator;
            UINT64 fenceValue;
        };

        View scene;

        /// pipeline objects
        Com<ID3D12Device> device;
        Com<ID3D12CommandQueue> queue;
        Com<IDXGISwapChain3> swapchain;

        Com<ID3D12RootSignature> rootSignature;
        Com<ID3D12PipelineState> pipelineState;

        Com<ID3D12DescriptorHeap> cbvHeap;
        Com<ID3D12DescriptorHeap> rtvHeap;
        UINT rtvDescriptorSize;

        Com<ID3D12GraphicsCommandList> commandList;

        /// resources
        Com<ID3D12Resource> vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        Com<ID3D12Resource> constBuffer;
        void *constBufferPtr;

        /// frame data
        Frame *frames;

        auto &getAllocator(size_t index = SIZE_MAX) {
            if (index == SIZE_MAX) { index = frameIndex; }
            return frames[index].allocator;
        } 

        /// sync objects
        Com<ID3D12Fence> fence;
        HANDLE fenceEvent;
        UINT frameIndex;
    };
}
