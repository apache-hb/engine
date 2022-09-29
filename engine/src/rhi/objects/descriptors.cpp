#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace engine;
using namespace engine::rhi;

DescriptorSet::DescriptorSet(ID3D12DescriptorHeap *heap, UINT stride)
    : Super(heap)
    , stride(stride)
{ }

rhi::CpuHandle DescriptorSet::cpuHandle(size_t offset) {
    const auto kCpuHeapStart = get()->GetCPUDescriptorHandleForHeapStart();
    return rhi::CpuHandle(kCpuHeapStart.ptr + (offset * stride));
}

rhi::GpuHandle DescriptorSet::gpuHandle(size_t offset) {
    const auto kGpuHeapStart = get()->GetGPUDescriptorHandleForHeapStart();
    return rhi::GpuHandle(kGpuHeapStart.ptr + (offset * stride));
}
