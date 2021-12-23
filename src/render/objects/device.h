#pragma once

#include "render/debug/debug.h"

#include "heap.h"
#include "library.h"
#include "compute.h"
#include "bundle.h"

namespace engine::render {
    template<typename T>
    struct Device : Object<T> {
        using Super = Object<T>;
        using Super::Super;

        Device() = default;
        Device(T* device) 
            : Super(device)
            , rootVersion(highestRootVersion())
        { }

        DescriptorHeap newHeap(std::wstring_view name, const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
            return DescriptorHeap(Super::get(), name, desc);
        }

        Object<ID3D12CommandQueue> newCommandQueue(std::wstring_view name, const D3D12_COMMAND_QUEUE_DESC& desc) {
            Object<ID3D12CommandQueue> queue;
            check(Super::get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)), "failed to create command queue");
            queue.rename(name);
            return queue;
        }

        Object<ID3D12GraphicsCommandList> newCommandList(std::wstring_view name, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc, ID3D12PipelineState* pipeline = nullptr) {
            Object<ID3D12GraphicsCommandList> list;
            check(Super::get()->CreateCommandList(0, type, alloc, pipeline, IID_PPV_ARGS(&list)), "failed to create command list");
            list.rename(name);
            return list;
        }

        template<typename F>
        CommandBundle newCommandBundle(std::wstring_view name, ID3D12PipelineState* state, F&& func) {
            /// create an allocator for just this list
            Object<ID3D12CommandAllocator> allocator;
            check(Super::get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&allocator)), "failed to create command allocator");
            
            /// create the actual bundle
            auto list = newCommandList(name, D3D12_COMMAND_LIST_TYPE_BUNDLE, allocator.get(), state);
            /// record all its instructions
            func(list);
            /// close the list
            check(list->Close(), "failed to close command list");

            return CommandBundle(list, allocator);
        }

        Object<ID3D12RootSignature> newRootSignature(std::wstring_view name, Com<ID3DBlob> signature) {
            Object<ID3D12RootSignature> result;
            check(Super::get()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&result)), "failed to create root signature");
            result.rename(name);
            return result;
        }

        Object<ID3D12RootSignature> compileRootSignature(std::wstring_view name, D3D_ROOT_SIGNATURE_VERSION version, const RootCreate& create) {
            return newRootSignature(name, render::compileRootSignature(version, create));
        }

        Object<ID3D12PipelineState> newGraphicsPSO(std::wstring_view name, ShaderLibrary& library, ID3D12RootSignature* root) {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
                .pRootSignature = root,
                .VS = library.vertex(),
                .PS = library.pixel(),
                .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
                .SampleMask = UINT_MAX,
                .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
                .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
                .InputLayout = library.layout(),
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
                .DSVFormat = DXGI_FORMAT_D32_FLOAT,
                .SampleDesc = { 1 }
            };

            Object<ID3D12PipelineState> result;

            check(Super::get()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&result)), "failed to create graphics pipeline state");
            result.rename(name);

            return result;
        }

        Object<ID3D12PipelineState> newComputePSO(std::wstring_view name, ComputeLibrary& library, ID3D12RootSignature* root) {
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
                .pRootSignature = root,
                .CS = library.shader()
            };

            Object<ID3D12PipelineState> result;

            check(Super::get()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&result)), "failed to create compute pipeline state");
            result.rename(name);

            return result;
        }

        D3D_ROOT_SIGNATURE_VERSION getHighestRootVersion() const { return rootVersion; }

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
