#pragma once

#include "render/util.h"

namespace engine::render {
    struct DescriptorHeap : Object<ID3D12DescriptorHeap> {
        DescriptorHeap() = default;
        DescriptorHeap(ID3D12Device *device, std::wstring_view name, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

        auto cpuHandle(size_t index) {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(get()->GetCPUDescriptorHandleForHeapStart(), UINT(index), descriptorSize);
        }

        auto gpuHandle(size_t index) {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(get()->GetGPUDescriptorHandleForHeapStart(), UINT(index), descriptorSize);
        }

    private:
        UINT descriptorSize;
    };
}