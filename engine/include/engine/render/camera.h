#pragma once

#include "engine/math/math.h"

namespace engine::render {
    using namespace math;

    struct alignas(256) Camera {
        Camera(float3 position, float3 direction, float fov = 90.f);

        float4x4 mvp(const float4x4 &model, float aspectRatio) const;

        void move(float3 offset);
        void rotate(float2 update);

    private:
        float3 position;
        float3 direction;

        float pitch;
        float yaw;

        float fov;
    };
}
