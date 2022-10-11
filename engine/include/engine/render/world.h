#pragma once

#include "engine/rhi/rhi.h"

#include <vector>
#include <string_view>

namespace engine::render {
    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    struct Node {
        std::string_view name;

        math::float3 translation;
        math::float3 rotation;
        math::float3 scale;

        std::vector<size_t> children;
        size_t mesh;
    };

    struct Scene {
        std::string_view name;
        std::vector<size_t> nodes;
    };

    struct Texture {
        std::string_view name;

        rhi::TextureSize size;
        std::vector<std::byte> data;
    };

    struct Mesh {
        size_t texture;
        std::vector<uint32_t> indices;
    };

    struct World {
        size_t primaryScene;
        std::vector<Scene> scenes;

        std::vector<Node> nodes;

        std::vector<Texture> textures;

        std::vector<Mesh> meshes;

        std::vector<Vertex> verts;
    };

    World loadGltf(const char *path);
}
