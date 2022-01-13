#pragma once

#include "queue.h"
#include "pipeline.h"
#include "allocator.h"

namespace engine::render {
    struct Device : Object<ID3D12Device4> {
        OBJECT(Device, ID3D12Device4);

        Allocator newAllocator(CommandList::Type type);
        CommandQueue newCommandQueue(CommandList::Type type);
        CommandList newCommandList(CommandList::Type type, Allocator allocator, PipelineState pipeline = nullptr);
    };
}
