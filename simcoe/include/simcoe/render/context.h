#pragma once

#include "simcoe/core/system.h"
#include "simcoe/core/logging.h"
#include "simcoe/core/util.h"

#include "simcoe/render/heap.h"

#include "simcoe/simcoe.h"

#include "dx/d3d12.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace simcoe::render {
    struct Fence {
        void newFence(ID3D12Device *pDevice);
        void deleteFence();
        
        void wait(ID3D12CommandQueue *pQueue);

        ID3D12Fence *pFence = nullptr;
        HANDLE hEvent = nullptr;
        UINT64 fenceValue = 1;
    };

    struct CommandBuffer {
        void newCommandBuffer(ID3D12Device *pDevice, D3D12_COMMAND_LIST_TYPE type);
        void deleteCommandBuffer();

        void execute(ID3D12CommandQueue *pQueue);

        ID3D12CommandAllocator *pAllocator = nullptr;
        ID3D12GraphicsCommandList *pCommandList = nullptr;

        Fence fence;
    };

    struct Context {
        constexpr static inline size_t kDefaultAdapter = SIZE_MAX;

        struct Info {
            size_t adapter = kDefaultAdapter; // which adapter to use
            size_t frames = 2; // number of back buffers
        
            size_t heapSize = 1024;
            size_t queueSize = 1024;
            size_t workerThreads = 2;
        };

        Context(system::Window& window, const Info& info);
        ~Context();

        void update(const Info& info);

        void beginRender();
        void endRender();

        void beginCopy();
        void endCopy();

        void present();

        ID3D12Device *getDevice() const { return pDevice; }
        IDXGISwapChain *getSwapChain() const { return pSwapChain; }
        
        ID3D12GraphicsCommandList *getDirectCommands() const { return pDirectCommandList; }
        ID3D12GraphicsCommandList *getCopyCommands() const { return pCopyCommandList; }

        size_t getFrames() const { return info.frames; }
        size_t getCurrentFrame() const { return frameIndex; }

        Heap& getCbvHeap() { return cbvHeap; }
        Heap& getRtvHeap() { return rtvHeap; }

        ID3D12Resource *newBuffer(
            size_t size, 
            const D3D12_HEAP_PROPERTIES *pProps, 
            D3D12_RESOURCE_STATES state,
            D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED
        );

        CommandBuffer newCommandBuffer(D3D12_COMMAND_LIST_TYPE type);
        
        void submitDirectCommands(CommandBuffer& buffer);
        void submitCopyCommands(CommandBuffer& buffer);

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

        void newDirectQueue();
        void deleteDirectQueue();

        void newCopyQueue();
        void deleteCopyQueue();

        void newSwapChain();
        void deleteSwapChain();

        void newRenderTargets();
        void deleteRenderTargets();

        void newFence();
        void deleteFence();

        void newDescriptorHeap();
        void deleteDescriptorHeap();

        void waitForFence();
        void nextFrame();

        constexpr size_t getRenderHeapSize() const { return info.frames + 1; }

        using DoOnceGroup = util::DoOnceGroup<D3D12_MESSAGE_ID>;

        system::Window& window;
        Info info;

        DoOnceGroup reportOnce;

        ID3D12Debug* pDebug = nullptr;

        IDXGIFactory6 *pFactory = nullptr;

        IDXGIAdapter1 *pAdapter = nullptr;

        ID3D12Device *pDevice = nullptr;

        ID3D12InfoQueue1 *pInfoQueue = nullptr;

        BOOL bTearingSupported = FALSE;
        IDXGISwapChain3 *pSwapChain = nullptr;

        ID3D12CommandQueue *pDirectQueue = nullptr;
        std::vector<ID3D12CommandAllocator*> directCommandAllocators;
        ID3D12GraphicsCommandList *pDirectCommandList = nullptr;

        ID3D12CommandQueue *pCopyQueue = nullptr;
        ID3D12CommandAllocator* pCopyAllocator = nullptr;
        ID3D12GraphicsCommandList *pCopyCommandList = nullptr;

        UINT frameIndex = 0;

        Fence presentFence;
        Fence copyFence;

        Heap rtvHeap;
        Heap cbvHeap;
    };
}
