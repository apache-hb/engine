#pragma once

#include "engine/render/graph.h"

namespace simcoe::render {
    struct BufferResource : Resource {
        virtual rhi::Buffer& get() = 0;
        size_t cbvHandle() const { return cbvOffset; }

    protected:
        BufferResource(const Info& info, size_t handle);

        void addBarrier(Barriers& barriers, Output* output, Input* input) override;

    private:
        size_t cbvOffset;
    };

    struct RenderTargetResource final : BufferResource {
        RenderTargetResource(const Info& info);

        rhi::Buffer& get() override { return getContext().getRenderTarget(); }
        rhi::CpuHandle rtvCpuHandle() { return getContext().getRenderTargetHandle(); }
    };

    struct TextureResource : BufferResource {
        TextureResource(const Info& info, State initial, rhi::TextureSize size, bool hostVisible);

        rhi::Buffer& get() override { return buffer; }

        rhi::CpuHandle cbvCpuHandle() const { return getContext().getHeap().cpuHandle(cbvHandle(), 1, DescriptorSlot::eTexture); }
        rhi::GpuHandle cbvGpuHandle() const { return getContext().getHeap().gpuHandle(cbvHandle(), 1, DescriptorSlot::eTexture); }

    private:
        rhi::Buffer buffer;
    };

    struct SceneTargetResource final : TextureResource {
        SceneTargetResource(const Info& info, State initial);
        rhi::CpuHandle rtvHandle() const { return rtvOffset; }

    private:
        rhi::CpuHandle rtvOffset;
    };
}
