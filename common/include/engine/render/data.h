#pragma once

#include "engine/math/math.h"

namespace simcoe::render {

    // data that is shared across the entire scene
    struct alignas(256) SceneBuffer {
        math::float4x4 camera = math::float4x4::identity(); // current camera
        math::float3 light = { 0.f, 0.f, 0.f };
    };

    // data that is specific to each object
    struct alignas(256) ObjectBuffer {
        math::float4x4 transform = math::float4x4::identity();
        uint32_t texture = 0;
    };
}
