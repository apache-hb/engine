#pragma once

#include "engine/math/math.h"

namespace engine::input {
    struct Input {
        math::float3 movement;
        math::float2 rotation;
    };

    bool poll(Input *input);
}
