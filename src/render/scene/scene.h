#pragma once

#include "render/render.h"

namespace engine::render {
    struct Scene {
        Scene(Context* ctx);
        virtual ~Scene() = default;
    };
}
