#pragma once

#include <vector>

#include "utils.h"
#include "window.h"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <DirectXMath.h>

#include <wrl.h>

namespace render {
    template<typename T>
    struct Com {
        using Type = T;
        using Pointer = T*;

        Com(Type *ptr = nullptr) : self(ptr) {}

        template<typename A>
        A as() {
            using Ptr = typename A::Pointer;
            Ptr ptr = nullptr;
            if (FAILED(self->QueryInterface(IID_PPV_ARGS(&ptr)))) {
                return nullptr;
            }
            return A(ptr);
        }

        bool valid() const { return get() != nullptr; }

        T *operator->() { return get(); }
        T *operator*() { return get(); }
        T **operator&() { return addr(); }

        T *get() const { return self; }
        T **addr() { return &self; }

        ULONG release() { return self->Release(); }
        void drop() { ASSERT(release() == 0); }
        void rename(LPCWSTR name) { self->SetName(name); }

    private:
        T *self;
    };

    struct Viewport : D3D12_VIEWPORT {
        using Super = D3D12_VIEWPORT;

        Viewport(FLOAT width, FLOAT height)
            : Super({ 0.f, 0.f, width, height, 0.f, 0.f })
        { 
            ASSERT(width >= 0.f);
            ASSERT(height >= 0.f);
        }
    };

    struct Scissor : D3D12_RECT {
        using Super = D3D12_RECT;

        Scissor(LONG width, LONG height)
            : Super({ 0, 0, width, height })
        { }
    };

    using namespace DirectX;

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT4 colour;
    };

    namespace win32 {
        using Handle = HANDLE;
    }

    namespace dxgi {
        using Factory4 = Com<IDXGIFactory4>;
        using Adapter1 = Com<IDXGIAdapter1>;
        using SwapChain1 = Com<IDXGISwapChain1>;
        using SwapChain3 = Com<IDXGISwapChain3>;
        using Debug = Com<IDXGIDebug>;
    }

    namespace d3d {
        using Blob = Com<ID3DBlob>;
    }

    namespace d3d12 {
        using InfoQueue1 = Com<ID3D12InfoQueue1>;
        using Debug1 = Com<ID3D12Debug1>;
        using Device1 = Com<ID3D12Device1>;
        using CommandQueue = Com<ID3D12CommandQueue>;
        using DescriptorHeap = Com<ID3D12DescriptorHeap>;
        using Resource = Com<ID3D12Resource>;
        using CommandAllocator = Com<ID3D12CommandAllocator>;
        using RootSignature = Com<ID3D12RootSignature>;
        using PipelineState = Com<ID3D12PipelineState>;
        using CommandList = Com<ID3D12GraphicsCommandList>;
        using Fence = Com<ID3D12Fence>;
    }

    struct Factory {
        void create(bool debugging);
        void destroy();

        d3d12::Debug1 debug;
        dxgi::Factory4 factory;
        std::vector<dxgi::Adapter1> adapters;
    };

    struct Context {
        /// create a render context 
        void create(Factory *factory, size_t index);
        void destroy();

        void attach(WindowHandle *handle, UINT frames);
        void detach();

        void load();
        void unload();

        void gpuWait();
        void nextFrame();
        void present();

        dxgi::Adapter1 getAdapter() const; /// get the selected adapter
        dxgi::Factory4 getFactory() const; /// get the factory

        Factory *factory; /// the factory used to create this render context
        size_t index; /// the index of the current adapter
        
        d3d12::Device1 device; /// the current device
        d3d12::CommandQueue commandQueue; /// the command queue

        WindowHandle *handle; /// the window handle
        UINT frames; /// total number of queued frames
        dxgi::SwapChain3 swapChain; /// the swap chain
    
        d3d12::DescriptorHeap rtvHeap; /// the render target view heap
        UINT rtvDescriptorSize; /// the size of a render target view descriptor

        d3d12::DescriptorHeap cbvHeap; /// the constant buffer view heap

        d3d12::Resource *renderTargets; /// the render target views
        d3d12::CommandAllocator *commandAllocators; /// the command allocator
        d3d12::CommandAllocator bundleAllocator; /// the bundle allocator

        d3d12::RootSignature rootSignature; /// the root signature
        d3d12::PipelineState pipelineState; /// the pipeline state
        d3d12::CommandList commandList; /// the command list
        d3d12::CommandList bundleList; /// the bundle list

        d3d12::Resource vertexBuffer; /// the vertex buffer
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView; /// the vertex buffer view

        UINT currentFrame; /// the current frame index
        d3d12::Fence fence; /// the fence
        UINT64 *fenceValues; /// the current fence value
        win32::Handle fenceEvent; /// the fence event

        Viewport viewport = Viewport(0.f, 0.f);
        Scissor scissor = Scissor(0, 0);
    };
}
