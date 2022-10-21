#pragma once

#include "engine/math/math.h"

namespace simcoe::render {
    using namespace math;
    constexpr uint32_t kDebugNormals = (1 << 0);
    constexpr uint32_t kDebugUVs = (1 << 1);

    struct alignas(256) DebugBuffer {
        uint32_t debugFlags = 0;
    };

    // data that is shared across the entire scene
    struct alignas(256) SceneBuffer {
        float4x4 model;
        float4x4 view;
        float4x4 projection;
        
        math::float4x4 camera = math::float4x4::identity(); // current camera
        math::float3 light = { 0.f, 0.f, 0.f };
    };

    // data that is specific to each object
    struct alignas(256) ObjectBuffer {
        math::float4x4 transform = math::float4x4::identity();
    };
}
