#pragma once

#include "engine/math/math.h"

#include "engine/rhi/rhi.h"

namespace simcoe::render {
    using namespace math;

    struct Camera {
        virtual ~Camera() = default;
        virtual float4x4 mvp(const float4x4 &model, float aspectRatio) const = 0;
    };

    struct Perspective : Camera {
        Perspective(float3 position, float3 direction, float fov = 90.f);

        virtual float4x4 mvp(const float4x4 &model, float aspectRatio) const override;

        void move(float3 offset);
        void rotate(float yawUpdate, float pitchUpdate);

        const float3 getPosition() const { return position; }
        const float3 getDirection() const { return direction; }

    private:
        float3 position;
        float3 direction;

        float pitch;
        float yaw;

        float fov;
    };

    struct LookAt : Camera {
        LookAt(float3 position, float3 focus, float fov = 90.f);

        virtual float4x4 mvp(const float4x4 &model, float aspectRatio) const override;

        void setPosition(float3 update);
        void setFocus(float3 update);

    private:
        float3 position;
        float3 focus;

        float fov;
    };
}
