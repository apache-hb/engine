#pragma once

#include "util.h"

namespace engine::render {
    struct Camera {
        Camera(XMFLOAT3 pos = XMFLOAT3(), XMFLOAT3 look = XMFLOAT3(1, 0, 0));

        cbuffer ConstBuffer {
            XMFLOAT4X4 world;
            XMFLOAT4X4 view;
            XMFLOAT4X4 projection;
        };

        void write(ConstBuffer *buffer, float fov, float aspect) const;

        XMFLOAT3 position;
        XMFLOAT3 target;
        XMFLOAT3 up;
    };
}
