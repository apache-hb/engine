#include "simcoe/render/queue.h"
#include "dx/d3d12.h"

using namespace simcoe;
using namespace simcoe::render;

Queue::Queue(size_t size)
    : queue(size)
{ }

void Queue::newQueue(ID3D12Device *pDevice) {
    D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COPY
    };

    HR_CHECK(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pQueue)));
    HR_CHECK(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&pAllocator)));
    HR_CHECK(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, pAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));

    HR_CHECK(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    ASSERT(fenceEvent != nullptr);
}

void Queue::deleteQueue() {
    CloseHandle(fenceEvent);
    RELEASE(pFence);

    RELEASE(pCommandList);
    RELEASE(pAllocator);
    RELEASE(pQueue);
}

void Queue::wait() {
    HR_CHECK(pQueue->Signal(pFence, fenceValue));
    
    HR_CHECK(pFence->SetEventOnCompletion(fenceValue, fenceEvent));
    WaitForSingleObject(fenceEvent, INFINITE);
    
    fenceValue += 1;
}

void Queue::push(void *pCommand) {
    queue.try_enqueue(pCommand);
}
