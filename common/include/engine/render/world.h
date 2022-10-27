#pragma once

#include "engine/rhi/rhi.h"

#include <vector>
#include <string_view>

namespace simcoe::assets {
    struct Vertex {
        math::float3 position;
        math::float3 normal;
        math::float2 uv;
    };

    using VertexBuffer = std::vector<Vertex>;
    using IndexBuffer = std::vector<uint32_t>;

    struct Node {
        bool dirty = true;

        std::string name;

        math::float4x4 transform;

        std::vector<size_t> children;
        std::vector<size_t> primitives;
    };

    struct Texture {
        std::string name;
        
        rhi::TextureSize size;
        std::vector<std::byte> data;
    };

    struct Primitive {
        size_t texture;
        size_t verts;
        size_t indices;
    };

    struct World {
        void makeDirty(size_t node);
        bool clearDirty(size_t node);

        std::vector<Node> nodes;

        std::vector<Texture> textures;
        std::vector<VertexBuffer> verts;
        std::vector<IndexBuffer> indices;

        std::vector<Primitive> primitives;
    };

    World loadGltf(const char *path);
}
