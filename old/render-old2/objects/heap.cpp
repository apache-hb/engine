#include "heap.h"

namespace engine::render {
    DescriptorHeap::DescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
        check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(addr())), "failed to create descriptor heap");
        descriptorSize = device->GetDescriptorHandleIncrementSize(desc.Type);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::cpuHandle(size_t offset) {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(get()->GetCPUDescriptorHandleForHeapStart(), UINT(offset), descriptorSize);
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::gpuHandle(size_t offset) {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(get()->GetGPUDescriptorHandleForHeapStart(), UINT(offset), descriptorSize);
    }
}
