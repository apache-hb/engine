#pragma once

#include "heap.h"
#include "resource.h"

namespace engine::render {
    struct RootCreate {
        std::span<const CD3DX12_ROOT_PARAMETER1> params;
        std::span<const D3D12_STATIC_SAMPLER_DESC> samplers;
        D3D12_ROOT_SIGNATURE_FLAGS flags;
    };

    template<typename T>
    concept IsDevice = std::is_convertible_v<T*, ID3D12Device*>;

    template<IsDevice T>
    struct Device : Object<T> {
        using Super = Object<T>;
        using Super::Super;

        Device(Object<ID3D12Device> device): Super(device.as<T>().get()) {
            if (!Super::valid()) { throw engine::Error("failed to create device"); }
            rootVersion = highestRootVersion();
        }

        Object<ID3D12CommandQueue> newCommandQueue(std::wstring_view name, const D3D12_COMMAND_QUEUE_DESC& desc) {
            ID3D12CommandQueue* queue = nullptr;
            check(Super::get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)), "failed to create command queue");
            return { queue, name };
        }

        Object<ID3D12CommandAllocator> newCommandAllocator(std::wstring_view name, D3D12_COMMAND_LIST_TYPE type) {
            ID3D12CommandAllocator* allocator = nullptr;
            check(Super::get()->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)), "failed to create command allocator");
            return { allocator, name };
        }

        Object<ID3D12GraphicsCommandList> newCommandList(std::wstring_view name, D3D12_COMMAND_LIST_TYPE type, Object<ID3D12CommandAllocator> allocator) {
            ID3D12GraphicsCommandList* list = nullptr;
            check(Super::get()->CreateCommandList(0, type, allocator.get(), nullptr, IID_PPV_ARGS(&list)), "failed to create command list");
            return { list, name };
        }

        DescriptorHeap newDescriptorHeap(std::wstring_view name, const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
            ID3D12DescriptorHeap* heap = nullptr;
            check(Super::get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)), "failed to create descriptor heap");
            auto incrementSize = Super::get()->GetDescriptorHandleIncrementSize(desc.Type);
            return { heap, incrementSize, name };
        }

        Resource newCommittedResource(
            std::wstring_view name, const D3D12_HEAP_PROPERTIES& heapProps, 
            D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC& desc, 
            D3D12_RESOURCE_STATES initial, const D3D12_CLEAR_VALUE* clearValue = nullptr) {
            ID3D12Resource* resource = nullptr;
            
            HRESULT hr = Super::get()->CreateCommittedResource(
                &heapProps, flags,
                &desc, initial, clearValue,
                IID_PPV_ARGS(&resource)
            );
            check(hr, "failed to create committed resource");

            return { resource, name };
        }

        Object<ID3D12Fence> newFence(std::wstring_view name, UINT64 initial, D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE) {
            ID3D12Fence* fence = nullptr;
            check(Super::get()->CreateFence(initial, flags, IID_PPV_ARGS(&fence)), "failed to create fence");
            return { fence, name };
        }

    private:
        D3D_ROOT_SIGNATURE_VERSION highestRootVersion() {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE features = { .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1 };

            if (FAILED(Super::get()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &features, sizeof(features)))) {
                features.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            return features.HighestVersion;
        }

        D3D_ROOT_SIGNATURE_VERSION rootVersion;
    };
}
