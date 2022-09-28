#pragma once

#include "common.h"

namespace engine {
    struct DxDevice final : rhi::Device {
        DxDevice(ID3D12Device *device, D3D_ROOT_SIGNATURE_VERSION version);
        ~DxDevice() override;


        rhi::Fence *newFence() override;
        rhi::CommandQueue *newQueue(rhi::CommandList::Type type) override;

        rhi::CommandList *newCommandList(rhi::Allocator *allocator, rhi::CommandList::Type type) override;
        
        rhi::Allocator *newAllocator(rhi::CommandList::Type type) override;
        rhi::DescriptorSet *newDescriptorSet(size_t count, rhi::DescriptorSet::Type type, bool shaderVisible) override;
        
        void createRenderTargetView(rhi::Buffer *target, rhi::CpuHandle rtvHandle) override;
        void createConstBufferView(rhi::Buffer *buffer, size_t size, rhi::CpuHandle srvHandle) override;
        void createDepthStencilView(rhi::Buffer *buffer, rhi::CpuHandle handle) override;

        rhi::Buffer *newBuffer(size_t size, rhi::DescriptorSet::Visibility visibility, rhi::Buffer::State state) override;

        rhi::PipelineState *newPipelineState(const rhi::PipelineBinding& bindings) override;

        void imguiInit(size_t frames, rhi::DescriptorSet *heap, rhi::CpuHandle cpuHandle, rhi::GpuHandle gpuHandle) override;
        void imguiNewFrame() override;

    private:
        ID3D12RootSignature *createRootSignature(ID3DBlob *signature);

        ID3D12PipelineState *createPipelineState(ID3D12RootSignature *root, D3D12_SHADER_BYTECODE vertex, D3D12_SHADER_BYTECODE pixel, std::span<D3D12_INPUT_ELEMENT_DESC> input);

        UniqueComPtr<ID3D12Device> device;
        const D3D_ROOT_SIGNATURE_VERSION kHighestVersion;
    };
}
