#pragma once

#include "engine/platform/platform.h"
#include <d3d12.h>

namespace engine::render {
    struct Context {
        Context(platform::Window *window);

    private:
        platform::Window *window;
    };
}
