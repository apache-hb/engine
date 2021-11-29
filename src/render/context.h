#include <vector>

#include "util.h"
#include "../window.h"

namespace render {
    struct Instance {
        HRESULT create();
        void destroy();

    private:
        friend struct Context;

        d3d12::Debug debug;
        dxgi::Factory4 factory;
        std::vector<dxgi::Adapter1> adapters;
    };

    struct Context {
        HRESULT create(const Instance &instance, size_t index);
        void destroy();

        HRESULT attach(WindowHandle *handle, UINT buffers);
        void detach();

        void present();

    private:
        HRESULT createDevice();
        HRESULT createSwapChain();
        HRESULT createAssets();
        void unload();

        void recover(HRESULT hr);

        /// immutable instance data
        dxgi::Factory4 factory;
        dxgi::Adapter1 adapter;

        // immutable window data
        WindowHandle *window;
        UINT frames;

        d3d12::Scissor scissor = d3d12::Scissor(0, 0);
        d3d12::Viewport viewport = d3d12::Viewport(0, 0);

        // core context data
        d3d12::InfoQueue1 infoQueue;
        d3d12::Device1 device;
        d3d12::CommandQueue commandQueue;
        dxgi::SwapChain3 swapChain;

        d3d12::CommandAllocator *commandAllocators;
        d3d12::Resource *renderTargets;

        d3d12::DescriptorHeapRTV rtvHeap;
        d3d12::RootSignature rootSignature;
        d3d12::PipelineState pipelineState;
        d3d12::Commands commandList;

        /// the triangle
        d3d12::CommandAllocator bundleAllocator;
        d3d12::Commands bundle;
        d3d12::Resource vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        // synchronization objects
        UINT frameIndex;
        HANDLE fenceEvent;
        d3d12::Fence fence;
        UINT64 *fenceValues;

        HRESULT waitForGpu();
        HRESULT nextFrame();
    };
}
