#pragma once

#include "engine/rhi/rhi.h"

#include <vector>
#include <string_view>

namespace engine::assets {
    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    using VertexBuffer = std::vector<Vertex>;
    using IndexBuffer = std::vector<uint32_t>;

    struct Node {
        std::string_view name;

        math::float4x4 transform;

        std::vector<size_t> children;
        std::vector<size_t> primitives;
    };

    struct Scene {
        std::string_view name;
        std::vector<size_t> nodes;
    };

    struct Texture {
        rhi::TextureSize size;
        std::vector<std::byte> data;
    };

    struct Primitive {
        size_t texture;
        size_t verts;
        size_t indices;
    };

    struct World {
        std::vector<Node> nodes;

        std::vector<Texture> textures;
        std::vector<VertexBuffer> verts;
        std::vector<IndexBuffer> indices;

        std::vector<Primitive> primitives;
    };

    World loadGltf(const char *path);
}
