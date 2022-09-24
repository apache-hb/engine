#include "objects/fence.h"

using namespace engine;

DxFence::DxFence(ID3D12Fence *fence, HANDLE event)
    : fence(fence)
    , event(event)
{ }

DxFence::~DxFence() {
    CloseHandle(event);
}

void DxFence::waitUntil(size_t signal) {
    if (fence->GetCompletedValue() < signal) {
        DX_CHECK(fence->SetEventOnCompletion(signal, event));
        WaitForSingleObject(event, INFINITE);
    }
}

ID3D12Fence *DxFence::get() { 
    return fence.get(); 
}
