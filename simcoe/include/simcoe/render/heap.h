#pragma once

#include "simcoe/render/render.h"
#include "simcoe/memory/bitmap.h"

namespace simcoe::render {
    struct Heap final {
        using Index = memory::AtomicBitMap::Index;
        using CpuHandle = D3D12_CPU_DESCRIPTOR_HANDLE;
        using GpuHandle = D3D12_GPU_DESCRIPTOR_HANDLE;

        Heap(size_t size);

        void newHeap(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_FLAGS flags, D3D12_DESCRIPTOR_HEAP_TYPE type);
        void deleteHeap();

        Index alloc();
        void release(Index index);

        CpuHandle cpuHandle(Index index) const;
        GpuHandle gpuHandle(Index index) const;

        ID3D12DescriptorHeap *getHeap() const { return pHeap; }

    private:
        memory::AtomicBitMap map;
        
        ID3D12DescriptorHeap *pHeap = nullptr;
        UINT descriptorSize = 0;
    };
}
