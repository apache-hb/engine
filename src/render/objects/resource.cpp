#include "resource.h"

using namespace engine::render;

void* Resource::map(UINT subresource, D3D12_RANGE* range) {
    void *data;
    check(Super::get()->Map(subresource, range, &data), "failed to map resource");
    return data;
}

void Resource::unmap(UINT subresource, D3D12_RANGE* range) {
    Super::get()->Unmap(subresource, range);
}

void Resource::writeBytes(UINT subresource, const void* data, size_t size) {
    D3D12_RANGE range = { 0, size };
    
    void *mapped = map(subresource);
    memcpy(mapped, data, size);
    unmap(subresource, &range);
}
