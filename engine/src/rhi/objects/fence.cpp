#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace engine;
using namespace engine::rhi;

Fence::Fence(ID3D12Fence *fence)
    : Super(fence)
    , event(CreateEvent(nullptr, false, false, nullptr))
{ }

void Fence::waitUntil(size_t signal) {
    if (get()->GetCompletedValue() < signal) {
        DX_CHECK(get()->SetEventOnCompletion(signal, event.get()));
        WaitForSingleObject(event.get(), INFINITE);
    }
}
