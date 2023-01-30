#include "simcoe/render/heap.h"

using namespace simcoe;
using namespace simcoe::render;

Heap::Heap(size_t size) 
    : map(size)
{ }

void Heap::newHeap(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = type,
        .NumDescriptors = UINT(map.getSize()),
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    };

    HR_CHECK(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap)));
    descriptorSize = pDevice->GetDescriptorHandleIncrementSize(type);
}

void Heap::deleteHeap() {
    RELEASE(pHeap);
}

Heap::Index Heap::alloc() {
    return map.alloc();
}

void Heap::release(Index index) {
    map.release(index);
}

Heap::CpuHandle Heap::cpuHandle(Index index) const {
    ASSERT(index != Index::eInvalid);
    ASSERT(map.test(index));

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(index), descriptorSize);
}

Heap::GpuHandle Heap::gpuHandle(Index index) const {
    ASSERT(index != Index::eInvalid);
    ASSERT(map.test(index));

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(pHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<INT>(index), descriptorSize);
}
