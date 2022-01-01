#pragma once

#include "render/util.h"

namespace engine::render {
    struct DescriptorHeap : Object<ID3D12DescriptorHeap> {
        using Super = Object<ID3D12DescriptorHeap>;
        using Super::Super;

        DescriptorHeap(ID3D12DescriptorHeap* heap, UINT incrementSize, std::wstring_view name);
    
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(UINT index);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(UINT index);
    private:
        UINT incrementSize = UINT_MAX;
    };
}
