#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

void Buffer::write(const void *src, size_t size) {
    void *ptr = map();
    memcpy(ptr, src, size);
    unmap();
}

void *Buffer::map() {
    void *ptr;
    CD3DX12_RANGE range(0, 0);
    HR_CHECK(get()->Map(0, &range, &ptr));
    return ptr;
}

void Buffer::unmap() {
    CD3DX12_RANGE range(0, 0);
    get()->Unmap(0, &range);
}

rhi::GpuHandle Buffer::gpuAddress() {
    return rhi::GpuHandle(get()->GetGPUVirtualAddress());
}
