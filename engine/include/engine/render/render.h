#pragma once

#include "engine/base/logging.h"

#include "engine/render/window.h"

#include "dx/d3dx12.h"
#include "dx/d3d12.h"

#include "util.h"

namespace engine::render {
    constexpr auto kFrameCount = 2;

    struct Vertex {
        math::float3 pos;
        math::float4 colour;
    };

    struct DX_CBUFFER ConstBuffer {
        math::float4 offset { 0.3f, 0.f, 0.f, 0.f };
    };

    struct Context {
        Context(Window *window, logging::Channel *channel);

        void begin(float elapsed);
        void end();

        void close();

        // dxgi objects
        Com<ID3D12Debug> debug;
        Com<IDXGIFactory4> factory;
        Com<IDXGIAdapter1> adapter;
        Com<IDXGISwapChain3> swapChain;
        Com<ID3D12Device> device;

        // general pipeline state
        Com<ID3D12CommandQueue> commandQueue;
        Com<ID3D12GraphicsCommandList> commandList;
        Com<ID3D12PipelineState> pipelineState;
        
        CD3DX12_VIEWPORT viewport;
        CD3DX12_RECT scissor;

        // render target heap
        Com<ID3D12DescriptorHeap> rtvHeap;
        UINT rtvSize;

        Com<ID3D12DescriptorHeap> cbvHeap;

        // per frame resources
        Com<ID3D12CommandAllocator> commandAllocators[kFrameCount];
        Com<ID3D12Resource> renderTargets[kFrameCount];

        // render data
        Com<ID3D12RootSignature> rootSignature;

        Com<ID3D12Resource> vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        Com<ID3D12Resource> indexBuffer;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        Com<ID3D12Resource> constantBuffer;
        ConstBuffer bufferData;
        void *bufferPtr;

        // synchronization objects
        UINT frameIndex;
        Com<ID3D12Fence> fence;
        UINT64 fenceValue;
        HANDLE fenceEvent;

    private:
        void waitForFrame();
    };
}
