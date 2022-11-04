#pragma once

#include <string>
#include <vector>

#include "engine/math/math.h"

#include "engine/render-new/graph.h"

namespace simcoe::assets {
    struct Vertex {
        math::float3 position;
        math::float3 normal;
        math::float2 uv;
    };
}

namespace simcoe::render {
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

    struct WorldGraph : Graph {
        WorldGraph(Context& ctx);

        void execute() {
            Graph::execute(primary);
        }

    private:
        Pass *primary;
    };
}
