#pragma once

#include "render/objects/resource.h"

namespace engine::render {
    struct Context;

    struct Scene {
        Scene(Context* context): ctx(context) { }

        virtual ~Scene() = default;

        virtual void create() = 0;
        virtual void destroy() = 0;

        virtual ID3D12CommandList* populate() = 0;

    protected:
        Context* ctx;
    };

    struct Scene3D : Scene {
        Scene3D(Context* context): Scene(context) { }
        virtual ~Scene3D() = default;

        virtual void create() override;
        virtual void destroy() override;

        virtual ID3D12CommandList* populate() override;

    private:
        void createCommandList();
        void destroyCommandList();

        void createDsvHeap();
        void destroyDsvHeap();

        void createDepthStencil();
        void destroyDepthStencil();

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle();

        Object<ID3D12DescriptorHeap> dsvHeap;
        Resource depthStencil;
        Object<ID3D12GraphicsCommandList> commandList;
        Object<ID3D12RootSignature> rootSignature;
        Object<ID3D12PipelineState> pipelineState;
    };
}
