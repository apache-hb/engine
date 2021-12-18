#pragma once

#include <DirectXMath.h>
#include <atomic>

namespace engine::input {
    using namespace DirectX;

    struct Camera {
        Camera(XMFLOAT3 position, XMFLOAT3 rotation, float fov = 110.f)
            : position(position)
            , rotation(rotation)
            , fov(fov)
        { }

        void move(XMFLOAT3 direction);

        void rotate(float roll, float pitch, float yaw);

        void store(XMFLOAT4X4 *view, XMFLOAT4X4 *projection, float aspect) const;
    private:
        std::atomic<XMFLOAT3> position;
        std::atomic<XMFLOAT3> rotation;
        float fov;
    };
}
