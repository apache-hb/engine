#include "resource.h"

using namespace engine::render;

D3D12_GPU_VIRTUAL_ADDRESS Resource::gpuAddress() {
    return get()->GetGPUVirtualAddress();
}

void* Resource::mapBytes(UINT subresource, D3D12_RANGE* range) {
    void *data;
    check(Super::get()->Map(subresource, range, &data), "failed to map resource");
    return data;
}

void Resource::unmap(UINT subresource, D3D12_RANGE* range) {
    Super::get()->Unmap(subresource, range);
}

void Resource::writeBytes(UINT subresource, const void* data, size_t size) {
    D3D12_RANGE range = { 0, size };
    
    void *mapped = mapBytes(subresource);
    memcpy(mapped, data, size);
    unmap(subresource, &range);
}
