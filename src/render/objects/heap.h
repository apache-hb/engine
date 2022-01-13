#pragma once

#include "render/util.h"

namespace engine::render {
    struct DescriptorHeap : Object<ID3D12DescriptorHeap> {
        OBJECT(DescriptorHeap, ID3D12DescriptorHeap);
    
        DescriptorHeap(ID3D12DescriptorHeap* self = nullptr, UINT increment = UINT_MAX)
            : Super(self)
            , increment(increment)
        { }

        using Type = D3D12_DESCRIPTOR_HEAP_TYPE;
        using Flags = D3D12_DESCRIPTOR_HEAP_FLAGS;

        using CpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE;
        using GpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE;

        static constexpr auto kCbvSrvUav = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        static constexpr auto kRtv = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        static constexpr auto kDsv = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        static constexpr auto kHidden = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        static constexpr auto kShaderVisible = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
        CpuHandle cpuHandle(UINT offset);
        GpuHandle gpuHandle(UINT offset);

    private:
        UINT increment;
    };
}
