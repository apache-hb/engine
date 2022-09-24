#pragma once

#include "common.h"

namespace engine {
    struct DxAllocator final : rhi::Allocator {
        DxAllocator(ID3D12CommandAllocator *allocator);
        
        void reset();
        ID3D12CommandAllocator *get();
    private:
        UniqueComPtr<ID3D12CommandAllocator> allocator;
    };
}
