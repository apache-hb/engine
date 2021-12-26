#pragma once

#include "heap.h"

namespace engine::render {
    template<typename T>
    concept IsDevice = std::is_convertible_v<T*, ID3D12Device*>;

    template<IsDevice T>
    struct Device : Object<T> {
        using Super = Object<T>;

        Device() = default;

        Device(Object<ID3D12Device> device): Super(device.as<T>().get()) {
            if (!Super::valid()) { throw engine::Error("failed to create device"); }
        }

        Object<ID3D12CommandQueue> newCommandQueue(std::wstring_view name, const D3D12_COMMAND_QUEUE_DESC& desc) {
            Object<ID3D12CommandQueue> queue;
            check(Super::get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)), "failed to create command queue");
            queue.rename(name);
            return queue;
        }

        DescriptorHeap newHeap(std::wstring_view name, const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
            auto heap = DescriptorHeap(Super::get(), desc);
            heap.rename(name);
            return heap;
        }
    };
}
