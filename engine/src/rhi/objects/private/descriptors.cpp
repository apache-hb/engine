#include "objects/descriptors.h"

using namespace engine;

DxDescriptorSet::DxDescriptorSet(ID3D12DescriptorHeap *heap, UINT stride)
    : heap(heap)
    , stride(stride)
{ }

rhi::CpuHandle DxDescriptorSet::cpuHandle(size_t offset) {
    const auto kCpuHeapStart = heap->GetCPUDescriptorHandleForHeapStart();
    return rhi::CpuHandle(kCpuHeapStart.ptr + (offset * stride));
}

rhi::GpuHandle DxDescriptorSet::gpuHandle(size_t offset) {
    const auto kGpuHeapStart = heap->GetGPUDescriptorHandleForHeapStart();
    return rhi::GpuHandle(kGpuHeapStart.ptr + (offset * stride));
}

ID3D12DescriptorHeap *DxDescriptorSet::get() { 
    return heap.get(); 
}
