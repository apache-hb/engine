#include "engine/rhi/rhi.h"
#include "objects/common.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"

using namespace simcoe;
using namespace simcoe::rhi;

namespace {
    constexpr size_t getElementSize(rhi::Format format) {
        switch (format) {
        case rhi::Format::uint32: return sizeof(uint32_t);
        
        case rhi::Format::float32: return sizeof(float);
        case rhi::Format::float32x2: return sizeof(math::float2);
        case rhi::Format::float32x3: return sizeof(math::float3);
        case rhi::Format::float32x4: return sizeof(math::float4);
        
        default: return SIZE_MAX;
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

void CommandList::setRenderTarget(rhi::CpuHandle rtv, rhi::CpuHandle dsv, const math::float4 &colour) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kRtv { size_t(rtv) };
    const D3D12_CPU_DESCRIPTOR_HANDLE kDsv { size_t(dsv) };
    
    const float kClear[] = { colour.x, colour.y, colour.z, colour.w };

    get()->OMSetRenderTargets(1, &kRtv, false, dsv == CpuHandle::Invalid ? nullptr : &kDsv);
    get()->ClearRenderTargetView(kRtv, kClear, 0, nullptr);
    
    if (dsv != CpuHandle::Invalid) {
        get()->ClearDepthStencilView(kDsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    }
}

void CommandList::clearRenderTarget(CpuHandle rtv, const math::float4 &colour) {
    const D3D12_CPU_DESCRIPTOR_HANDLE kRtv { size_t(rtv) };
    
    const float kClear[] = { colour.x, colour.y, colour.z, colour.w };

    get()->ClearRenderTargetView(kRtv, kClear, 0, nullptr);
}

void CommandList::setViewAndScissor(const rhi::View &view) {
    get()->RSSetViewports(1, &view.viewport);
    get()->RSSetScissorRects(1, &view.scissor);
}

void CommandList::copyBuffer(Buffer &dst, Buffer &src, size_t size) {
    get()->CopyBufferRegion(dst.get(), 0, src.get(), 0, size);
}

void CommandList::copyTexture(Buffer &dst, Buffer &src, const void *ptr, TextureSize size) {
    const D3D12_SUBRESOURCE_DATA kUpdate {
        .pData = ptr,
        .RowPitch = LONG_PTR(size.width * 4),
        .SlicePitch = LONG_PTR(size.width * 4 * size.height)
    };

    UpdateSubresources(get(), dst.get(), src.get(), 0, 0, 1, &kUpdate);
}

void CommandList::transition(std::span<const StateTransition> barriers) {
    ASSERT(barriers.size() > 0);

    get()->ResourceBarrier(UINT(barriers.size()), barriers.data());
}

void CommandList::bindDescriptors(std::span<ID3D12DescriptorHeap*> sets) {
    get()->SetDescriptorHeaps(UINT(sets.size()), sets.data());
}

void CommandList::bindTable(size_t index, rhi::GpuHandle handle) {
    const D3D12_GPU_DESCRIPTOR_HANDLE kOffset { size_t(handle) };
    get()->SetGraphicsRootDescriptorTable(UINT(index), kOffset);
}

void CommandList::bindConst(size_t index, size_t offset, uint32_t value) {
    get()->SetGraphicsRoot32BitConstant(UINT(index), value, UINT(offset));
}

void CommandList::setPipeline(rhi::PipelineState &pipeline) {
    get()->SetGraphicsRootSignature(pipeline.signature.get());
    get()->SetPipelineState(pipeline.pipeline.get());
}

void CommandList::setVertexBuffers(std::span<const rhi::VertexBufferView> buffers) {
    get()->IASetVertexBuffers(0, UINT(buffers.size()), buffers.data());
}

void CommandList::drawMesh(const rhi::IndexBufferView &indexView) {
    const D3D12_INDEX_BUFFER_VIEW kIndexBufferView = {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS(indexView.buffer),
        .SizeInBytes = UINT(indexView.length * getElementSize(indexView.format)),
        .Format = getElementFormat(indexView.format)
    };

    get()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    get()->IASetIndexBuffer(&kIndexBufferView);
    get()->DrawIndexedInstanced(UINT(indexView.length), 1, 0, 0, 0);
}

void CommandList::imguiFrame() {
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), get());
}
