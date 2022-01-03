#pragma once

#include "render/util.h"

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

    protected:
        void createCommandList();
        void destroyCommandList();
        
        Object<ID3D12DescriptorHeap> dsvHeap;
        Object<ID3D12GraphicsCommandList> commandList;
        Object<ID3D12RootSignature> rootSignature;
        Object<ID3D12PipelineState> pipelineState;
    };
}
