#pragma once

#include "commands.h"
#include "root.h"
#include "queue.h"
#include "pipeline.h"
#include "library.h"
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

            rootVersion = highestRootVersion();
        }

        Queue newCommandQueue(std::wstring_view name, const D3D12_COMMAND_QUEUE_DESC& desc) {
            Queue queue(Super::get(), desc);
            queue.rename(name);
            return queue;
        }

        Object<ID3D12CommandAllocator> newAllocator(std::wstring_view name, D3D12_COMMAND_LIST_TYPE type) {
            Object<ID3D12CommandAllocator> allocator;
            check(Super::get()->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)), "failed to create command allocator");
            allocator.rename(name);
            return allocator;
        }

        DescriptorHeap newHeap(std::wstring_view name, const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
            auto heap = DescriptorHeap(Super::get(), desc);
            heap.rename(name);
            return heap;
        }

        Commands newCommandList(std::wstring_view name, Object<ID3D12CommandAllocator> allocator, PipelineState state, D3D12_COMMAND_LIST_TYPE type) {
            Commands commands(Super::get(), type, allocator.get(), state);
            commands.rename(name);
            return commands;
        }

        Bundle newCommandBundle(std::wstring_view name, Object<ID3D12CommandAllocator> allocator, PipelineState state) {
            Bundle bundle(Super::get(), allocator.get(), state);
            bundle.rename(name);
            return bundle;
        }

        RootSignature newRootSignature(std::wstring_view name, root::Create create) {
            auto blob = root::compile(rootVersion, create);
            RootSignature root(Super::get(), blob);
            root.rename(name);
            return root;
        }

        PipelineState newGraphicsPSO(std::wstring_view name, ShaderLibrary shaders, RootSignature root) {
            PipelineState state(Super::get(), shaders, root);
            state.rename(name);
            return state;
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
