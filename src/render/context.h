#include <vector>

#include "util.h"
#include "../window.h"

namespace render {
    namespace dxgi {
        using Factory4 = Ptr<IDXGIFactory4>;
        using Adapter1 = Ptr<IDXGIAdapter1>;
        using SwapChain1 = Ptr<IDXGISwapChain1>;
        using SwapChain3 = Ptr<IDXGISwapChain3>;
    }

    namespace d3d {
        using Blob = Ptr<ID3DBlob>;
    }

    namespace d3d12 {
        using Device1 = Ptr<ID3D12Device1>;
        using CommandQueue = Ptr<ID3D12CommandQueue>;
        using InfoQueue1 = Ptr<ID3D12InfoQueue1>;
        using Debug = Ptr<ID3D12Debug>;
        using Fence = Ptr<ID3D12Fence>;
        using DescriptorHeap = Ptr<ID3D12DescriptorHeap>;
        using CommandAllocator = Ptr<ID3D12CommandAllocator>;
        using Resource = Ptr<ID3D12Resource>;
        using RootSignature = Ptr<ID3D12RootSignature>;
        using PipelineState = Ptr<ID3D12PipelineState>;
        using CommandList = Ptr<ID3D12GraphicsCommandList>;
    
        struct DescriptorHeapRTV : DescriptorHeap {
            UINT descriptorSize;
        };
    }

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
        d3d12::CommandList commandList;

        // synchronization objects
        UINT frameIndex;
        HANDLE fenceEvent;
        d3d12::Fence fence;
        UINT64 *fenceValues;
    };

}
