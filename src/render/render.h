#pragma once

#include "system/system.h"
#include "util.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

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
        HRESULT waitForFrame();
        bool retry();

        void loseDevice() {
            if (auto device5 = device.as<d3d12::Device5>(); device5) {
                auto real = device5.value();
                render->warn("dropping device");
                real.release();
                real->RemoveDevice();
            }
        }

        d3d12::Viewport viewport = d3d12::Viewport(0.f, 0.f);
        d3d12::Scissor scissor = d3d12::Scissor(0, 0);

        d3d12::Device1 device;
        d3d12::CommandQueue commandQueue;
        dxgi::SwapChain3 swapchain;
        
        d3d12::DescriptorHeap rtvHeap;
        UINT rtvDescriptorSize;

        d3d12::DescriptorHeap srvHeap;

        d3d12::Resource *renderTargets;
        d3d12::CommandAllocator commandAllocator;

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

        d3d12::Fence fence;
        HANDLE fenceEvent;
        UINT frameIndex;
        UINT64 fenceValue;
    };

    Context createContext(Factory factory, system::Window *window, size_t adapter, UINT frameCount);
}
