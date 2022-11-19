#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

Fence::Fence(ID3D12Fence *fence)
    : Super(fence)
    , event(CreateEvent(nullptr, false, false, nullptr))
{ }

void Fence::wait(size_t signal) {
    if (get()->GetCompletedValue() < signal) {
        HR_CHECK(get()->SetEventOnCompletion(signal, event.get()));
        WaitForSingleObject(event.get(), INFINITE);
    }
}
