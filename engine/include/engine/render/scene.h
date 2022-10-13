#pragma once

#include "engine/render/render.h"
#include "engine/render/world.h"
#include "engine/render/data.h"

#include "engine/base/util.h"

namespace engine::render {
    struct Object {
        std::vector<std::byte> vs;
        std::vector<std::byte> ps;

        std::vector<assets::Vertex> vertices;
        std::vector<uint32_t> inidices;
    };

    struct SceneBufferHandle {
        void update(Camera *camera, float aspectRatio);

        void attach(rhi::Device& device, rhi::CpuHandle handle);

        rhi::Buffer buffer;
        void *ptr;

        SceneBuffer data;
    };

    struct ObjectBufferHandle {
        ObjectBufferHandle(rhi::Device& device, rhi::CpuHandle handle);
        
        void update(math::float4x4 transform);

        rhi::Buffer buffer;
        void *ptr;

        ObjectBuffer data;
    };

    struct BasicScene : RenderScene {
        struct Create {
            Camera *camera;
            assets::World *world;
            rhi::TextureSize resolution = { 1920, 1080 };
        };

        BasicScene(Create&& info);

        ID3D12CommandList *populate(Context *ctx) override;
        ID3D12CommandList *attach(Context *ctx) override;

    private:
        Camera *camera;
        assets::World *world;

        void updateObject(size_t index, math::float4x4 parent);

        size_t cbvSetSize() const;
        
        rhi::GpuHandle getObjectBufferGpuHandle(size_t index);
        rhi::CpuHandle getObjectBufferCpuHandle(size_t index);
        
        rhi::GpuHandle getTextureGpuHandle(size_t index);
        rhi::CpuHandle getTextureCpuHandle(size_t index);

        rhi::View view;
        float aspectRatio;

        SceneBufferHandle sceneData;

        std::vector<ObjectBufferHandle> objectData;

        rhi::Allocator allocators[kFrameCount];

        rhi::DescriptorSet dsvHeap;
        rhi::Buffer depthBuffer;

        rhi::DescriptorSet cbvHeap;
        rhi::CommandList commands;

        std::vector<rhi::Buffer> vertexBuffers;
        std::vector<rhi::Buffer> indexBuffers;

        std::vector<rhi::VertexBufferView> vertexBufferViews;
        std::vector<rhi::IndexBufferView> indexBufferViews;

        std::vector<rhi::Buffer> textures;
        
        rhi::PipelineState pso;
    };
}
