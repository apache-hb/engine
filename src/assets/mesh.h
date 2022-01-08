#pragma once

#include <vector>
#include "math/math.h"

namespace engine::assets {
    using namespace math;

    struct Vertex {
        float3 position;
        float2 texcoord;
    };

    using VertexBuffer = std::vector<Vertex>;
    using IndexBuffer = std::vector<uint32_t>;

    struct IndexBufferView {
        size_t offset;
        size_t length;
    };
}
