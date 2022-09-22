#pragma once

#include "dx/d3d12.h"
#include "engine/base/logging.h"

#include "engine/render/window.h"

#include "engine/render/objects/device.h"
#include "engine/render/objects/resource.h"
#include "engine/render/objects/commands.h"
#include "engine/render/objects/fence.h"

namespace engine::render {
    constexpr auto kFrameCount = 2;

    struct Vertex {
        math::float3 pos;
        math::float4 colour;
    };

    struct DX_CBUFFER ConstBuffer {
        math::float4 offset { 0.3f, 0.f, 0.f, 0.f };
    };

    void enableSm6(logging::Channel *channel);

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
        d3d12::Device<> device;

        // cached features
        bool tearing = false;

        // graphics pipeline state
        d3d12::CommandQueue<> commandQueue;
        d3d12::GraphicsCommandList<> graphicsCommandList;

        // copy related resources
        d3d12::CommandQueue<> copyQueue;
        d3d12::GraphicsCommandList<> copyCommandList;
        Object<ID3D12CommandAllocator> copyAllocator;

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

        // d3d12::ConstBuffer<ConstBuffer> constBuffer;

        Com<ID3D12Resource> constantBuffer;
        ConstBuffer bufferData;
        void *bufferPtr;

        // synchronization objects
        UINT frameIndex;
        
        d3d12::Fence<> fence;

        UINT64 fenceValue;

        d3d12::Resource<> uploadBuffer(size_t size, const void *src);

    private:
        void waitForFrame();
    };
}
