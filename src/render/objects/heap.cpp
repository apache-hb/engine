#include "heap.h"

using namespace engine::render;

DescriptorHeap::DescriptorHeap(ID3D12DescriptorHeap* heap, UINT incrementSize, std::wstring_view name) 
    : Super(heap, name)
    , incrementSize(incrementSize)
{ }

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::cpuHandle(UINT index) {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(get()->GetCPUDescriptorHandleForHeapStart(), index, incrementSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::gpuHandle(UINT index) {
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(get()->GetGPUDescriptorHandleForHeapStart(), index, incrementSize);
}
