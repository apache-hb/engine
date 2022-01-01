#pragma once

#include "render/util.h"

namespace engine::render {
    struct DescriptorHeap : Object<ID3D12DescriptorHeap> {
        DescriptorHeap() = default;
        DescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(size_t offset);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(size_t offset);
    private:
        UINT descriptorSize;
    };
}
