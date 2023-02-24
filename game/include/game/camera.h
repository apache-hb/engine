#pragma once

#include "simcoe/math/math.h"

namespace game {
    using namespace simcoe::math;

    struct ICamera {
        ICamera(float3 position, float fov)
            : position(position)
            , fov(fov)
        { }

        virtual ~ICamera() = default;

        virtual float4x4 mvp(const float4x4& model, float aspectRatio) const = 0;

        float4x4 getProjection(float aspectRatio) const;

        float3 position;
        float fov;
    };

    struct FirstPerson : ICamera {
        FirstPerson(float3 position, float3 direction, float fov);

        float4x4 mvp(const float4x4& model, float aspectRatio) const override;

        void move(float3 offset);
        void rotate(float yawUpdate, float pitchUpdate);

        float3 direction;
        float pitch;
        float yaw;
    };

    struct ThirdPerson : ICamera {
        ThirdPerson(float3 focus, float3 position, float fov);

        float4x4 mvp(const float4x4& model, float aspectRatio) const override;
 
        void move(float3 to);
        void refocus(float3 to);

        float3 focus;
    };
}