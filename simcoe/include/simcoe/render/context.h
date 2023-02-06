#pragma once

#include "simcoe/core/system.h"
#include "simcoe/core/logging.h"
#include "simcoe/render/heap.h"
#include "simcoe/simcoe.h"

#include "dx/d3d12.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace simcoe::render {
    enum struct DisplayMode {
        eLetterBox,
        eStretch,

        eTotal
    };
    
    struct Context {
        struct Info {
            size_t adapter = SIZE_MAX; // which adapter to use
            size_t frames = 2; // number of back buffers
            system::Size resolution = { 1280, 720 }; // resolution of the back buffers
            DisplayMode displayMode = DisplayMode::eLetterBox; // how to display the back buffers
        
            size_t heapSize = 1024;
            size_t workerThreads = 2;
        };

        Context(system::Window& window, const Info& info);
        ~Context();

        void update(const Info& info);

        void begin();
        void end();

        void present();

        ID3D12Device *getDevice() const { return pDevice; }

        size_t getFrames() const { return info.frames; }
        size_t getCurrentFrame() const { return frameIndex; }

        Heap& getHeap() { return cbvHeap; }

        ID3D12GraphicsCommandList *getCommandList() const { return pCommandList; }
        
        D3D12_CPU_DESCRIPTOR_HANDLE getRenderTarget() const { 
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRenderTargetHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
            return rtvHandle;
        }

        ID3D12Resource *getRenderTargetResource() const { return renderTargets[frameIndex]; }

    private:
        void newFactory();
        void deleteFactory();

        void newDevice();
        void deleteDevice();

        void selectAdapter(size_t index);
        void selectDefaultAdapter();

        void newPresentQueue();
        void deletePresentQueue();

        void newSwapChain();
        void deleteSwapChain();

        void newRenderTargets();
        void deleteRenderTargets();

        void newCommandList();
        void deleteCommandList();

        void newFence();
        void deleteFence();

        void newDescriptorHeap();
        void deleteDescriptorHeap();

        void waitForFence();
        void nextFrame();

        system::Window& window;
        Info info;

        ID3D12Debug* pDebug = nullptr;

        IDXGIFactory6 *pFactory = nullptr;

        IDXGIAdapter1 *pAdapter = nullptr;

        ID3D12Device *pDevice = nullptr;

        ID3D12CommandQueue *pPresentQueue = nullptr;

        BOOL bTearingSupported = FALSE;
        IDXGISwapChain3 *pSwapChain = nullptr;

        ID3D12DescriptorHeap *pRenderTargetHeap = nullptr;
        UINT rtvDescriptorSize = 0;

        std::vector<ID3D12Resource*> renderTargets;

        std::vector<ID3D12CommandAllocator*> commandAllocators;
        ID3D12GraphicsCommandList *pCommandList = nullptr;

        UINT64 fenceValue = 1;
        UINT frameIndex = 0;
        HANDLE fenceEvent = nullptr;
        ID3D12Fence *pFence = nullptr;

        Heap cbvHeap;
    };
}