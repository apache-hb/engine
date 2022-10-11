#pragma once

#include "engine/render/render.h"
#include "engine/render/world.h"

#include "engine/base/util.h"

namespace engine::render {
    struct Object {
        std::vector<std::byte> vs;
        std::vector<std::byte> ps;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> inidices;
    };

    struct BasicScene : RenderScene {
        struct Create {
            Camera *camera;
            World *world;
            rhi::TextureSize resolution = { 1920, 1080 };
        };

        BasicScene(Create&& info);

        ID3D12CommandList *populate(Context *ctx) override;
        ID3D12CommandList *attach(Context *ctx) override;

    private:
        World *world;

        size_t cbvSetSize() const;

        rhi::View view;
        float aspectRatio;

        CameraBuffer camera;

        rhi::Allocator allocators[kFrameCount];

        rhi::DescriptorSet heap;
        rhi::CommandList commands;

        std::vector<rhi::Buffer> textures;
        MeshObject mesh;
    };
}
