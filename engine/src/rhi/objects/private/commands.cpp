#include "objects/commands.h"

#include "objects/allocator.h"
#include "objects/buffer.h"
#include "objects/pipeline.h"
#include "objects/descriptors.h"

using namespace engine;

namespace {
    constexpr size_t getElementSize(rhi::Format format) {
        switch (format) {
        case rhi::Format::uint32: return sizeof(uint32_t);
        
        case rhi::Format::float32: return sizeof(float);
        case rhi::Format::float32x2: return sizeof(math::float2);
        case rhi::Format::float32x3: return sizeof(math::float3);
        case rhi::Format::float32x4: return sizeof(math::float4);
        
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }
}

DxCommandList::DxCommandList(ID3D12GraphicsCommandList *commands)
    : commands(commands)
{ }

void DxCommandList::beginRecording(rhi::Allocator *allocator) {
    auto *alloc = static_cast<DxAllocator*>(allocator);
    alloc->reset();

    DX_CHECK(commands->Reset(alloc->get(), nullptr));
}

void DxCommandList::endRecording() {
    DX_CHECK(commands->Close());
}

void DxCommandList::setRenderTarget(rhi::CpuHandle target, const math::float4 &colour) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kRtv { size_t(target) };
    const float kClear[] = { colour.x, colour.y, colour.z, colour.w };

    commands->OMSetRenderTargets(1, &kRtv, false, nullptr);
    commands->ClearRenderTargetView(kRtv, kClear, 0, nullptr);
}

void DxCommandList::setViewport(const rhi::Viewport &view) {
    const D3D12_VIEWPORT kViewport = {
        .TopLeftX = 0.f,
        .TopLeftY = 0.f,
        .Width = view.width,
        .Height = view.height,
        .MinDepth = D3D12_MIN_DEPTH,
        .MaxDepth = D3D12_MAX_DEPTH
    };

    const D3D12_RECT kScissor = {
        .left = 0,
        .top = 0,
        .right = LONG(view.width),
        .bottom = LONG(view.height)
    };

    commands->RSSetViewports(1, &kViewport);
    commands->RSSetScissorRects(1, &kScissor);
}

void DxCommandList::copyBuffer(rhi::Buffer *dst, rhi::Buffer *src, size_t size) {
    auto *input = static_cast<DxBuffer*>(src);
    auto *output = static_cast<DxBuffer*>(dst);

    commands->CopyBufferRegion(output->get(), 0, input->get(), 0, size);
}

void DxCommandList::transition(rhi::Buffer *buffer, rhi::Buffer::State before, rhi::Buffer::State after) {
    auto *resource = static_cast<DxBuffer*>(buffer);
    
    transitionBarrier(resource->get(), getResourceState(before), getResourceState(after));
}

void DxCommandList::bindDescriptors(std::span<rhi::DescriptorSet*> sets) {
    UniquePtr<ID3D12DescriptorHeap*[]> heaps(sets.size());
    for (size_t i = 0; i < sets.size(); i++) {
        auto *set = static_cast<DxDescriptorSet*>(sets[i]);
        heaps[i] = set->get();
    }

    commands->SetDescriptorHeaps(UINT(sets.size()), heaps.get());
}

void DxCommandList::bindTable(size_t index, rhi::GpuHandle handle) {
    const D3D12_GPU_DESCRIPTOR_HANDLE kOffset { size_t(handle) };
    commands->SetGraphicsRootDescriptorTable(UINT(index), kOffset);
}

void DxCommandList::setPipeline(rhi::PipelineState *pipeline) {
    auto *pso = static_cast<DxPipelineState*>(pipeline);
    commands->SetGraphicsRootSignature(pso->getSignature());
    commands->SetPipelineState(pso->getPipelineState());
}

void DxCommandList::drawMesh(const rhi::IndexBufferView &indexView, const rhi::VertexBufferView &vertexView) {
    const D3D12_VERTEX_BUFFER_VIEW kVertexBufferView = {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(vertexView.buffer->gpuAddress()),
        .SizeInBytes = UINT(vertexView.size * vertexView.stride),
        .StrideInBytes = UINT(vertexView.stride)
    };
    
    const D3D12_INDEX_BUFFER_VIEW kIndexBufferView = {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(indexView.buffer->gpuAddress()),
        .SizeInBytes = UINT(indexView.size * getElementSize(indexView.format)),
        .Format = getElementFormat(indexView.format)
    };

    commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commands->IASetVertexBuffers(0, 1, &kVertexBufferView);
    commands->IASetIndexBuffer(&kIndexBufferView);
    commands->DrawIndexedInstanced(UINT(indexView.size), 1, 0, 0, 0);
}

ID3D12GraphicsCommandList *DxCommandList::get() { 
    return commands.get(); 
}

void DxCommandList::transitionBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    const auto kBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);

    commands->ResourceBarrier(1, &kBarrier);
}
