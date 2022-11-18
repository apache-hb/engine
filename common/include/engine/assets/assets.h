#pragma once

#include "engine/math/math.h"

#include <vector>

namespace simcoe::assets {
    struct Vertex {
        math::float3 position;
        math::float3 normal;
        math::float2 uv;
    };

    struct Node {
        std::string name;
        size_t index; // cbv index

        math::float3 position;
        math::float4 rotation;
        math::float3 scale;

        std::vector<size_t> children; // all child nodes
        std::vector<size_t> primitives; // all primitives that comprise this node
    };

    struct Texture {
        std::string name;
        size_t index; // srv index
    };

    struct Primitive {
        size_t texture;
        size_t verts;
        size_t indices;
    };

    // basic scene graph system
    struct World {
        std::vector<Node> nodes;
        std::vector<Texture> textures;
        std::vector<Primitive> primitives;
    };
}
