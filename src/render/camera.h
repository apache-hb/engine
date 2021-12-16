#pragma once

#include "util.h"

namespace engine::render {
    struct Camera {
        Camera();
        
        cbuffer ConstBuffer {
            XMFLOAT4X4 model;
            XMFLOAT4X4 view;
            XMFLOAT4X4 projection;
        };

        float fov = 90.f;
        ConstBuffer buffer;
    };
}
