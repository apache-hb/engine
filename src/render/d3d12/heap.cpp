#include "render/objects/heap.h"

using namespace engine::render;

DescriptorHeap::CpuHandle DescriptorHeap::cpuHandle(UINT offset) {
    return CpuHandle(get()->GetCPUDescriptorHandleForHeapStart(), increment, offset);
}

DescriptorHeap::GpuHandle DescriptorHeap::gpuHandle(UINT offset) {
    return GpuHandle(get()->GetGPUDescriptorHandleForHeapStart(), increment, offset);
}
