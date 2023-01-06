#pragma once

#include <string>
#include <vector>
#include <functional>

#include "engine/base/math/math.h"

#include "engine/render/graph.h"

#include "engine/render/passes/imgui.h"

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

    struct WorldGraphInfo {
        Context& ctx;
        ImGuiUpdate update;
    };

    struct WorldGraph : Graph {
        WorldGraph(const WorldGraphInfo& info);

        void execute() { Graph::execute(primary); }

    private:
        Pass *primary;
    };
}
