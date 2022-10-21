#pragma once

#include "engine/math/math.h"

#include "engine/rhi/rhi.h"

namespace simcoe::render {
    using namespace math;

    struct Camera {
        Camera(float3 position, float fov) 
            : position(position)
            , fov(fov) 
        { }

        virtual ~Camera() = default;
        virtual float4x4 mvp(const float4x4 &model, float aspectRatio) const = 0;
        virtual float4x4 getView() const = 0;

        float4x4 getProjection(float aspectRatio) const;

        float3 getPosition() const { return position; }

    protected:
        float3 position;

        float fov;
    };

    struct Perspective final : Camera {
        Perspective(float3 position, float3 direction, float fov = 90.f);

        float4x4 mvp(const float4x4 &model, float aspectRatio) const override;
        float4x4 getView() const override;

        void move(float3 offset);
        void rotate(float yawUpdate, float pitchUpdate);

        float3 getDirection() const { return direction; }
    private:
        float3 direction;

        float pitch;
        float yaw;
    };

    struct LookAt final : Camera {
        LookAt(float3 position, float3 focus, float fov = 90.f);

        float4x4 mvp(const float4x4 &model, float aspectRatio) const override;
        float4x4 getView() const override;

        void setPosition(float3 update);
        void setFocus(float3 update);

    private:
        float3 focus;
    };
}
