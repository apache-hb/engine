#pragma once

#include "common.h"

namespace engine {
    struct DxDescriptorSet final : rhi::DescriptorSet {
        DxDescriptorSet(ID3D12DescriptorHeap *heap, UINT stride);

        rhi::CpuHandle cpuHandle(size_t offset) override;
        rhi::GpuHandle gpuHandle(size_t offset) override;

        ID3D12DescriptorHeap *get();

    private:
        UniqueComPtr<ID3D12DescriptorHeap> heap;
        UINT stride;
    };
}
