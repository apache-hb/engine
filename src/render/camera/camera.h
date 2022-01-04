#pragma once

#include "render/util.h"

#include <DirectXMath.h>
#include <atomic>

namespace engine::render {
    using namespace DirectX;

    struct Camera {
        Camera(XMFLOAT3 pos, XMFLOAT3 dir, float fov = 90.f)
            : position(pos)
            , direction(dir)
            , pitch(dir.x)
            , yaw(dir.y)
            , fov(fov)
        { }

        void move(float x, float y, float z);

        void rotate(float pitchChange, float yawChange);

        void store(XMFLOAT4X4* view, XMFLOAT4X4* projection, float aspect) const;
    
        XMFLOAT3 where() const { return position.load(); }
        XMFLOAT3 look() const { return direction.load(); }
        
    private:
        std::atomic<XMFLOAT3> position;
        std::atomic<XMFLOAT3> direction;
        
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
