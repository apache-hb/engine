#pragma once

#include "engine/render/render.h"

namespace engine::render {
    struct GltfScene : Scene {
        struct Create {
            Camera &camera;
            rhi::TextureSize resolution = { 1920, 1080 };
        };

        GltfScene(Create&& info);

        ID3D12CommandList *populate(Context *ctx) override;
        ID3D12CommandList *attach(Context *ctx) override;

    private:
        rhi::View view;
        float aspectRatio;

        CameraBuffer camera;

        rhi::Allocator allocators[kFrameCount];

        rhi::DescriptorSet heap;
        rhi::CommandList commands;

        rhi::Buffer texture;
        Mesh mesh;
    };
}
