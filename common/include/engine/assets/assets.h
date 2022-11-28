#pragma once

#include "engine/math/math.h"
#include "engine/rhi/rhi.h"

#include <vector>

namespace simcoe::assets {
    struct Vertex {
        math::float3 position;
        math::float3 normal;
        math::float2 uv;
    };

    using VertexBuffer = std::vector<Vertex>;
    using IndexBuffer = std::vector<uint32_t>;

    struct Texture {
        std::string name;
        rhi::TextureSize size;
        std::vector<uint8_t> data;
    };

    struct Primitive {
        size_t texture;
        size_t verts;
        size_t indices;
    };

    struct Node {
        std::string name;

        math::float3 position;
        math::float4 rotation;
        math::float3 scale;

        std::vector<size_t> children; // all child nodes
        std::vector<size_t> primitives; // all primitives that comprise this node
    };

    struct IWorldSink {
        virtual ~IWorldSink() = default;

        void loadGltfAsync(const char* path);

        virtual size_t addVertexBuffer(VertexBuffer&& verts) = 0;
        virtual size_t addIndexBuffer(IndexBuffer&& indices) = 0;

        virtual size_t addTexture(const Texture& texture) = 0;
        virtual size_t addPrimitive(const Primitive& primitive) = 0;
        virtual size_t addNode(const Node& node) = 0;
    };
}
