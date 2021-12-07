#pragma once

#include "system/system.h"
#include "util.h"

namespace engine::render {
    struct Context {
        Context() = default;
        Context(Factory factory, size_t adapterIndex, system::Window *window, UINT frameCount)
            : factory(factory)
            , adapterIndex(adapterIndex)
            , window(window)
            , frameCount(frameCount) 
        { }

        /// data given in from the constructor
        Factory factory;
        size_t adapterIndex;
        system::Window *window;
        UINT frameCount;

        Adapter getAdapter() { return factory.adapters[adapterIndex]; }

        HRESULT createCore();
        void destroyCore();

        HRESULT createAssets();
        void destroyAssets();

        HRESULT present();
        HRESULT populate();
        HRESULT nextFrame();
        HRESULT waitForGpu();
        bool retry(size_t attempts = 1);

        void loseDevice() {
            if (auto device5 = device.as<d3d12::Device5>(); device5) {
                auto real = device5.value();
                render->warn("dropping device");
                real.release();
                real->RemoveDevice();
            }
        }

        struct FrameData {
            d3d12::Resource renderTarget;
            d3d12::CommandAllocator commandAllocator;
            UINT64 fenceValue;
        };

        cbuffer ConstBuffer {
            XMFLOAT4 offset;
        };

        d3d12::Viewport viewport = d3d12::Viewport(0.f, 0.f);
        d3d12::Scissor scissor = d3d12::Scissor(0, 0);

        d3d12::Device1 device;
        d3d12::CommandQueue commandQueue;
        dxgi::SwapChain3 swapchain;

        d3d12::DescriptorHeap cbvHeap;        
        d3d12::DescriptorHeap srvHeap;
        d3d12::DescriptorHeap rtvHeap;
        UINT rtvDescriptorSize;

        FrameData *frameData;

        d3d12::RootSignature rootSignature;
        d3d12::PipelineState pipelineState;
        d3d12::CommandList commandList;

        HRESULT createCommandList(
            D3D12_COMMAND_LIST_TYPE type,
            d3d12::CommandAllocator allocator,
            d3d12::PipelineState state,
            ID3D12GraphicsCommandList **list
        );

        d3d12::Resource vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        d3d12::Resource constBuffer;
        ConstBuffer constBufferData;
        void *constBufferPtr;

        d3d12::Fence fence;
        HANDLE fenceEvent;
        UINT frameIndex;
    };

    Context createContext(Factory factory, system::Window *window, size_t adapter, UINT frameCount);
}
