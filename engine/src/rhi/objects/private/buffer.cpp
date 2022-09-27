#include "objects/buffer.h"

using namespace engine;

DxBuffer::DxBuffer(ID3D12Resource *resource)
    : resource(resource)
{ }

void DxBuffer::write(const void *src, size_t size) {
    void *ptr;
    CD3DX12_RANGE range(0, 0);

    DX_CHECK(resource->Map(0, &range, &ptr));
    memcpy(ptr, src, size);
    resource->Unmap(0, &range);
}

void *DxBuffer::map() {
    void *ptr;
    CD3DX12_RANGE range(0, 0);
    DX_CHECK(resource->Map(0, &range, &ptr));
    return ptr;
}

void DxBuffer::unmap() {
    CD3DX12_RANGE range(0, 0);
    resource->Unmap(0, &range);
}


rhi::GpuHandle DxBuffer::gpuAddress() {
    return rhi::GpuHandle(resource->GetGPUVirtualAddress());
}

ID3D12Resource *DxBuffer::get() { 
    return resource.get();
}
