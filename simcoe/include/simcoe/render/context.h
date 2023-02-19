#pragma once

#include "simcoe/core/system.h"
#include "simcoe/core/logging.h"
#include "simcoe/core/util.h"

#include "simcoe/render/heap.h"
#include "simcoe/render/queue.h"

#include "simcoe/simcoe.h"

#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <unordered_map>

namespace simcoe::render {
    struct Context {
        constexpr static inline size_t kDefaultAdapter = SIZE_MAX;

        struct Info {
            size_t adapter = kDefaultAdapter; // which adapter to use
            size_t frames = 2; // number of back buffers

            system::Size windowSize = { 1280, 720 }; //presentation resolution
        
            size_t heapSize = 1024;
            size_t queueSize = 1024;
            size_t workerThreads = 2;
        };

        Context(system::Window& window, const Info& info);
        ~Context();

        void update(const Info& info);

        void begin();
        void end();

        void present();

        ID3D12Device *getDevice() const { return pDevice; }
        IDXGISwapChain *getSwapChain() const { return pSwapChain; }
        ID3D12GraphicsCommandList *getCommandList() const { return pCommandList; }
        
        size_t getFrames() const { return info.frames; }
        size_t getCurrentFrame() const { return frameIndex; }

        Heap& getCbvHeap() { return cbvHeap; }
        Heap& getRtvHeap() { return rtvHeap; }

        Queue& getCopyQueue() { return copyQueue; }

    private:
        void newFactory();
        void deleteFactory();

        void newDevice();
        void deleteDevice();

        void newInfoQueue();
        void deleteInfoQueue();

        void listAdapters();

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

        void newCopyQueue();
        void deleteCopyQueue();

        void waitForFence();
        void nextFrame();

        constexpr size_t getRenderHeapSize() const { return info.frames + 1; }

        using ReportOnceMap = std::unordered_map<D3D12_MESSAGE_ID, util::DoOnce>;

        system::Window& window;
        Info info;

        ID3D12Debug* pDebug = nullptr;

        IDXGIFactory6 *pFactory = nullptr;

        IDXGIAdapter1 *pAdapter = nullptr;

        ID3D12Device *pDevice = nullptr;

        ReportOnceMap reportOnceMap;
        ID3D12InfoQueue1 *pInfoQueue = nullptr;

        ID3D12CommandQueue *pPresentQueue = nullptr;

        BOOL bTearingSupported = FALSE;
        IDXGISwapChain3 *pSwapChain = nullptr;

        Heap rtvHeap;

        ID3D12Resource *pSceneTarget = nullptr;

        std::vector<ID3D12CommandAllocator*> commandAllocators;
        ID3D12GraphicsCommandList *pCommandList = nullptr;

        UINT64 fenceValue = 1;
        UINT frameIndex = 0;
        HANDLE fenceEvent = nullptr;
        ID3D12Fence *pFence = nullptr;

        Heap cbvHeap;
        Queue copyQueue;
    };
}
