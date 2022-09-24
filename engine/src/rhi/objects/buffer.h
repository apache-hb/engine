#pragma once

#include "common.h"

namespace engine {
    struct DxBuffer : rhi::Buffer {
        DxBuffer(ID3D12Resource *resource);

        void write(const void *src, size_t size) override;
        
        rhi::GpuHandle gpuAddress() override;

        ID3D12Resource *get();
    private:
        UniqueComPtr<ID3D12Resource> resource;
    };
}
