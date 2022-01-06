#pragma once

#include "render/util.h"
#include "math/math.h"

#include <atomic>

namespace engine::render {
    using namespace engine::math;

    struct Camera {
        Camera(float3 pos, float3 dir, float fov = 90.f)
            : position(pos)
            , direction(dir)
            , pitch(dir.x)
            , yaw(dir.y)
            , fov(fov)
        { }

        void move(float x, float y, float z);

        void rotate(float pitchChange, float yawChange);

        void store(float4x4* view, float4x4* projection, float aspect) const;
    
        float3 where() const { return position.load(); }
        float3 look() const { return direction.load(); }
        
        void imgui();

    private:
        std::atomic<float3> position;
        std::atomic<float3> direction;
        
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
