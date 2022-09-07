#pragma once

#include "render/objects/device.h"
#include "render/objects/library.h"

namespace engine::render {
    struct Context;

    struct DisplayViewport {
        DisplayViewport(Context* context);

        void create();
        void destroy();

        ID3D12CommandList* populate();

    private:
        void createScreenQuad();
        void destroyScreenQuad();

        void createCommandList();
        void destroyCommandList();

        void createRootSignature();
        void destroyRootSignature();

        void createPipelineState();
        void destroyPipelineState();

        Device<ID3D12Device4>& getDevice();


        Context* ctx;
        VertexBuffer screenQuad;
        ShaderLibrary shaders;
        Object<ID3D12GraphicsCommandList> commandList;
        Object<ID3D12RootSignature> rootSignature;
        Object<ID3D12PipelineState> pipelineState;
    };
}
