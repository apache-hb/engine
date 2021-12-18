#pragma once

#include <DirectXMath.h>
#include <atomic>

namespace engine::input {
    using namespace DirectX;

    struct Camera {
        Camera(XMFLOAT3 position, XMFLOAT3 direction, float fov = 90.f)
            : position(position)
            , direction(direction)
            , pitch(direction.x)
            , yaw(direction.y)
            , fov(fov)
        { }

        void move(float x, float y, float z);

        void rotate(float pitchChange, float yawChange);

        void store(XMFLOAT4X4 *view, XMFLOAT4X4 *projection, float aspect) const;
    private:
        std::atomic<XMFLOAT3> position;
        std::atomic<XMFLOAT3> direction;
        
        float pitch;
        float yaw;

        float fov;
    };
}
