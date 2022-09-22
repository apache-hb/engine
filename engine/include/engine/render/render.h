#pragma once

#include "engine/base/logging.h"

#include "engine/render/window.h"

#include "engine/render/objects/queue.h"
#include "engine/render/objects/view.h"
#include "engine/render/objects/resource.h"

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

        // cached features
        bool tearing = false;

        // general pipeline state
        d3d12::CommandQueue<> commandQueue;
        Com<ID3D12GraphicsCommandList> commandList;
        Com<ID3D12PipelineState> pipelineState;
        
        d3d12::View view;

        // render target heap
        Com<ID3D12DescriptorHeap> rtvHeap;
        UINT rtvSize;

        Com<ID3D12DescriptorHeap> cbvHeap;

        // per frame resources
        Com<ID3D12CommandAllocator> commandAllocators[kFrameCount];
        Com<ID3D12Resource> renderTargets[kFrameCount];

        // render data
        Com<ID3D12RootSignature> rootSignature;

        d3d12::VertexBuffer<> vertexBuffer;
        d3d12::IndexBuffer<> indexBuffer;

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
