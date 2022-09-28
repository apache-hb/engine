#pragma once

#include "common.h"

namespace engine {
    struct DxBuffer final : rhi::Buffer {
        DxBuffer(ID3D12Resource *resource);

        void write(const void *src, size_t size) override;
        
        rhi::GpuHandle gpuAddress() override;

        void *map() override;
        void unmap() override;

        ID3D12Resource *get();
    private:
        rhi::UniqueComPtr<ID3D12Resource> resource;
    };
}
