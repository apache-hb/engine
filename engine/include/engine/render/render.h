#pragma once

#include "engine/base/logging.h"

#include "engine/render/window.h"

#include "dx/d3d12.h"

#include "util.h"

namespace engine::render {
    constexpr auto kFrameCount = 2;
    struct Context {
        Context(Window *window, logging::Channel *channel);

        void begin();
        void end();

        void close();

        Com<ID3D12Debug> debug;
        Com<IDXGIFactory4> factory;
        Com<IDXGIAdapter1> adapter;
        Com<ID3D12Device> device;

        Com<ID3D12CommandQueue> commandQueue;
        Com<ID3D12GraphicsCommandList> commandList;
        Com<ID3D12PipelineState> pipelineState;
        Com<IDXGISwapChain3> swapChain;

        Com<ID3D12DescriptorHeap> rtvHeap;
        UINT rtvSize;

        Com<ID3D12CommandAllocator> commandAllocators[kFrameCount];
        Com<ID3D12Resource> renderTargets[kFrameCount];

        UINT frameIndex;

        Com<ID3D12Fence> fence;
        UINT64 fenceValue;
        HANDLE fenceEvent;

    private:
        void waitForFrame();
    };
}
