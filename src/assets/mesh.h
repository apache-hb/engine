#pragma once

#include <vector>
#include "math/math.h"

namespace engine::assets {
    using namespace math;

    struct Vertex {
        float3 position;
        float2 texcoord;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };
}
