#pragma once

#include "render/util.h"
#include "math/math.h"

#include <atomic>

struct CameraDebugObject;

namespace engine::render {
    using namespace math;
    struct Camera {
        Camera(float3 pos, float3 dir, float fov = 90.f);

        void move(float x, float y, float z);

        void rotate(float pitchChange, float yawChange);

        void store(float4x4* view, float4x4* projection, float aspect) const;
    
        float3 getPosition() const { return position; }
        float3 getDirection() const { return direction; }
        float getFov() const { return fov; }
        float getPitch() const { return pitch; }
        float getYaw() const { return yaw; }

    private:
        friend CameraDebugObject;

        float3 position;
        float3 direction;
        
        float pitch;
        float yaw;

        float fov;
    };

    cbuffer CameraBuffer {
        float4x4 model;
        float4x4 view;
        float4x4 projection;
    };
}
