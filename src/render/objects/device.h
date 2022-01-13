#pragma once

#include "queue.h"
#include "pipeline.h"
#include "allocator.h"
#include "heap.h"
#include "resource.h"

namespace engine::render {
    struct Device : Object<ID3D12Device4> {
        OBJECT(Device, ID3D12Device4);

        using InfoCallback = D3D12MessageFunc;

        Allocator newAllocator(CommandList::Type type);
        CommandQueue newCommandQueue(CommandList::Type type);
        CommandList newCommandList(CommandList::Type type, Allocator allocator, PipelineState pipeline = nullptr);
        DescriptorHeap newHeap(DescriptorHeap::Type type, UINT size, DescriptorHeap::Flags flags = DescriptorHeap::kHidden);

        Resource newCommittedResource(
            const D3D12_HEAP_PROPERTIES& props,
            D3D12_HEAP_FLAGS flags,
            const D3D12_RESOURCE_DESC& desc,
            D3D12_RESOURCE_STATES initial,
            const D3D12_CLEAR_VALUE* clear = nullptr
        );

        DWORD attachInfoQueue(InfoCallback callback, void* data = nullptr);
    };
}
