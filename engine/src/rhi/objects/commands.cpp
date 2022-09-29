#include "engine/rhi/rhi.h"
#include "objects/common.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"

using namespace engine;
using namespace engine::rhi;

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


void CommandList::beginRecording(rhi::Allocator &allocator) {
    allocator.reset();

    DX_CHECK(get()->Reset(allocator.get(), nullptr));
}

void CommandList::endRecording() {
    DX_CHECK(get()->Close());
}

void CommandList::setRenderTarget(rhi::CpuHandle target, const math::float4 &colour) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kRtv { size_t(target) };
    const float kClear[] = { colour.x, colour.y, colour.z, colour.w };

    get()->OMSetRenderTargets(1, &kRtv, false, nullptr);
    get()->ClearRenderTargetView(kRtv, kClear, 0, nullptr);
}

void CommandList::setViewport(const rhi::Viewport &view) {
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

    get()->RSSetViewports(1, &kViewport);
    get()->RSSetScissorRects(1, &kScissor);
}

void CommandList::copyBuffer(rhi::Buffer &dst, rhi::Buffer &src, size_t size) {
    get()->CopyBufferRegion(dst.get(), 0, src.get(), 0, size);
}

void CommandList::transition(rhi::Buffer &buffer, rhi::Buffer::State before, rhi::Buffer::State after) {
    transitionBarrier(buffer.get(), D3D12_RESOURCE_STATES(before), D3D12_RESOURCE_STATES(after));
}

void CommandList::bindDescriptors(std::span<ID3D12DescriptorHeap*> sets) {
    get()->SetDescriptorHeaps(UINT(sets.size()), sets.data());
}

void CommandList::bindTable(size_t index, rhi::GpuHandle handle) {
    const D3D12_GPU_DESCRIPTOR_HANDLE kOffset { size_t(handle) };
    get()->SetGraphicsRootDescriptorTable(UINT(index), kOffset);
}

void CommandList::setPipeline(rhi::PipelineState &pipeline) {
    get()->SetGraphicsRootSignature(pipeline.signature.get());
    get()->SetPipelineState(pipeline.pipeline.get());
}

void CommandList::drawMesh(const rhi::IndexBufferView &indexView, const rhi::VertexBufferView &vertexView) {
    const D3D12_VERTEX_BUFFER_VIEW kVertexBufferView = {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(vertexView.buffer),
        .SizeInBytes = UINT(vertexView.size * vertexView.stride),
        .StrideInBytes = UINT(vertexView.stride)
    };
    
    const D3D12_INDEX_BUFFER_VIEW kIndexBufferView = {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(indexView.buffer),
        .SizeInBytes = UINT(indexView.size * getElementSize(indexView.format)),
        .Format = getElementFormat(indexView.format)
    };

    get()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    get()->IASetVertexBuffers(0, 1, &kVertexBufferView);
    get()->IASetIndexBuffer(&kIndexBufferView);
    get()->DrawIndexedInstanced(UINT(indexView.size), 1, 0, 0, 0);
}

void CommandList::transitionBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    const auto kBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);

    get()->ResourceBarrier(1, &kBarrier);
}

void CommandList::imguiRender() {
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), get());
}