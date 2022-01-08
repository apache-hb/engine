#pragma once

#include "render/util.h"

#include <atomic>

struct CameraDebugObject;

namespace engine::render {
    using namespace DirectX;
    struct Camera {
        Camera(XMFLOAT3 pos, XMFLOAT3 dir, float fov = 90.f);

        void move(float x, float y, float z);

        void rotate(float pitchChange, float yawChange);

        void store(XMFLOAT4X4* view, XMFLOAT4X4* projection, float aspect) const;
    
        XMFLOAT3 getPosition() const { return position; }
        XMFLOAT3 getDirection() const { return direction; }
        float getFov() const { return fov; }
        float getPitch() const { return pitch; }
        float getYaw() const { return yaw; }

    private:
        friend CameraDebugObject;

        XMFLOAT3 position;
        XMFLOAT3 direction;
        
        float pitch;
        float yaw;

        float fov;
    };

    cbuffer CameraBuffer {
        XMFLOAT4X4 model;
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
    };
}
