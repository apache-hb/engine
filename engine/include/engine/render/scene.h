#pragma once

#include "engine/render/render.h"

#include <filesystem>
#include "engine/base/util.h"

#include "fastgltf/fastgltf_types.hpp"

namespace engine::render {
    namespace fg = fastgltf;

    struct GltfScene : Scene {
        struct Create {
            Camera &camera;
            std::filesystem::path path;
            rhi::TextureSize resolution = { 1920, 1080 };
        };

        GltfScene(Create&& info);

        ID3D12CommandList *populate(Context *ctx) override;
        ID3D12CommandList *attach(Context *ctx) override;

    private:
        void loadFile(const std::filesystem::path &path);

        UniquePtr<fg::Asset> asset;

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
