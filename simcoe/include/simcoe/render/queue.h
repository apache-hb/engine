#pragma once

#include "simcoe/render/render.h"
#include "simcoe/core/win32.h"

#include "concurrentqueue.h"

namespace simcoe::render {
    struct Queue {
        Queue(size_t size);

        void newQueue(ID3D12Device *pDevice);
        void deleteQueue();

        void wait();

        void push(void *pCommand);

    private:
        moodycamel::ConcurrentQueue<void*> queue;

        ID3D12CommandQueue *pQueue = nullptr;
        ID3D12CommandAllocator *pAllocator = nullptr;
        ID3D12GraphicsCommandList *pCommandList = nullptr;

        UINT64 fenceValue = 1;
        HANDLE fenceEvent = nullptr;
        ID3D12Fence *pFence = nullptr;
    };
}
