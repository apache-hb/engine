#include "render/objects/device.h"

using namespace engine::render;

Allocator Device::newAllocator(CommandList::Type type) {
    ID3D12CommandAllocator* allocator = nullptr;
    HRESULT hr = get()->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator));
    check(hr, "failed to create command allocator");
    return Allocator(allocator);
}

CommandQueue Device::newCommandQueue(CommandList::Type type) {
    D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = type,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0
    };
    ID3D12CommandQueue* queue = nullptr;
    HRESULT hr = get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue));
    check(hr, "failed to create command queue");
    return CommandQueue(queue);
}

CommandList Device::newCommandList(CommandList::Type type, Allocator allocator, PipelineState pipeline) {
    ID3D12GraphicsCommandList* list = nullptr;
    HRESULT hr = get()->CreateCommandList(0, type, allocator.get(), pipeline.get(), IID_PPV_ARGS(&list));
    check(hr, "failed to create command list");
    return CommandList(list);
}
