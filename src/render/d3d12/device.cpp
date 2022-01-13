#include "render/objects/device.h"

#include <array>

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

DescriptorHeap Device::newHeap(DescriptorHeap::Type type, UINT size, DescriptorHeap::Flags flags) {
    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = type,
        .NumDescriptors = size,
        .Flags = flags,
        .NodeMask = 0
    };
    HRESULT hr = get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
    UINT increment = get()->GetDescriptorHandleIncrementSize(type);
    check(hr, "failed to create descriptor heap");
    return DescriptorHeap(heap, increment);
}

Resource Device::newCommittedResource(
    const D3D12_HEAP_PROPERTIES& props,
    D3D12_HEAP_FLAGS flags,
    const D3D12_RESOURCE_DESC& desc,
    D3D12_RESOURCE_STATES initial,
    const D3D12_CLEAR_VALUE* clear
)
{
    ID3D12Resource* resource = nullptr;
    HRESULT hr = get()->CreateCommittedResource(&props, flags, &desc, initial, clear, IID_PPV_ARGS(&resource));
    check(hr, "failed to create committed resource");
    return Resource(resource);
}

constexpr auto kFilterFlags = D3D12_MESSAGE_CALLBACK_FLAG_NONE;


constexpr auto kFilters = std::to_array({
    D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
});

constexpr D3D12_INFO_QUEUE_FILTER kFilter = {
    .DenyList = {
        .NumIDs = UINT(kFilters.size()),
        .pIDList = kFilters.data()
    }
};

DWORD Device::attachInfoQueue(InfoCallback callback, void* data) {
    auto infoQueue = Super::as<ID3D12InfoQueue1>();
    if (!infoQueue) { return UINT_MAX; }

    DWORD cookie = 0;
    HRESULT hrCallback = infoQueue->RegisterMessageCallback(callback, kFilterFlags, data, &cookie);
    if (FAILED(hrCallback)) { 
        log::render->warn("failed to register info queue callback");
        return UINT_MAX; 
    }

    HRESULT hrFilter = infoQueue->AddStorageFilterEntries(&kFilter);
    if (FAILED(hrFilter)) {
        log::render->warn("failed to add info queue filters");
        return UINT_MAX;
    }

    return cookie;
}
