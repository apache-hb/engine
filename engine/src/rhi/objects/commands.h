#pragma once

#include "common.h"

namespace engine {
    struct DxCommandList : rhi::CommandList {
        DxCommandList(ID3D12GraphicsCommandList *commands);
        void beginRecording(rhi::Allocator &allocator) override;
        
        void endRecording() override;
        void setRenderTarget(rhi::CpuHandle target, const math::float4 &colour) override;

        void setViewport(const rhi::Viewport &view) override;

        void copyBuffer(rhi::Buffer &dst, rhi::Buffer &src, size_t size) override;
        void transition(rhi::Buffer &buffer, rhi::Buffer::State before, rhi::Buffer::State after) override;
        
        void bindDescriptors(std::span<ID3D12DescriptorHeap*> sets) override;
        void bindTable(size_t index, rhi::GpuHandle handle) override;

        void setPipeline(rhi::PipelineState *pipeline) override;

        void drawMesh(const rhi::IndexBufferView &indexView, const rhi::VertexBufferView &vertexView) override;

        void imguiRender() override;
        ID3D12GraphicsCommandList *get();

    private:
        void transitionBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

        rhi::UniqueComPtr<ID3D12GraphicsCommandList> commands;
    };
}
