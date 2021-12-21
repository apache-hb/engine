#pragma once

#include "heap.h"
#include "library.h"

namespace engine::render {
    template<typename T>
    struct Device : Com<T> {
        using Super = Com<T>;
        using Super::Super;

        DescriptorHeap newHeap(std::wstring_view name, const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
            return DescriptorHeap(Super::get(), name, desc);
        }

        Com<ID3D12CommandQueue> newCommandQueue(std::wstring_view name, const D3D12_COMMAND_QUEUE_DESC& desc) {
            Com<ID3D12CommandQueue> queue;
            check(Super::get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)), "failed to create command queue");
            queue.rename(name);
            return queue;
        }

        Com<ID3D12GraphicsCommandList> newCommandList(std::wstring_view name, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc, ID3D12PipelineState* pipeline = nullptr) {
            Com<ID3D12GraphicsCommandList> list;
            check(Super::get()->CreateCommandList(0, type, alloc, pipeline, IID_PPV_ARGS(&list)), "failed to create command list");
            list.rename(name);
            return list;
        }

        Com<ID3D12RootSignature> newRootSignature(std::wstring_view name, Com<ID3DBlob> signature) {
            Com<ID3D12RootSignature> result;
            check(Super::get()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&result)), "failed to create root signature");
            result.rename(name);
            return result;
        }

        Com<ID3D12PipelineState> newPipelineState(std::wstring_view name, ShaderLibrary& library, ID3D12RootSignature* root) {
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

            Com<ID3D12PipelineState> result;

            check(Super::get()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&result)), "failed to create graphics pipeline state");
            result.rename(name);

            return result;
        }
    };
}
